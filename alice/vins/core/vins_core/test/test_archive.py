# coding: utf-8
from __future__ import unicode_literals

import mock
import pytest
import os
import attr

from copy import deepcopy
from uuid import uuid4 as gen_uuid
from multiprocessing.pool import ThreadPool


from vins_core.common.sample import Sample
from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.dm.form_filler.dialog_manager import DialogManager
from vins_core.dm.session import Session
from vins_core.nlu.token_tagger import create_token_tagger
from vins_core.nlu.token_classifier import create_token_classifier
from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.sample_processors.normalize import NormalizeSampleProcessor
from vins_core.nlu.sample_processors.wizard import WizardSampleProcessor
from vins_core.nlu.neural.metric_learning.metric_learning import TrainMode
from vins_core.nlu.features_extractor import create_features_extractor, FeaturesExtractorFactory
from vins_core.nlu.neural.metric_learning.helpers import metric_learning_with_index

from vins_core.app_utils import compile_app
from vins_core.utils.data import get_resource_full_path
from vins_core.utils.archives import TarArchive
from vins_core.utils.misc import is_close


from vins_sdk.app import VinsApp
from vins_sdk.connectors import TestConnector


class DummyArch(TarArchive):
    def __init__(self, *args, **kwargs):
        super(DummyArch, self).__init__(*args, **kwargs)
        self._data = {}

    def add(self, name, path):
        super(DummyArch, self).add(name, path)
        self._data[self._get_name(name)] = path

    def get_by_name(self, name):
        super(DummyArch, self).get_by_name(name)
        return self._data.get(self._get_name(name))

    def _list_all(self):
        with mock.patch.object(self._tar, 'getnames',
                               return_value=self._data.keys()):
            for i in super(DummyArch, self)._list_all():
                yield i


@pytest.fixture
def dummy_arch(mocker):
    mocker.patch('vins_core.utils.data.tarfile.open')
    mocker.patch('vins_core.utils.data.shutil.move')
    yield DummyArch('', mode='w')


@pytest.fixture(scope='function')
def tar_archive(mocker, tmpdir):
    tar_file = tmpdir.join('archive.tar.gz').strpath
    mocker.patch('vins_core.utils.data.vins_temp_dir', lambda: tmpdir.strpath)
    arch = TarArchive(tar_file, mode=b'w:gz')
    yield arch
    tmpdir.remove()


@pytest.fixture(scope='module')
def feature_extractor():
    return create_features_extractor(ngrams={'n': 1})


@pytest.fixture(scope='module')
def feature_extractor_embeddings(dummy_embeddings):
    return create_features_extractor(ngrams={'n': 1}, embeddings={'file': dummy_embeddings})


@pytest.fixture
def classifier(feature_extractor):
    f = feature_extractor
    cls = create_token_classifier(
        model='maxent',
        classifiers=[dict(
            model='charcnn',
            model_file='resource://query_subtitles_charcnn',
            feature='scores'
        )]
    )
    cls.train({
        'intent1': f([Sample.from_string('ba', tokens=['b', 'a']), Sample.from_string('ac', tokens=['a', 'c'])]),
        'intent2': f([Sample.from_string('ba', tokens=['b', 'a']), Sample.from_string('bc', tokens=['b', 'c'])])
    })
    return cls


@pytest.fixture
def combine_scores_classifier_config():
    config = {
        'name': 'combine_scores',
        'model': 'combine_scores',
        'method': 'best_score',
        'classifiers': [
            {
                'model': 'maxent',
                'name': 'maxent',
                'classifiers': [
                    {
                        'model': 'charcnn',
                        'name': 'charcnn',
                        'model_file': 'query.subtitles.2M.charcnn.tar.gz',
                        'feature': 'scores'
                    }
                ]
            }
        ]
    }
    return config


@pytest.fixture
def combine_scores_classifier(feature_extractor, combine_scores_classifier_config):
    f = feature_extractor
    cls = create_token_classifier(**combine_scores_classifier_config)
    cls.train({
        'intent1': f([Sample.from_string('ba', tokens=['b', 'a']), Sample.from_string('ac', tokens=['a', 'c'])]),
        'intent2': f([Sample.from_string('ba', tokens=['b', 'a']), Sample.from_string('bc', tokens=['b', 'c'])])
    })
    return cls


