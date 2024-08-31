# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import threading
import multiprocessing
import time
import Queue
import pytest
import requests_mock
import json
import boto3
import numpy as np
from datetime import datetime
from moto import mock_s3
from contextlib import contextmanager

from vins_core.common.sample import Sample
from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.nlu.token_classifier import create_token_classifier
from vins_core.nlu.nontrainable_token_classifier import create_nontrainable_token_classifier
from vins_core.nlu.features_extractor import create_features_extractor, FeaturesExtractorFactory
from vins_core.nlu.features.base import SampleFeatures
from vins_core.nlu.features.extractor.base import DenseFeatures
from vins_core.nlu.lookup_classifier import regexp_constructor
from vins_core.nlu.neural.metric_learning.metric_learning import TrainMode, MetricLearningFeaturesPostProcessor
from vins_core.nlu.neural.metric_learning.helpers import metric_learning_with_index
from vins_core.utils.misc import call_once_on_dict, is_close
from vins_core.ext.s3 import S3DownloadAPI
from vins_core.nlu.nlu_data_cache import NluDataCache
from vins_core.utils.data import open_resource_file

from vins_core.dm.request import create_request
from vins_core.utils.misc import gen_uuid_for_tests
from vins_core.common.annotations.wizard import WizardAnnotation


EPS = 1e-6

DSSM_RESOURCE = 'resource://dssm_embeddings/tf_model'


@pytest.fixture(scope='module')
def feature_extractor(parser):
    return create_features_extractor(
        parser=parser,
        ngrams={'n': 1},
        ner={},
        postag={},
        lemma={},
        case={}
    )


@pytest.fixture(scope='module')
def feature_extractor_with_embeddings(dummy_embeddings, parser):
    return create_features_extractor(
        parser=parser,
        ngrams={'n': 1}, ner={}, postag={},
        lemma={}, embeddings={'file': dummy_embeddings}, case={}
    )


@pytest.fixture(scope='module')
def data_taxi_alarm(samples_extractor):
    se = samples_extractor
    return {
        'alarm': se(FuzzyNLUFormat.parse_iter([
            'поставь будильник на "6 утра"(when)',
            'разбуди меня в "9 вечера"(when)',
        ]).items),
        'taxi': se(FuzzyNLUFormat.parse_iter([
            'поехали до "проспекта Маршала Жукова 3"(location_to)',
            'машину от "тверской улицы"(location_from) в "без 15 6 вечера"(when) не дороже "500 рублей"(price)',
        ]).items)
    }


@pytest.fixture(scope='module')
def features_taxi_alarm(data_taxi_alarm, feature_extractor):
    return {intent: feature_extractor(samples) for intent, samples in data_taxi_alarm.iteritems()}


@pytest.fixture(scope='module')
def features_taxi_alarm_with_embeddings(data_taxi_alarm, feature_extractor_with_embeddings):
    return {intent: feature_extractor_with_embeddings(samples) for intent, samples in data_taxi_alarm.iteritems()}


@pytest.fixture(scope='module')
def data_gc_search(samples_extractor):
    se = samples_extractor
    gc_nlu = 'vins_core/test/test_data/personal_assistant/general_conversation.nlu'
    search_nlu = 'vins_core/test/test_data/personal_assistant/search.nlu'

    out = {}
    with open_resource_file(gc_nlu) as f:
        out['gc'] = se(FuzzyNLUFormat.parse_iter(f.readlines()).items)
    with open_resource_file(search_nlu) as f:
        out['search'] = se(FuzzyNLUFormat.parse_iter(f.readlines()).items)
    return out


@pytest.fixture(scope='module')
def features_gc_search(data_gc_search, feature_extractor):
    return {intent: feature_extractor(samples) for intent, samples in data_gc_search.iteritems()}


def test_maxent_simple(feature_extractor):
    train_data = {
        'intent1': feature_extractor([Sample(tokens=['b', 'a']), Sample(tokens=['a', 'c'])]),
        'intent2': feature_extractor([Sample(tokens=['b', 'a']), Sample(tokens=['b', 'c'])]),
    }
    test_data = feature_extractor([Sample(tokens=['a', 'd']), Sample(tokens=['b', 'd'])])
    clf = create_token_classifier(model='maxent')
    clf.train(train_data)
    test1 = map(lambda p: round(p, 2), clf.predict(test_data[:1])[0])
    test2 = map(lambda p: round(p, 2), clf.predict(test_data[1:2])[0])
    assert test1[0]-test1[1] >= 0.6
    assert test2[1]-test2[0] >= 0.7


