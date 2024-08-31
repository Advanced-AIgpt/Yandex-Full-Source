# coding: utf-8
import codecs
import os
import pytest

from ..token_classifier import create_token_classifier
from vins_core.nlu.nontrainable_token_classifier import create_nontrainable_token_classifier
from vins_core.utils.data import TarArchive
from vins_core.nlu.features_extractor import create_features_extractor
from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.common.sample import Sample

TEST_DATA_DIR = os.path.join(os.path.dirname(__file__), 'test_data')


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
def data_gc_search(samples_extractor):
    se = samples_extractor
    gc_nlu = os.path.join(TEST_DATA_DIR, 'personal_assistant', 'general_conversation.nlu')
    search_nlu = os.path.join(TEST_DATA_DIR, 'personal_assistant', 'search.nlu')

    out = {}
    with codecs.open(gc_nlu, encoding='utf-8') as f:
        out['gc'] = se(FuzzyNLUFormat.parse_iter(f.readlines()).items)
    with codecs.open(search_nlu, encoding='utf-8') as f:
        out['search'] = se(FuzzyNLUFormat.parse_iter(f.readlines()).items)
    return out


@pytest.fixture(scope='module')
def features_gc_search(data_gc_search, feature_extractor):
    return {intent: feature_extractor(samples) for intent, samples in data_gc_search.iteritems()}


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
def features_taxi_alarm_with_embeddings(data_taxi_alarm, feature_extractor_with_embeddings):
    return {intent: feature_extractor_with_embeddings(samples) for intent, samples in data_taxi_alarm.iteritems()}


@pytest.mark.parametrize('parameters', [
    {'select_features': ['embeddings']},
    pytest.param({}, marks=pytest.mark.xfail(raises=NotImplementedError, strict=True)),
])
def test_nn_classifier_sample_features_input(features_taxi_alarm_with_embeddings, parameters):
    clf = create_token_classifier('rnn', vectorizer=False, **parameters)
    clf.train(features_taxi_alarm_with_embeddings)
    assert clf.final_estimator.input_dim == 3


@pytest.mark.parametrize("model_name", ['rnn', 'cnn'])
def test_nn_classifier(features_taxi_alarm_with_embeddings, feature_extractor_with_embeddings,
                       model_name):
    clf = create_token_classifier(model_name)
    clf.train(features_taxi_alarm_with_embeddings)
    output = clf(feature_extractor_with_embeddings([
        Sample.from_string('поехали до проспекта маршала жукова')
    ])[0])
    assert output['taxi'] > output['alarm']


@pytest.mark.parametrize("model_name", ['rnn', 'charcnn'])
def test_nn_classifier_applier(tmpdir, features_taxi_alarm_with_embeddings,
                               feature_extractor_with_embeddings, model_name):
    input = feature_extractor_with_embeddings(
        [Sample.from_string('поехали до проспекта маршала жукова')]
    )[0]

    trainer = create_token_classifier(model_name)
    trainer.train(features_taxi_alarm_with_embeddings)
    trainer_output = trainer(input)

    archive_path = os.path.join(tmpdir.strpath, 'archive.tar')
    with TarArchive(archive_path, 'w') as archive:
        trainer.save(archive, model_name)

    with TarArchive(archive_path) as archive:
        applier = create_nontrainable_token_classifier(model_name, archive=archive)
    applier_output = applier(input)

    for label in trainer.classes:
        assert abs(applier_output[label] - trainer_output[label]) < 1e-6, 'error is too high'

    tmpdir.remove()


@pytest.mark.slowtest
def test_class_weights_nn_classifier(features_gc_search):
    clf = create_token_classifier('rnn', encoder_dim=2, nb_epoch=10, batch_size=1)
    features = dict()
    features['gc'] = features_gc_search['gc'][:9]
    features['search'] = features_gc_search['search'][:90]

    result = clf.gridsearch(features, {
        'class_balancing': [None, 'freq']
    }, cv=3, average='macro', n_jobs=1, refit=False)
    unbalanced_score = result[result.param_rnnclassifiermodel__class_balancing.isnull()].mean_test_score.iloc[0]
    balanced_score = result[result.param_rnnclassifiermodel__class_balancing == 'freq'].mean_test_score.iloc[0]
    assert balanced_score > unbalanced_score