@pytest.fixture(scope='module')
def samples_extractor_with_wizard():
    pipeline = [NormalizeSampleProcessor('normalizer_ru'), WizardSampleProcessor()]
    return SamplesExtractor(pipeline, allow_wizard_request=True)


@pytest.fixture(scope='module', params=[
    {'use_wizard': True},
    {'use_wizard': False}
])
def feature_extractor_prod(dummy_embeddings, request, parser):
    factory = FeaturesExtractorFactory()
    factory.register_parser(parser)
    factory.add('postag', 'postag')
    factory.add('ner', 'ner')
    if request.param['use_wizard']:
        factory.add('wizard', 'wizard')
    factory.add('alice_requests_emb', 'embeddings', file=dummy_embeddings)
    factory.add('word', 'ngrams', n=1)
    return factory.create_extractor()


@pytest.fixture(params=['intent', 'tagger'])
def intent_or_tagger(request, classifier, tagger):
    if request.param == 'intent':
        return classifier
    else:
        return tagger


@pytest.fixture
def tagger(feature_extractor, normalizing_samples_extractor):
    tagger = create_token_tagger('crf')
    norm = normalizing_samples_extractor
    train_data = feature_extractor(norm(FuzzyNLUFormat.parse_iter([
        'поехали до "проспекта Маршала Жукова 3"(location_to)',
        'машину от "тверской улицы"(location_from)',
    ]).items))

    tagger.train(train_data)
    return tagger


@pytest.fixture
def combine_scores_tagger_config():
    params = {
        "taggers": [
            {
                "model": "crf",
                "name": "tagger1"
            },
            {
                "model": "rnn_new",
                "name": "tagger2"
            }
        ]
    }
    return params


@pytest.fixture
def combine_scores_tagger(feature_extractor_prod, combine_scores_tagger_config):
    tagger = create_token_tagger('combine_scores', **combine_scores_tagger_config)
    norm = SamplesExtractor([NormalizeSampleProcessor('normalizer_ru')], allow_wizard_request=True)
    train_data = feature_extractor_prod(norm(FuzzyNLUFormat.parse_iter([
        'поехали до "проспекта Маршала Жукова 3"(location_to)',
        'машину от "тверской улицы"(location_from)',
    ]).items))

    tagger.train(train_data)
    return tagger


@pytest.fixture(scope='module')
def dummy_taxi_alarm_data():
    return {
        'taxi': FuzzyNLUFormat.parse_iter([
            'поехали до "проспекта Маршала Жукова 3"(location_to)',
            'машину от "тверской улицы"(location_from)',
        ]).items,
        'alarm': FuzzyNLUFormat.parse_iter([
            'будильник на "8 утра"(when)',
            'разбуди меня',
        ]).items
    }


def test_add(dummy_arch):
    dummy_arch.add('test', '/path/to/file')
    dummy_arch._tar.add.assert_called_once_with(
        '/path/to/file',
        'test',
    )


def test_add_nested(dummy_arch):
    with dummy_arch.nested('subdir') as arch:
        arch.add('test', '/path/to/file')

    dummy_arch._tar.add.assert_called_once_with(
        '/path/to/file',
        'subdir/test',
    )


def test_get_by_name(dummy_arch):
    dummy_arch.get_by_name('test')
    dummy_arch._tar.extractfile.assert_called_once_with(
        'test'
    )


def test_get_by_name_nested(dummy_arch):
    with dummy_arch.nested('subdir') as arch:
        arch.get_by_name('test')

    dummy_arch._tar.extractfile.assert_called_once_with(
        'subdir/test'
    )


def test_save_by_name(dummy_arch):
    dummy_arch.save_by_name('test', 'to/file')
    dummy_arch._tar.extract.assert_called_once_with(
        'test', 'to/file'
    )


def test_save_by_name_nested(dummy_arch):
    with dummy_arch.nested('subdir') as arch:
        arch.save_by_name('test', 'to/file')

    dummy_arch._tar.extract.assert_called_once_with(
        'subdir/test', 'to/file'
    )