def test_sgb_simple(feature_extractor):
    clf = create_token_classifier(model='sgb', sparse=False)
    clf.train({
        'intent1': feature_extractor([Sample(tokens=['b', 'a']), Sample(tokens=['a', 'c'])]),
        'intent2': feature_extractor([Sample(tokens=['b', 'a']), Sample(tokens=['b', 'c'])])
    })
    test1 = map(lambda p: round(p, 2), clf.predict(feature_extractor([Sample(tokens=['a', 'd'])]))[0])
    test2 = map(lambda p: round(p, 2), clf.predict(feature_extractor([Sample(tokens=['b', 'd'])]))[0])
    assert test1[0]-test1[1] >= 0.6
    assert test2[1]-test2[0] >= 0.7


def test_baseline_single_intent(feature_extractor):
    clf = create_token_classifier(model='maxent')
    clf.train({
        'intent': feature_extractor([Sample(tokens=['a']), Sample(tokens=['b']), Sample(tokens=['c'])])
    })
    assert clf.predict(feature_extractor([Sample(tokens=['a'])])) == [[1.0]]
    assert clf.predict(feature_extractor([Sample(tokens=['d'])])) == [[1.0]]


def test_with_classifier_features(features_gc_search, feature_extractor, samples_extractor):
    clf = create_token_classifier(
        model='maxent',
        classifiers=[dict(
            model='charcnn',
            model_file='resource://query_subtitles_charcnn',
            feature='rankings'
        )]
    )
    clf.train(features_gc_search)
    samples = feature_extractor(samples_extractor([
        'как отдыхаем в майские?'
    ]))
    out = clf(samples[0])
    assert out['search'] > out['gc']


def test_transformers_pipeline(data_taxi_alarm, features_taxi_alarm,
                               feature_extractor, feature_extractor_with_embeddings, dummy_embeddings):

    def transform(clf, x):
        y = clf.get_input(x, reset_model=False)
        for name, transfomer in clf._model.steps[:-1]:
            y = transfomer.transform(y)
        return y[0]

    x = [Sample(['улица'])]

    # creating classifier with common one-hot features
    clf = create_token_classifier(
        model='maxent',
    )
    clf.train(features_taxi_alarm)
    one_hot_only = transform(clf, feature_extractor(x))

    # creating classifier with common one-hot features + embedding features
    clf = create_token_classifier(
        model='maxent'
    )
    clf.train({i: feature_extractor_with_embeddings(s) for i, s in data_taxi_alarm.iteritems()})

    one_hot_and_embs = transform(clf, feature_extractor_with_embeddings(x))

    # creating classifier with embedding features only
    feature_extractor_embeddings_only = create_features_extractor(
        embeddings={'file': dummy_embeddings}
    )
    clf = create_token_classifier(
        model='maxent', sparse=False
    )
    clf.train({i: feature_extractor_embeddings_only(s) for i, s in data_taxi_alarm.iteritems()})

    embs_only = transform(clf, feature_extractor_embeddings_only(x))

    assert one_hot_and_embs.getnnz() == one_hot_only.getnnz() + len(embs_only)


def test_select_features(features_taxi_alarm_with_embeddings):
    clf = create_token_classifier(
        model='maxent',
        select_features=('postag', 'case', 'embeddings'),
    )
    clf.train(features_taxi_alarm_with_embeddings)
    assert clf._model.named_steps['vectorizerfeaturespostprocessor'].vocabulary == [
        'case=UNKN',
        'case=accs',
        'case=gent',
        'case=loct',
        'postag=ADJF',
        'postag=NOUN',
        'postag=NPRO',
        'postag=PRCL',
        'postag=PREP',
        'postag=UNKN',
        'postag=VERB'
    ]


