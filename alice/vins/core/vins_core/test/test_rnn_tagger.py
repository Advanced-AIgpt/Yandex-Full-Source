# coding: utf-8
from __future__ import unicode_literals

import pytest

from multiprocessing.pool import ThreadPool
from multiprocessing import Manager, Process

from vins_core.nlu.token_tagger import RNNTokenTagger
from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.nlu.features_extractor import FeaturesExtractorFactory
from vins_core.utils.data import TarArchive


@pytest.fixture(scope='module')
def feature_extractor_prod(dummy_embeddings, parser):
    factory = FeaturesExtractorFactory()
    factory.register_parser(parser)
    factory.add('postag', 'postag')
    factory.add('ner', 'ner')
    factory.add('wizard', 'wizard')
    factory.add('alice_requests_emb', 'embeddings', resource='resource://req_embeddings')
    return factory.create_extractor()


@pytest.fixture(scope='function')
def tar_archive(mocker, tmpdir):
    tar_file = tmpdir.join('archive.tar.gz').strpath
    mocker.patch('vins_core.utils.data.vins_temp_dir', lambda: tmpdir.strpath)
    arch = TarArchive(tar_file, mode=b'w:gz')
    yield arch


@pytest.mark.slowtest
@pytest.mark.parametrize('asynchronous', (True, False))
def test_rnn_tagger_new(monkeypatch, tar_archive, samples_extractor_with_wizard, feature_extractor_prod, asynchronous):
    monkeypatch.setenv('VINS_LOAD_TF_ON_CALL', True)

    f = feature_extractor_prod
    train_data = [
        'из пункта "1"(what_from) на "улице 1"(where_from) до пункта "1"(what_to) на "улице 1"(where_to)',
        'из пункта "2"(what_from) на "улице 1"(where_from) до пункта "1"(what_to) на "улице 1"(where_to)',
        'из пункта "1"(what_from) на "улице 2"(where_from) до пункта "1"(what_to) на "улице 1"(where_to)',
        'из пункта "1"(what_from) на "улице 1"(where_from) до пункта "2"(what_to) на "улице 1"(where_to)',
        'из пункта "1"(what_from) на "улице 1"(where_from) до пункта "1"(what_to) на "улице 2"(where_to)',
        'из пункта "2"(what_from) на "улице 2"(where_from) до пункта "1"(what_to) на "улице 1"(where_to)',
        'из пункта "2"(what_from) на "улице 1"(where_from) до пункта "2"(what_to) на "улице 1"(where_to)',
        'из пункта "2"(what_from) на "улице 1"(where_from) до пункта "1"(what_to) на "улице 2"(where_to)',
        'из пункта "1"(what_from) на "улице 2"(where_from) до пункта "2"(what_to) на "улице 1"(where_to)',
        'из пункта "1"(what_from) на "улице 2"(where_from) до пункта "1"(what_to) на "улице 2"(where_to)',
        'из пункта "1"(what_from) на "улице 1"(where_from) до пункта "2"(what_to) на "улице 2"(where_to)',
        'из пункта "2"(what_from) на "улице 2"(where_from) до пункта "2"(what_to) на "улице 1"(where_to)',
        'из пункта "2"(what_from) на "улице 1"(where_from) до пункта "2"(what_to) на "улице 2"(where_to)',
        'из пункта "2"(what_from) на "улице 2"(where_from) до пункта "1"(what_to) на "улице 2"(where_to)',
        'из пункта "2"(what_from) на "улице 2"(where_from) до пункта "1"(what_to) на "улице 2"(where_to)',
    ]

    train_features = f(samples_extractor_with_wizard(FuzzyNLUFormat.parse_iter(train_data).items))
    test_data = f(samples_extractor_with_wizard(['из пункта 2 на улице 2 до пункта 2 на улице 2']))

    tagger_builder = RNNTokenTagger()
    with tar_archive as root_arch:
        with root_arch.nested('taggers') as arch:
            tagger_builder.train(train_features)
            tagger_builder.save(arch, 'test-tagger')

    tagger_applier = RNNTokenTagger()
    with TarArchive(tar_archive.path) as root_arch:
        with root_arch.nested('taggers') as arch:
            tagger_applier.load(arch, 'test-tagger')

    def get_result():
        return tagger_applier.predict(test_data)

    if asynchronous:
        pool = ThreadPool(processes=1)
        tags_nbest, probs_nbest = pool.apply_async(get_result).get()
    else:
        tags_nbest, probs_nbest = get_result()

    assert tags_nbest[0][0] == [
        'O', 'O', 'B-what_from', 'O', 'B-where_from', 'I-where_from',
        'O', 'O', 'B-what_to', 'O', 'B-where_to', 'I-where_to'
    ]


@pytest.mark.skip
@pytest.mark.parametrize('load_on_call, num_procs', [
    (False, 1),
    (False, 4),
    (True, 1),
    (True, 4),
])
def test_rnn_tagger_new_multiprocess(monkeypatch, tar_archive, samples_extractor_with_wizard,
                                     feature_extractor_prod, load_on_call, num_procs):
    monkeypatch.setenv('VINS_LOAD_TF_ON_CALL', load_on_call)

    f = feature_extractor_prod
    train_data = [
        'такси до "москвы"(to) на "8 утра"(when)',
        'такси из "москвы"(from) на "8 утра"(when)'
    ]

    train_features = f(samples_extractor_with_wizard(FuzzyNLUFormat.parse_iter(train_data).items))
    test_data = f(samples_extractor_with_wizard(['такси в одинцово на 8 утра', 'такси из москвы на 8 утра']))

    tagger_builder = RNNTokenTagger()
    with tar_archive as root_arch:
        with root_arch.nested('taggers') as arch:
            tagger_builder.train(train_features)
            tagger_builder.save(arch, 'test-tagger')

    tagger_applier = RNNTokenTagger()
    with TarArchive(tar_archive.path) as root_arch:
        with root_arch.nested('taggers') as arch:
            tagger_applier.load(arch, 'test-tagger')

    def tagger_runner(proc_id, statuses):
        statuses[proc_id] = statuses[proc_id] + ['started']
        statuses[proc_id] = statuses[proc_id] + ['predicting']

        for i in range(100):
            tagger_applier.predict(test_data)

        statuses[proc_id] = statuses[proc_id] + ['predicted']
        statuses[proc_id] = statuses[proc_id] + ['finished']

    manager = Manager()
    statuses = manager.list()
    for i in xrange(num_procs):
        statuses.append(['not started'])

    procs = [Process(target=tagger_runner, args=(i, statuses)) for i in xrange(num_procs)]
    for proc in procs:
        proc.start()
    for proc in procs:
        proc.join()

    for i in xrange(num_procs):
        assert statuses[i] == [
            'not started',
            'started',
            'predicting',
            'predicted',
            'finished'
        ]