def test_list(dummy_arch):
    dummy_arch.add('test', '')
    dummy_arch.add('foo', '')
    dummy_arch.add('bar', '')

    with dummy_arch.nested('subdir') as arch:
        arch.add('inner', '')

    assert sorted(dummy_arch.list()) == ['bar', 'foo', 'subdir', 'test']


def test_nested_not_closed(dummy_arch):
    arch = dummy_arch.nested('qwe')
    arch.close()
    dummy_arch._tar.close.assert_not_called()


def test_tmp_files_cleared(mocker, dummy_arch):
    mocker.patch('tempfile.mkstemp', side_effect=[(1, 'file1'), (2, 'file2')])
    mocker.patch('os.close')
    mocker.patch('tempfile.mkdtemp', return_value='dir1/')

    remove = mocker.patch('vins_core.utils.data.os.remove')

    file1 = dummy_arch.get_tmp_file()

    with dummy_arch.nested('foo') as arch:
        file2 = arch.get_tmp_file()
        dir1 = dummy_arch.get_tmp_dir()

    # check that file still not removed after nested close
    remove.assert_not_called()

    dummy_arch.close()

    assert remove.call_args_list == [
        mock.call(file1),
        mock.call(file2),
        mock.call(dir1),
    ]


def test_add_files(dummy_arch):
    files = ['foo/bar.py', 'foo/bar/baz.py', 'testfile.txt']
    dummy_arch.add_files(files)
    assert dummy_arch._data == dict(zip(['bar.py', 'baz.py', 'testfile.txt'], files))


def test_object_save(mocker, dummy_arch, intent_or_tagger):
    mocker.patch('tempfile.mkstemp', return_value=(1, 'filename'))
    mocker.patch('os.remove')
    mocker.patch('os.close')

    mopen = mocker.mock_open()
    mocker.patch('__builtin__.open', mopen)

    with dummy_arch as arch:
        intent_or_tagger.save(arch, 'test')

    open.assert_called_once_with('filename', 'wb')
    assert dummy_arch._data == {'test': 'filename'}


def test_classifier_save_load(classifier, tar_archive, feature_extractor):
    f = feature_extractor
    with tar_archive as arch:
        classifier.save(arch, 'test-classifier')

    new_cls = create_token_classifier(model='maxent')
    with TarArchive(tar_archive.path) as arch:
        new_cls.load(arch, 'test-classifier')

    def predict(cls):
        return list(cls.predict(f([Sample.from_string('ad', tokens=['a', 'd'])]))[0])

    assert predict(new_cls) == predict(classifier)


def test_combine_scores_classifier_save_load(combine_scores_classifier, combine_scores_classifier_config,
                                             tar_archive, feature_extractor):
    f = feature_extractor
    with tar_archive as arch:
        combine_scores_classifier.save(arch, 'combine_scores')

    new_cls = create_token_classifier(**combine_scores_classifier_config)
    with TarArchive(tar_archive.path) as arch:
        new_cls.load(arch, 'combine_scores')

    def predict(cls):
        return cls(f([Sample.from_string('ad', tokens=['a', 'd'])])[0])

    assert predict(new_cls) == predict(combine_scores_classifier)


def test_tagger_save_load(tagger, tar_archive, feature_extractor):
    f = feature_extractor
    with tar_archive as arch:
        tagger.save(arch, 'test-tagger')

    new = create_token_tagger('crf')
    with TarArchive(tar_archive.path) as arch:
        new.load(arch, 'test-tagger')

    def predict(mdl):
        utt = 'до улицы Льва Толстого'
        x = Sample(utt.split())
        y, _ = mdl.predict(f([x]))
        return list(y[0])

    assert predict(new) == predict(tagger)


def test_combine_scores_tagger_save_load(combine_scores_tagger, combine_scores_tagger_config,
                                         tar_archive, feature_extractor_prod):
    f = feature_extractor_prod
    with tar_archive as arch:
        combine_scores_tagger.save(arch, 'tagger1')
        combine_scores_tagger.save(arch, 'tagger2')

    new = create_token_tagger('combine_scores', **combine_scores_tagger_config)
    with TarArchive(tar_archive.path) as arch:
        new.load(arch, 'tagger1')
        new.load(arch, 'tagger2')

    def predict(mdl):
        utt = 'до улицы Льва Толстого'
        x = Sample(utt.split())
        y, _ = mdl.predict(f([x]))
        return list(y[0])

    assert predict(new) == predict(combine_scores_tagger)