def test_charcnn_with_capitalization(samples_extractor, feature_extractor):
    clf = create_nontrainable_token_classifier(
        model='charcnn',
        model_file='resource://query_subtitles_charcnn'
    )

    samples = samples_extractor([
        'Кто такой Путин',
        'кто такой путин'
    ])
    features = feature_extractor(samples)
    assert clf(features[0]) == clf(features[1])


def test_crossvalidation(features_gc_search):
    assert create_token_classifier('maxent').crossvalidation(features_gc_search)[0] > 0.98


def test_gridsearch(features_gc_search):
    report = create_token_classifier('maxent').gridsearch(features_gc_search, dict(
        C=[0.001, 100]
    ), n_jobs=1)
    fscore_at_C_001 = report.mean_test_score[report.param_logisticregression__C == 0.001].iloc[0]
    fscore_at_C_100 = report.mean_test_score[report.param_logisticregression__C == 100].iloc[0]
    assert fscore_at_C_001 < fscore_at_C_100


def test_inference_in_multithreading(monkeypatch, samples_extractor):
    monkeypatch.setenv('VINS_LOAD_TF_ON_CALL', False)
    clf = create_nontrainable_token_classifier(
        model='charcnn',
        model_file='resource://query_subtitles_charcnn'
    )
    fex = create_features_extractor()

    samples_features = fex(samples_extractor([
        'кто такой путин',
        'а ты кто такой'
    ]))
    output = []

    def run(sample):
        scores = clf(sample)
        output.append((sample.sample.text, max(scores, key=scores.get)))

    threads = [threading.Thread(target=run, args=(sample_features,)) for sample_features in samples_features]
    [t.start() for t in threads]
    [t.join() for t in threads]
    assert set(output) == {
        ('кто такой путин', 'search'),
        ('а ты кто такой', 'gc')
    }


def test_inference_in_multiprocessing_init_before_fork(monkeypatch, samples_extractor):
    """
    This test checks expected behaviour of tensorflow hanging when first call is made before fork.
    """
    fex = create_features_extractor()
    samples_features = fex(samples_extractor([
        'кто такой путин',
        'а ты кто такой'
    ]))

    def run(clf, sample_features, outq):
        result = None
        try:
            result = clf(sample_features)
        except Exception:
            pass
        outq.put(result)

    def check(clf):
        outq = multiprocessing.Queue()
        processes = [multiprocessing.Process(target=run, args=(clf, sample_features, outq))
                     for sample_features in samples_features]
        [p.start() for p in processes]
        for p in processes:
            p.join(timeout=2)
            if p.exitcode is None:
                p.terminate()
        try:
            output = [outq.get(timeout=1) for _ in xrange(len(samples_features))]
        except Queue.Empty:
            return 'process hangs'
        if output and all(item is not None for item in output):
            return 'process terminated'
        else:
            return 'process raised error'

    clf = create_nontrainable_token_classifier(
        model='charcnn',
        model_file='resource://query_subtitles_charcnn'
    )
    res = check(clf)
    assert res == 'process terminated'


def _clf_on_text(clf, text):
    return clf(SampleFeatures(Sample.from_string(text)))


def _make_yt_mock(yt_path, result, status_code=200):
    m = requests_mock.mock()
    m.get('http://hahn.yt.yandex.net/api/v3/read_table?path=%s' % yt_path, text=result, status_code=status_code)
    m.get('http://hahn.yt.yandex.net/api/v3/get', text='"qwerty"')
    return m


@contextmanager
def make_s3_mock(bucket_name, source_key, content):
    mock = mock_s3()
    mock.start()
    s3 = boto3.client('s3')
    s3.create_bucket(Bucket=bucket_name)
    s3.put_object(Bucket=bucket_name, Key=source_key, Body=content)
    yield mock
    mock.stop()


@pytest.mark.parametrize("table_data, result", (
    ([{'intent': 'gc', 'text': 'какая погода на марсе'}], {'gc': 1}),
    ([{'intent': 'gc', 'text': 'какая погода на марсе?'}, {'intent': 'nogc', 'text': 'ффф'}], {'gc': 1, 'nogc': 0}),
    ([], {}),
))
def test_yt_lookup(table_data, result, samples_extractor):
    mock_yt_path = '//mock-yt-path'
    table_data_encoded = '\n'.join(json.dumps(line, ensure_ascii=False) for line in table_data)
    intent_infos = {line['intent']: [None] for line in table_data}
    with _make_yt_mock(mock_yt_path, table_data_encoded):
        clf = create_token_classifier('yt_lookup', source=mock_yt_path, samples_extractor=samples_extractor,
                                      intent_infos=intent_infos)
        assert _clf_on_text(clf, 'какая погода на марсе') == result


@pytest.mark.parametrize('utterance, result', (
    ('текст 123', {'intent_d': 1, 'intent_w': 0, 'intent_p': 0}),
    ('фраза 123', {'intent_d': 1, 'intent_w': 0, 'intent_p': 0}),
    ('тут что угодно текст 123', {'intent_d': 0, 'intent_w': 0, 'intent_p': 0}),
    ('текст 123 тут что угодно', {'intent_d': 0, 'intent_w': 0, 'intent_p': 0}),
    ('текст абв', {'intent_d': 0, 'intent_w': 1, 'intent_p': 0}),
    ('фраза абв', {'intent_d': 0, 'intent_w': 1, 'intent_p': 0}),
    ('фразочка', {'intent_d': 0, 'intent_w': 0, 'intent_p': 1}),
    ('префикс фразочка', {'intent_d': 0, 'intent_w': 0, 'intent_p': 1}),
    ('фразочка суффикс', {'intent_d': 0, 'intent_w': 0, 'intent_p': 1}),
    ('префикс фразочка суффикс', {'intent_d': 0, 'intent_w': 0, 'intent_p': 1})
))
def test_regexp_lookup(utterance, result):
    clf = create_token_classifier('data_lookup', intent_texts={
        'intent_d': [r'текст\s\d+', r'фраза\s\d+'],
        'intent_w': [r'текст\s[абв]+', r'фраза\s[абв]+'],
        'intent_p': [r'.*фразочка.*']
    }, regexp=True, intent_infos={'intent_d': [None], 'intent_w': [None], 'intent_p': [None]})
    assert _clf_on_text(clf, utterance) == result


def test_joint_regexp_lookup():
    clf = create_token_classifier('data_lookup', intent_texts={
        'intent_d': [r'текст\s\d+', r'фраза\s\d+'],
        'intent_w': [r'текст\s[абв]+', r'фраза\s[абв]+'],
        'intent_p': [r'.*фразочка.*']
    }, regexp=True, intent_infos={'intent_d': [None], 'intent_w': [None], 'intent_p': [None]})
    patterns = {item[0] for item in clf._matcher._lookup.keys()}
    assert regexp_constructor(r'^(?:(?:текст\s\d+)|(?:фраза\s\d+))$') in patterns


def test_matching_score_lookup_classifier():
    clf = create_token_classifier('data_lookup',
                                  regexp=True,
                                  default_score=1,
                                  intent_texts={'other': ["включи .*"], 'weather': ["точно не запрос про погоду"]},
                                  matching_score=0.5,
                                  nonmatching_score=1.5,
                                  intent_infos={'other': [None], 'weather': [None]})
    assert _clf_on_text(clf, "включи музыку") == {'other': 0.5, 'weather': 1.5}
    assert _clf_on_text(clf, "выключи музыку") == {'other': 1.0, 'weather': 1.0}


def test_s3_lookup_classifier_update(mocker):
    mocker.patch.object(S3DownloadAPI, 'get_if_modified', return_value=(
        True,
        datetime(1988, 2, 16),
        json.dumps([{'text': 'тест', 'intent': 'intent1'}])
    ))

    clf = create_token_classifier('s3_lookup', source='/test.json', update_period=0.01,
                                  intent_infos={'intent1': [None], 'intent2': [None]})
    clf._updater.start()
    time.sleep(0.5)

    assert _clf_on_text(clf, 'тест') == {'intent1': 1.0, 'intent2': 0.0}
    assert _clf_on_text(clf, 'тест2') == {'intent1': 0.0, 'intent2': 0.0}

    mocker.patch.object(S3DownloadAPI, 'get_if_modified', return_value=(
        True,
        datetime(1988, 2, 17),
        json.dumps([{'text': 'тест2', 'intent': 'intent2'}])
    ))

    time.sleep(0.5)

    assert _clf_on_text(clf, 'тест') == {'intent1': 0.0, 'intent2': 0.0}
    assert _clf_on_text(clf, 'тест2') == {'intent1': 0.0, 'intent2': 1.0}
    clf._updater.stop()