@attr.s
class DummIntentInfos(object):
    utterance_tagger = attr.ib(default=None)


@pytest.fixture(scope='module', params=[
    {'utterance_tagger': {'model': 'crf', 'features': 'ner'}},
    {'utterance_tagger': {'model': 'rnn_new'}},
])
def dummy_intent_infos_mixed_tagger(request):

    return {
        'taxi': DummIntentInfos(),
        'alarm': DummIntentInfos(**request.param)
    }


@pytest.fixture(scope='module')
def mixed_tagger(
    feature_extractor_prod, dummy_taxi_alarm_data, samples_extractor_with_wizard, dummy_intent_infos_mixed_tagger
):

    def f(items):
        return feature_extractor_prod(samples_extractor_with_wizard(items))
    data = {intent: f(items) for intent, items in dummy_taxi_alarm_data.iteritems()}
    tagger = create_token_tagger('mixed', default_config=dict(
        model='crf',
        features=['word']
    ), intent_infos=dummy_intent_infos_mixed_tagger)
    tagger.train(data)
    return tagger


def test_mixed_tagger_save_load(mixed_tagger, feature_extractor_prod, tar_archive, dummy_intent_infos_mixed_tagger):

    f = feature_extractor_prod
    with tar_archive as arch:
        mixed_tagger.save(arch, 'test-tagger')

    new = create_token_tagger('mixed', default_config=dict(
        model='crf',
        features=['word', 'ner']
    ), intent_infos=dummy_intent_infos_mixed_tagger)
    with TarArchive(tar_archive.path) as arch:
        new.load(arch, 'test-tagger')

    def predict(mdl):
        utt = 'до улицы Льва Толстого'
        x = Sample(utt.split())
        y, _ = mdl.predict(f([x]), intent='alarm')
        return list(y[0])

    assert predict(new) == predict(mixed_tagger)


@pytest.fixture(scope='module', params=(
    {'restore_weights_mode': 'last', 'model_archive': 'archive_last.tar.gz', 'logdir': 'last'},
))
def knn_with_metric_learning(tmpdir_factory, request, nlu_demo_data,
                             feature_extractor_embeddings, normalizing_samples_extractor):
    f = feature_extractor_embeddings
    model_dir = str(tmpdir_factory.mktemp('metric_learning'))
    model_archive = str(tmpdir_factory.mktemp('model').join(request.param['model_archive']))
    logdir = str(tmpdir_factory.mktemp('logs').join(request.param['logdir']))
    os.makedirs(logdir)
    model_name = 'test'

    norm = normalizing_samples_extractor
    train_data = {
        intent: f(norm(FuzzyNLUFormat.parse_iter(texts).items))
        for intent, texts in nlu_demo_data.iteritems()
    }

    params = dict(
        convert_to_prob=False, train_split=0.95, num_epochs=100,
        metric_function='metric_learning', metric_learning=TrainMode.METRIC_LEARNING_FROM_SCRATCH,
        restore_weights_mode=request.param['restore_weights_mode'],
        model_dir=model_dir, metric_learning_logdir=logdir, model_name=model_name,
        num_neighbors=1,
    )
    clf = metric_learning_with_index(train_data, params)

    assert os.path.exists(os.path.join(logdir, model_name))
    clf_name = 'knn-metric-learning'
    with TarArchive(model_archive, mode=b'w:gz') as arch:
        clf.save(arch, clf_name)
    return clf, deepcopy(params), clf_name, model_archive