def test_passing_intent_infos():
    clf = create_token_classifier('data_lookup',
                                  regexp=True,
                                  intent_infos={'other': [None], 'music': [None], 'video': [None]},
                                  intent_texts={'music': ["включи .*"], 'weather': ["запрос про погоду"]}
                                  )
    assert _clf_on_text(clf, "включи музыку") == {'other': 0, 'music': 1, 'video': 0}


@pytest.mark.parametrize('utterance, result', (
    ('a', {'intent1': 1, 'intent2': 1, 'intent3': 1}),
    ('b', {'intent1': 0, 'intent2': 1, 'intent3': 1}),
    ('c', {'intent1': 0, 'intent2': 0, 'intent3': 1}),
))
def test_regexp_multi_match(utterance, result):
    clf = create_token_classifier('data_lookup', intent_texts={
        'intent1': ['a'],
        'intent2': ['a', 'b'],
        'intent3': ['.*']
    }, regexp=True, intent_infos={'intent1': [None], 'intent2': [None], 'intent3': [None]})
    assert _clf_on_text(clf, utterance) == result


@pytest.mark.parametrize('use_hnsw', [False, True])
def test_knn(features_taxi_alarm_with_embeddings, feature_extractor_with_embeddings, use_hnsw):
    clf = create_token_classifier(
        'knn',
        sparse=False,
        metric_learning=TrainMode.NO_METRIC_LEARNING,
        use_hnsw=use_hnsw,
        num_neighbors=1,
    )
    clf.train(features_taxi_alarm_with_embeddings)
    test = feature_extractor_with_embeddings([Sample.from_string('будильник на восемь')])[0]
    result = clf(test)
    assert result['alarm'] > result['taxi']


@pytest.fixture(scope='module')
def dssm_embeddings():
    return {'resource': DSSM_RESOURCE}


@pytest.fixture(scope='module')
def features_nlu_demo_sequential_and_global(dummy_embeddings, parser, nlu_demo_samples, dssm_embeddings):
    feature_extractor = create_features_extractor(
        parser=parser,
        ngrams={'n': 1}, ner={},
        embeddings={'file': dummy_embeddings},
        dssm_embeddings=dssm_embeddings,
        classifier=dict(
            model='charcnn',
            model_file='resource://query_subtitles_charcnn',
            feature='scores'
        )
    )
    return call_once_on_dict(feature_extractor, nlu_demo_samples)


@pytest.mark.parametrize('use_hnsw', [False, True])
def test_knn_metric_learning(tmpdir, features_nlu_demo_sequential_and_global, use_hnsw):
    model_dir = str(tmpdir.mkdir('metric_learning'))

    params = {
        'model_dir': model_dir,
        'sparse': False,
        'convert_to_prob': False,
        'metric_function': 'metric_learning',
        'num_neighbors': 2,
        'checkpoint_freq_in_batches': 1,
        'use_hnsw': use_hnsw,
    }

    clf = metric_learning_with_index(features_nlu_demo_sequential_and_global, params)

    result = clf(features_nlu_demo_sequential_and_global['alarm'][0])
    assert result['alarm'] > result['taxi']
    assert result['alarm'] == 1
    result = clf(features_nlu_demo_sequential_and_global['taxi'][0])
    assert result['taxi'] > result['alarm']
    assert is_close(result['taxi'], 1)


@pytest.mark.parametrize('use_hnsw', [False, True])
def test_knn_metric_learning_encoder_applier(tmpdir, features_nlu_demo_sequential_and_global, use_hnsw):
    model_dir = str(tmpdir.mkdir('metric_learning'))

    params = {
        'model_dir': model_dir,
        'sparse': False,
        'convert_to_prob': False,
        'metric_function': 'metric_learning',
        'num_neighbors': 2,
        'checkpoint_freq_in_batches': 1,
        'use_hnsw': use_hnsw,
    }

    classifier = metric_learning_with_index(features_nlu_demo_sequential_and_global, params)

    input = features_nlu_demo_sequential_and_global['alarm'][0]
    output = classifier(input)

    for transformer in classifier._pipeline:
        if isinstance(transformer, MetricLearningFeaturesPostProcessor):
            transformer.make_applier()
    new_output = classifier(input)

    def check_is_subdict(dict1, dict2):
        for key, value in dict1.iteritems():
            assert key in dict2, 'no {} in dict'.format(key)
            assert abs(value - dict2[key]) < EPS

    check_is_subdict(output, new_output)
    check_is_subdict(new_output, output)


@pytest.mark.parametrize("old_features, new_features, oov", [
    (dict(ngrams={'n': 1}), dict(ngrams={'n': 1}, ner={}), 'ignore'),
    pytest.param(
        dict(ngrams={'n': 1}), dict(ngrams={'n': 1}, ner={}), 'error',
        marks=pytest.mark.xfail(raises=KeyError, strict=True, reason='OOV tokens found in new classifier')
    ),
    pytest.param(
        dict(ngrams={'n': 1}), dict(ngrams={'n': 1}, ner={}), 'index',
        marks=pytest.mark.xfail(strict=True, reason='New classifier adds OOV token')
    ),
    pytest.param(
        dict(
            ngrams={'n': 1},
            dssm_embeddings={"resource": DSSM_RESOURCE}
        ), dict(
            ngrams={'n': 1},
            classifier=dict(
                model='charcnn',
                model_file='resource://query_subtitles_charcnn',
                feature='scores'
            )
        ), 'ignore',
        marks=pytest.mark.xfail(raises=ValueError, strict=True, reason='Changing dense dimensions is not implemented')
    ),
])
@pytest.mark.parametrize('use_hnsw', [False, True])
def test_metric_learning_changing_feature_space(parser, tmpdir, nlu_demo_samples, old_features, new_features, oov, use_hnsw):
    fex_old = create_features_extractor(parser=parser, **old_features)
    fex_new = create_features_extractor(parser=parser, **new_features)

    features = call_once_on_dict(fex_old, nlu_demo_samples)
    features_with_new_vars = call_once_on_dict(fex_new, nlu_demo_samples)

    model_dir = str(tmpdir.mkdir('metric_learning'))

    params = {
        'model_dir': model_dir,
        'sparse': False,
        'convert_to_prob': False,
        'metric_function': 'metric_learning',
        'oov': oov,
        'checkpoint_freq_in_batches': 1,
        'use_hnsw': use_hnsw,
        'num_neighbors': 1,
    }
    clf = metric_learning_with_index(features, params)

    clf_metric_learning_fixed = create_token_classifier(
        'knn', model_dir=model_dir, sparse=False, convert_to_prob=False, metric_function='metric_learning',
        metric_learning=TrainMode.NO_METRIC_LEARNING, raise_on_inconsistent_input=False,
        restore_weights_mode='last', oov=oov, use_hnsw=use_hnsw, num_neighbors=1,
    )
    clf_metric_learning_fixed.train(features_with_new_vars)

    r_old = clf(features['alarm'][0])
    r_new = clf_metric_learning_fixed(features['alarm'][0])
    assert r_old == r_new


def _knn_and_extractor_on_dssm_embeddings(nlu_demo_samples, metric_function, dssm_embeddings, model_dir=None):
    fex = create_features_extractor(dssm_embeddings=dssm_embeddings)
    train_data = {
        intent: fex(samples)
        for intent, samples in nlu_demo_samples.iteritems()
    }
    params = {
        'convert_to_prob': False, 'sparse': False, 'metric_function': metric_function, 'model_dir': model_dir,
        'num_epochs': 100, 'train_split': 1, 'batch_samples_per_class': 1, 'combiner_dropout': 0.5,
        'num_neighbors': 1,
    }

    knn = metric_learning_with_index(train_data, params)

    return knn, fex