@pytest.mark.slowtest
@pytest.mark.parametrize('metric_learning', (TrainMode.METRIC_LEARNING_FROM_SCRATCH, TrainMode.NO_METRIC_LEARNING))
def test_knn_metric_learning_save_load(knn_with_metric_learning, metric_learning, feature_extractor_embeddings):
    old_clf, params, clf_name, model_archive = knn_with_metric_learning
    params['metric_learning'] = metric_learning
    new_clf = create_token_classifier('knn', **params)
    with TarArchive(model_archive) as arch:
        new_clf.load(arch, clf_name)

    def predict(clf):
        return clf(feature_extractor_embeddings([Sample.from_string('будильник на 8:30')])[0])
    pred_old_clf = predict(old_clf)
    pred_new_clf = predict(new_clf)
    for intent in pred_old_clf:
        assert is_close(pred_old_clf[intent], pred_new_clf[intent], 1e-6)
    assert is_close(pred_old_clf['alarm'], 1, 1e-6)
    assert is_close(pred_new_clf['alarm'], 1, 1e-6)


@pytest.fixture(scope='module')
def rnn_tagger_new_data(feature_extractor_prod, samples_extractor_with_wizard, dummy_taxi_alarm_data):
    def f(data):
        return feature_extractor_prod(samples_extractor_with_wizard(data))
    return {intent: f(items) for intent, items in dummy_taxi_alarm_data.iteritems()}


@pytest.fixture(scope='module', params=(True, False))
def rnn_tagger_new(request, rnn_tagger_new_data):
    tagger = create_token_tagger('rnn_new', intent_conditioned=request.param)
    tagger.train(rnn_tagger_new_data)
    return tagger


def _save_tagger(archive, tagger):
    with archive as root_arch:
        with root_arch.nested('taggers') as arch:
            tagger.save(arch, 'test-tagger')


def _load_tagger(archive, tagger_type):
    tagger = create_token_tagger(tagger_type)
    with archive as root_arch:
        with root_arch.nested('taggers') as arch:
            tagger.load(arch, 'test-tagger')
    return tagger


@pytest.mark.parametrize('asynchronous', (True, False))
def test_rnntaggernew_save_load(rnn_tagger_new, tar_archive, feature_extractor_prod, asynchronous, mocker, tmpdir):
    mocker.patch('vins_core.utils.data.vins_temp_dir', lambda: tmpdir.strpath)
    f = feature_extractor_prod

    _save_tagger(tar_archive, rnn_tagger_new)
    new_tagger = _load_tagger(TarArchive(tar_archive.path), tagger_type='rnn_new')

    def predict(mdl):
        utt = 'до улицы Льва Толстого'
        x = Sample(utt.split())
        y, p = mdl.predict(f([x]), intent='taxi')
        return list(y), list(p)

    if asynchronous:
        pool = ThreadPool(processes=1)
        tags, preds = pool.apply_async(predict, args=(new_tagger,)).get()
    else:
        tags, preds = predict(new_tagger)

    assert new_tagger._intent_conditioned == rnn_tagger_new._intent_conditioned
    assert tags, preds == predict(rnn_tagger_new)


def test_rnntaggernew_save_load_and_retrain(
    rnn_tagger_new, tar_archive, rnn_tagger_new_data, feature_extractor_prod, mocker, tmpdir
):
    mocker.patch('vins_core.utils.data.vins_temp_dir', lambda: tmpdir.strpath)
    f = feature_extractor_prod

    _save_tagger(tar_archive, rnn_tagger_new)
    old_tagger = _load_tagger(TarArchive(tar_archive.path), tagger_type='rnn_new')

    old_intent, retrained_intent = 'alarm', 'taxi'
    new_tagger = _load_tagger(TarArchive(tar_archive.path), tagger_type='rnn_new')
    new_tagger.train(rnn_tagger_new_data, intents_to_train=retrained_intent)

    def predict(mdl, intent):
        utt = 'до улицы Льва Толстого'
        x = Sample(utt.split())
        y, p = mdl.predict(f([x]), intent=intent)
        return list(y), list(p)

    assert old_tagger._intent_conditioned == new_tagger._intent_conditioned

    if old_tagger._intent_conditioned:
        assert predict(old_tagger, old_intent) == predict(new_tagger, old_intent)

        # Just checking, that tagger can predict
        predict(new_tagger, intent=retrained_intent)
    else:
        # Just checking, that tagger can predict
        predict(new_tagger, intent=old_intent)