@pytest.mark.slowtest
@pytest.mark.parametrize('use_metric_learning', (True, False))
@pytest.mark.parametrize('utt', ['добрый день', ''])
def test_knn_dssm(tmpdir, use_metric_learning, utt, nlu_demo_samples, dssm_embeddings):
    if use_metric_learning:
        knn, fex = _knn_and_extractor_on_dssm_embeddings(
            nlu_demo_samples,
            metric_function='metric_learning',
            dssm_embeddings=dssm_embeddings,
            model_dir=str(tmpdir.mkdir('metric_learning'))
        )
        vectors = knn.final_estimator._vectors
        assert vectors.shape[1] == 100
    else:
        knn, fex = _knn_and_extractor_on_dssm_embeddings(
            nlu_demo_samples,
            metric_function='euclidean',
            dssm_embeddings=dssm_embeddings
        )
        vectors = knn.final_estimator._vectors
        assert vectors.shape[1] == 50
    test_data = fex([Sample.from_string(utt)])[0]
    labels = knn.final_estimator._labels
    xcorr = np.dot(vectors, vectors.T)
    positives = labels[:, np.newaxis] == labels[np.newaxis, :]
    positive_scores = np.extract(positives, xcorr)
    negative_scores = np.extract(np.logical_not(positives), xcorr)
    assert np.mean(positive_scores) > np.mean(negative_scores)
    result = knn(test_data)
    assert all(score < 1 for score in result.itervalues())


def make_feature(text, vec):
    feature = SampleFeatures(Sample.from_string(text))
    feature.add(DenseFeatures(np.array(vec)), 'vec')
    return feature


def test_knn_update_only():
    trainset_1 = {
        'hello': [make_feature(*p) for p in [('привет', [1, 0, 0]), ('дратути', [0.5, 0.2, 0.2])]],
        'bye': [make_feature(*p) for p in [('до свидания', [0, 0.8, 0.2]), ('пока', [0.5, 0.7, 0.3])]]
    }
    trainset_2 = {
        'hello': [make_feature(*p) for p in [('здравствуйте', [2, 0, 0])]],
        'how are you': [make_feature(*p) for p in [('как дела', [0, 0, 2])]]
    }
    model = create_token_classifier('knn', num_neighbors=3)
    model.train(trainset_1)
    assert sorted(model.final_estimator._texts) == sorted(['привет', 'дратути', 'до свидания', 'пока'])
    model.train(trainset_2, update_only=True)
    assert sorted(model.final_estimator._texts) == sorted(['здравствуйте', 'как дела', 'до свидания', 'пока'])
    model.train(trainset_2, update_only=False)
    assert sorted(model.final_estimator._texts) == sorted(['здравствуйте', 'как дела'])


def test_clf_on_empty_phrases(parser_base, samples_extractor, dummy_embeddings, dssm_embeddings):
    fex = create_features_extractor(
        parser=parser_base,
        embeddings={'file': dummy_embeddings}, ner={}, postag={},
        dssm_embeddings=dssm_embeddings
    )
    fake_samples = ['��', ', ,,, ,', '.', '^_^', '©', ')', ':-\\', '♉♈♐�', '?-**; , ,', '...',
                    ';-)', ')))', '))(', ':|', '??', '☆', '?-**; , ,', ]
    train_data = {
        'intent1': samples_extractor(FuzzyNLUFormat.parse_iter(['', ' ', 'asd'] + fake_samples).items,
                                     filter_errors=True),
        'intent2': samples_extractor(FuzzyNLUFormat.parse_iter(['{}', 'fgh']).items,
                                     filter_errors=True)
    }
    train_data = call_once_on_dict(fex, train_data)
    clf = create_token_classifier('maxent')
    clf.train(train_data)
    r = clf(fex(samples_extractor(FuzzyNLUFormat.parse_iter(['��']).items, filter_errors=True))[0])
    assert r.values() == [0, 0]