def test_save_load_app(simple_app, tar_archive):
    client = simple_app._vins_app

    with tar_archive as arch:
        client._dm.nlu.save(arch)

    # create new app
    app_cfg = deepcopy(client._dm.app_cfg)
    app_cfg.nlu.update({
        'compiled_model': {
            'path': tar_archive.path,
            'archive': 'TarArchive',
        }
    })
    dm = DialogManager.from_config(app_cfg)
    dm.nlu.set_entity_parsers(client._dm.nlu.get_custom_entity_parsers())

    new_app = TestConnector(vins_app=VinsApp(dm=dm))

    def handle(app, utt):
        return app.handle_utterance(gen_uuid(), utt)

    utt1 = 'что такое яблоко'
    utt2 = 'а какого цвета'

    assert handle(simple_app, utt1) == handle(new_app, utt1)
    assert handle(simple_app, utt2) == handle(new_app, utt2)


@pytest.mark.slowtest
@pytest.mark.parametrize('load_tf_on_call', [True, False])
def test_save_load_app_update_classifiers(monkeypatch, simple_app_with_cascade, tar_archive, load_tf_on_call):

    def handle(app, utt):
        return app.handle_utterance(gen_uuid(), utt)

    def create_new_app():
        app_cfg = deepcopy(client._dm.app_cfg)
        app_cfg.nlu.update({
            'compiled_model': {
                'path': tar_archive.path,
                'archive': 'TarArchive',
            }
        })
        return DialogManager.from_config(app_cfg, force_rebuild=True, classifiers=['test'])

    utt1 = 'какого цвета яблоко'

    monkeypatch.setenv('VINS_LOAD_TF_ON_CALL', load_tf_on_call)
    client = simple_app_with_cascade._vins_app

    handle(simple_app_with_cascade, utt1)

    with tar_archive as arch:
        client._dm.nlu.save(arch)

    dm = create_new_app()
    with TarArchive(tar_archive.path, mode=b'w:gz') as arch:
        dm.nlu.save(arch)

    dm = create_new_app()
    dm.nlu.set_entity_parsers(client._dm.nlu.get_custom_entity_parsers())

    new_app = TestConnector(vins_app=VinsApp(dm=dm))

    assert handle(simple_app_with_cascade, utt1) == handle(new_app, utt1)


def test_compile_app_vinsfile(mocker, dummy_arch):
    fv_mock = mocker.patch(
        'vins_core.app_utils.find_vinsfile', autospec=True,
        return_value='testpath',
    )

    app_cfg_mock = mocker.patch(
        'vins_core.config.app_config.AppConfig', autospec=True
    )
    dm_mock = mocker.patch('vins_core.dm.form_filler.dialog_manager.DialogManager', autospec=True)

    with dummy_arch as arch:
        compile_app('test-app', arch)

    fv_mock.assert_called_once_with('test-app')
    app_cfg_mock().parse_vinsfile.assert_called_once_with('testpath')
    dm_mock.from_config.assert_called_once_with(app_cfg_mock(), load_data=True, load_model=True)
    dm_mock.from_config(app_cfg_mock()).nlu.save.assert_called_once_with(dummy_arch)


@pytest.fixture(scope='function')
def test_entities_update_arch():
    resource = get_resource_full_path('resource://custom_enitities/custom_enitities/test_entities_update.tar.gz')
    arch = TarArchive(resource)
    yield arch
    arch.close()


@pytest.mark.slowtest
def test_update_custom_entity(function_scoped_simple_app, test_entities_update_arch):
    dm = function_scoped_simple_app._vins_app._dm

    sample1 = Sample.from_string('включи test')
    sample2 = Sample.from_string('включи ololo-trololo')

    session = Session(app_id='test_app', uuid='123')

    def handle(sample):
        return dm.nlu.handle(sample, session).semantic_frames[0]['slots']['what'][0]['entities']

    # with old entity we can handle only first sample
    assert handle(sample1)[0].type == 'TEST_ENTITY'
    assert handle(sample2) == []

    dm.update_custom_entities(test_entities_update_arch)

    # after update we can handle both
    assert handle(sample1)[0].type == 'TEST_ENTITY'
    assert handle(sample2)[0].type == 'TEST_ENTITY'