@pytest.mark.parametrize('utt, features, winner', [
    ('мегафон', ('ngrams', 'bagofchar'), 'megafon'),
    ('мигафон', ('ngrams',), 'yota'),
    ('мигафон', ('ngrams', 'bagofchar'), 'megafon')
])
def test_bagofchar_mispeller(utt, features, winner):
    factory = FeaturesExtractorFactory()
    features_cfg = [
        {'type': 'ngrams', 'id': 'ngrams', 'params': {'n': 1}},
        {'type': 'bagofchar', 'id': 'bagofchar', 'params': {'n': 3}},
        {'type': 'bagofchar', 'id': 'bagofchar', 'params': {'n': 3}}
    ]
    for cfg in features_cfg:
        factory.add(cfg['id'], cfg['type'], **cfg['params'])

    fex = factory.create_extractor()
    sample_features = {
        'megafon': fex([Sample.from_string('мегафон')]),
        'yota': fex([Sample.from_string('йота')])
    }
    clf = create_token_classifier('maxent', select_features=features)
    clf.train(sample_features)
    result = clf(fex([Sample.from_string(utt)])[0])
    assert winner == max(result, key=result.get)


@pytest.fixture
def nlu_sources_data(scope='module'):
    nlu_content_1 = [
        "сколько будет 'пять'(percent) процентов от '100'(value)?",
        "в каком 'городе'(town) живет 'Баба-яга'(person)?",
        "назови три самых популярных мемчика"
    ]

    nlu_content_2 = [
        "кто такой 'Кощей Бессмертный'(name)?",
        "назови три самых популярных мемчика"
    ]

    nlu_sources_data = NluDataCache()
    nlu_sources_data.add('intent1', FuzzyNLUFormat.parse_iter(nlu_content_1).items)
    nlu_sources_data.add('intent2', FuzzyNLUFormat.parse_iter(nlu_content_2).items)

    return nlu_sources_data


@pytest.mark.parametrize('utt, exact_matched_intents, result', [
    ('сколько будет пять процентов от 100', [], {}),
    ('сколько будет пять процентов от 100', ['intent1'], {'intent1': 1.0}),
    ('в каком городе живет Баба-яга?', [], {}),
    ('в каком городе живет Баба-яга?', ['intent1'], {'intent1': 1.0}),
    ('кто такой Кощей Бессмертный?', [], {}),
    ('назови три самых популярных мемчика', ['intent2'], {'intent2': 1.0}),
    ('назови три самых популярных мемчика', ['intent1', 'intent2'], {'intent1': 1.0, 'intent2': 1.0})
])
def test_exact_match_classifier(utt, exact_matched_intents, result, nlu_sources_data, samples_extractor):
    clf = create_token_classifier(model='nlu_exact_matching', name='test', samples_extractor=samples_extractor,
                                  exact_matched_intents=exact_matched_intents, nlu_sources_data=nlu_sources_data)

    assert _clf_on_text(clf, samples_extractor([utt])[0].text) == result


@pytest.fixture(params=[
    None, {'granet_classifier': 'personal_assistant.scenarios.repeat_after_me'}
])
def req_info(request):
    return create_request(gen_uuid_for_tests(), experiments=request.param)


def test_granet_token_classifier(samples_extractor, req_info):
    features_extractor = create_features_extractor(granet={})
    token_classifier = create_token_classifier(model='granet')

    form_name = 'personal_assistant.scenarios.repeat_after_me'
    granet_response = {
        'Granet': {
            'Tokens': [
                {'Begin': 0, 'End': 10, 'Text': 'алиса'},
                {'Begin': 11, 'End': 25, 'Text': 'повтори'},
                {'Begin': 26, 'End': 30, 'Text': 'ка'},
                {'Begin': 31, 'End': 35, 'Text': 'за'},
                {'Begin': 36, 'End': 44, 'Text': 'мной'},
                {'Begin': 45, 'End': 55, 'Text': 'алиса'}
            ],
            'Forms': [
                {
                    'Name': form_name,
                    'LogProbability': -24.62995529,
                    'Tags': [{'Begin': 5, 'End': 6, 'Name': 'request'}]
                }
            ],
            'Text': 'алиса повтори-ка за мной алиса'
        }
    }

    utterance = 'алиса повтори-ка за мной алиса'
    sample = samples_extractor([utterance])[0]
    sample.annotations['wizard'] = WizardAnnotation(markup={}, rules=granet_response)
    features = features_extractor([sample])[0]

    predictions = token_classifier(features, req_info=req_info)

    if req_info.experiments['granet_classifier'] is not None:
        assert len(predictions) == 1
        assert predictions[form_name] == 1.
    else:
        assert len(predictions) == 0
