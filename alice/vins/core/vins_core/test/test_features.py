# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import json
import time
import scipy.sparse
import mock
import numpy as np
import pytest
import copy
import requests_mock

import vins_core

from itertools import izip
from collections import Counter

from vins_core.common.sample import Sample

from vins_core.common.annotations import WizardAnnotation
from vins_core.dm.formats import NluSourceItem, FuzzyNLUFormat
from vins_core.dm.response import VinsResponse, FeaturesExtractorErrorMeta
from vins_core.ner.fst_presets import PARSER_RU_BASE_PARSERS
from vins_core.nlu.features.base import SampleFeatures, IntentScore, TaggerScore, TaggerSlot
from vins_core.nlu.features.extractor.base import SparseFeatureValue
from vins_core.nlu.features.extractor.ngram import NGramFeatureExtractor
from vins_core.nlu.features.extractor.serp import SerpFeatureExtractor
from vins_core.nlu.features.extractor.embeddings import EmbeddingsFeaturesExtractor
from vins_core.nlu.features.extractor.ner import NerFeatureExtractor
from vins_core.nlu.features.extractor.music import MusicFeaturesExtractor
from vins_core.nlu.features.post_processor.splicer import SplicerFeaturesPostProcessor
from vins_core.nlu.features.post_processor.vectorizer import VectorizerFeaturesPostProcessor
from vins_core.nlu.features_extractor import FeaturesExtractorFactory, create_features_extractor
from vins_core.nlu.features.post_processor.selector import SelectorFeaturesPostProcessor
from vins_core.utils.misc import is_close
from vins_core.nlu.base_nlu import FeatureExtractorFromItem
from vins_core.nlu.features.cache.picklecache import PickleCache, SampleFeaturesCache
from vins_core.nlu.features.cache.protobufcache import YtProtobufFeatureCache
from vins_core.ext.wizard_api import WizardHTTPAPI

BOS_TAG = NGramFeatureExtractor.BOS_TAG
EOS_TAG = NGramFeatureExtractor.EOS_TAG


@pytest.fixture(scope='module')
def utts():
    res = [
        '"улица Льва Толстого 16"(where)',
        'будильник на "7 утра"(when)',
        'погода в "москве"(where) "25-го мая"(when)',
        'я передумал подайте машину на "тверскую 1"(location_to)',
    ]
    return res


@pytest.fixture(scope='module')
def utts_nlu_source_items(utts):
    nlu_source_items = FuzzyNLUFormat.parse_iter(utts).items
    return utts, nlu_source_items


@pytest.fixture(scope='module')
def utts_samples(samples_extractor, utts_nlu_source_items):
    utts, nlu_source_items = utts_nlu_source_items
    return utts, samples_extractor(nlu_source_items)


@pytest.fixture(scope='module')
def custom_ner_currency_parser(parser_factory, custom_ner_currency):
    return parser_factory.create_parser(PARSER_RU_BASE_PARSERS, additional_parsers=custom_ner_currency.values())


@pytest.fixture(scope='module')
def only_custom_ner_currency_parser(parser_factory, custom_ner_currency):
    return parser_factory.create_parser([], additional_parsers=custom_ner_currency.values())


@pytest.fixture(scope='module')
def features_extractor(dummy_embeddings, custom_ner_currency_parser):
    factory = FeaturesExtractorFactory()
    factory.register_parser(custom_ner_currency_parser)
    features_cfg = [
        {'type': 'ngrams', 'id': 'word', 'params': {'n': 1}},
        {'type': 'ngrams', 'id': 'bigram', 'params': {'n': 2}},
        {'type': 'ner', 'id': 'ner'},
        {'type': 'postag', 'id': 'postag'},
        {'type': 'lemma', 'id': 'lemma'},
        {'type': 'embeddings', 'id': 'embeddings', 'params': {'file': dummy_embeddings}},
    ]
    for cfg in features_cfg:
        factory.add(cfg['id'], cfg['type'], **cfg.get('params', {}))

    return factory.create_extractor()


def _wrap_sparse_list(strings):
    return [SparseFeatureValue(s) for s in strings]


def _wrap_sparse_lol(list_of_lists, sets_mode=True):
    if sets_mode:
        return [set(_wrap_sparse_list(strings)) for strings in list_of_lists]
    else:
        return [_wrap_sparse_list(strings) for strings in list_of_lists]


def convert_to_sets(list_of_lists):
    return list(map(lambda l: set(l), list_of_lists))


def test_features_extractor_general(features_extractor, samples_extractor):
    utt = 'улица Льва Толстого 16'
    sample = samples_extractor([NluSourceItem(utt)])
    features = features_extractor(sample)[0]
    assert features.sparse_seq
    assert convert_to_sets(features.sparse_seq['word']) == _wrap_sparse_lol([
        ['улица'], ['льва'], ['толстого'], ['16']
    ])
    assert convert_to_sets(features.sparse_seq['bigram']) == _wrap_sparse_lol([
        ['%s улица' % BOS_TAG], ['улица льва'], ['льва толстого'], ['толстого 16 %s' % EOS_TAG]
    ])
    assert convert_to_sets(features.sparse_seq['postag']) == _wrap_sparse_lol([
        ['NOUN'], ['NOUN'], ['NOUN'], ['UNKN']
    ])
    assert convert_to_sets(features.sparse_seq['lemma']) == _wrap_sparse_lol([
        ['улица'], ['лев'], ['толстой'], ['16']
    ])

    assert features.dense_seq
    assert np.array_equal(features.dense_seq['embeddings'], [
        [1, 2, 3],
        [-1, -2, -3],
        [3, 6, 9],
        [0, 0, 0]
    ])


@pytest.mark.xfail(reason="DIALOG-2533")
def test_features_extractor_general_ner(features_extractor, samples_extractor):
    utt = 'улица Льва Толстого 16'
    sample = samples_extractor([NluSourceItem(utt)])
    features = features_extractor(sample)[0]
    assert features.sparse_seq
    assert convert_to_sets(features.sparse_seq['ner']) == _wrap_sparse_lol([
        ['B-GEO'], ['I-GEO'], ['I-GEO', 'B-FIO'], ['B-UNITS_TIME', 'I-GEO', 'B-NUM', 'B-TIME']
    ])


def test_features_extractor_with_num(features_extractor, samples_extractor):
    utt = '1 доллар это 65 рублей'
    sample = samples_extractor([NluSourceItem(utt)])
    features = features_extractor(sample)[0]

    assert features.sparse_seq
    word = convert_to_sets(features.sparse_seq['word'])
    assert word == _wrap_sparse_lol([['1'], ['доллар'], ['это'], ['65'], ['рублей']])
    bigram = convert_to_sets(features.sparse_seq['bigram'])
    assert bigram == _wrap_sparse_lol(
        [['%s 1' % BOS_TAG], ['1 доллар'], ['доллар это'], ['это 65'], ['65 рублей %s' % EOS_TAG]]
    )
    postag = convert_to_sets(features.sparse_seq['postag'])
    assert postag == _wrap_sparse_lol([['UNKN'], ['NOUN'], ['NPRO'], ['UNKN'], ['NOUN']])
    lemma = convert_to_sets(features.sparse_seq['lemma'])
    assert lemma == _wrap_sparse_lol([['1'], ['доллар'], ['это'], ['65'], ['рубль']])
    ner = convert_to_sets(features.sparse_seq['ner'])

    assert ner == _wrap_sparse_lol([
        ['B-UNITS_TIME', 'B-SITE', 'B-NUM', 'B-TIME'], ['B-CUSTOM_CURRENCY', 'B-CURRENCY'], [],
        ['B-UNITS_TIME', 'B-NUM'], ['B-CUSTOM_CURRENCY', 'B-CURRENCY']
    ])


def test_features_extractor_custom_entities(samples_extractor, only_custom_ner_currency_parser):
    factory = FeaturesExtractorFactory()
    factory.register_parser(only_custom_ner_currency_parser)
    factory.add('ner', 'ner')
    fex = factory.create_extractor()

    utt = '1 доллар это 65 рублей'
    sample = samples_extractor([NluSourceItem(utt)])
    features = fex(sample)[0]
    ner = convert_to_sets(features.sparse_seq['ner'])
    assert ner == _wrap_sparse_lol([[], ['B-CUSTOM_CURRENCY'], [], [], ['B-CUSTOM_CURRENCY']])


def test_features_post_processor_splicer(features_extractor, utts_samples):
    utts, samples = utts_samples
    splicer = SplicerFeaturesPostProcessor(window_size=3)
    features = splicer(features_extractor(samples))[1]
    word = features.sparse_seq['word']
    assert len(word) == len(samples[1])
    assert word[0] == _wrap_sparse_list(['будильник(t=0)', 'на(t=1)'], )
    assert word[1] == _wrap_sparse_list(['будильник(t=-1)', 'на(t=0)', '7(t=1)'])
    assert word[2] == _wrap_sparse_list(['на(t=-1)', '7(t=0)', 'утра(t=1)'])
    assert word[3] == _wrap_sparse_list(['7(t=-1)', 'утра(t=0)'])
    ner = convert_to_sets(features.sparse_seq['ner'])

    assert ner == _wrap_sparse_lol(map(sorted, [
        {},
        {'B-DATETIME(t=1)', 'B-NUM(t=1)', 'B-TIME(t=1)', 'B-UNITS_TIME(t=1)'},
        {'B-DATETIME(t=0)', 'B-NUM(t=0)', 'I-DATETIME(t=1)', 'B-TIME(t=0)', 'I-TIME(t=1)', 'B-UNITS_TIME(t=0)'},
        {'B-DATETIME(t=-1)', 'B-NUM(t=-1)', 'I-DATETIME(t=0)', 'B-TIME(t=-1)', 'I-TIME(t=0)', 'B-UNITS_TIME(t=-1)'}
    ]))


def _wizard_mocked(answer):
    m = requests_mock.mock()
    content = json.dumps(answer, ensure_ascii=False).encode('utf-8')
    m.get(WizardHTTPAPI.WIZARD_URL, content=content)
    return m


class TestWizard(object):
    def test_wizard_GeoAddr(self, utts_nlu_source_items, samples_extractor_with_wizard):
        utts, nlu_src_items = utts_nlu_source_items

        with _wizard_mocked(
            answer={
                'markup': {
                    'Tokens': [
                        {'EndChar': 5, 'Text': 'улица', 'EndByte': 10, 'BeginByte': 0, 'BeginChar': 0},
                        {'EndChar': 10, 'Text': 'льва', 'EndByte': 19, 'BeginByte': 11, 'BeginChar': 6},
                        {'EndChar': 19, 'Text': 'толстого', 'EndByte': 36, 'BeginByte': 20, 'BeginChar': 11},
                        {'EndChar': 22, 'Text': '16', 'EndByte': 39, 'BeginByte': 37, 'BeginChar': 20}
                    ],
                    'Delimiters': [{}, {'Text': ' '}, {'Text': ' '}, {'Text': ' '}, {}],
                    'GeoAddr': [{
                        'Fields': [{
                            'Tokens': {'Begin': 0, 'End': 3},
                            'Type': 'Street',
                            'Name': 'улица льва толстого'
                        }, {
                            'Tokens': {'Begin': 3, 'End': 4},
                            'Type': 'HouseNumber',
                            'Name': '16'
                        }]
                    }]
                },
                'rules': {
                    'IsNav': {'RuleResult': '3'},
                }
            }
        ):
            samples = samples_extractor_with_wizard([nlu_src_items[0]])
            extractor = create_features_extractor(
                wizard=dict(rules=('GeoAddr', 'IsNav'))
            )
            features = extractor(samples)[0].sparse_seq['wizard']

            assert features[0] == _wrap_sparse_list(['B-GeoAddr_Street', 'B-IsNav'])
            assert features[1] == _wrap_sparse_list(['I-GeoAddr_Street', 'I-IsNav'])
            assert features[2] == _wrap_sparse_list(['I-GeoAddr_Street', 'I-IsNav'])
            assert features[3] == _wrap_sparse_list(['B-GeoAddr_HouseNumber', 'I-IsNav'])

    def test_wizard_GeoAddr_misaligned_token(self, samples_extractor_with_wizard):
        with _wizard_mocked(
            answer={
                'markup': {
                    'GeoAddr': [{
                        'Tokens': {'Begin': 1, 'End': 3},
                        'Fields': [{
                            'Tokens': {'Begin': 2, 'End': 3},
                            'Type': 'City',
                            'Name': 'москва',
                            'Id': [213, 104116]
                        }]
                    }],
                    'Date': [{'Tokens': {'Begin': 3, 'End': 6}, 'Day': 25, 'Month': 5}],
                    'Tokens': [
                        {'EndChar': 6, 'Text': 'погода', 'EndByte': 12, 'BeginByte': 0, 'BeginChar': 0},
                        {'EndChar': 8, 'Text': 'в', 'EndByte': 15, 'BeginByte': 13, 'BeginChar': 7},
                        {'EndChar': 15, 'Text': 'москве', 'EndByte': 28, 'BeginByte': 16, 'BeginChar': 9},
                        {'EndChar': 18, 'Text': '25', 'EndByte': 31, 'BeginByte': 29, 'BeginChar': 16},
                        {'EndChar': 21, 'Text': 'го', 'EndByte': 36, 'BeginByte': 32, 'BeginChar': 19},
                        {'EndChar': 25, 'Text': 'мая', 'EndByte': 43, 'BeginByte': 37, 'BeginChar': 22}
                    ],
                    'Delimiters': [
                        {},
                        {'EndChar': 7, 'Text': ' ', 'EndByte': 13, 'BeginByte': 12, 'BeginChar': 6},
                        {'EndChar': 9, 'Text': ' ', 'EndByte': 16, 'BeginByte': 15, 'BeginChar': 8},
                        {'EndChar': 16, 'Text': ' ', 'EndByte': 29, 'BeginByte': 28, 'BeginChar': 15},
                        {'EndChar': 19, 'Text': ' ', 'EndByte': 32, 'BeginByte': 31, 'BeginChar': 18},
                        {'EndChar': 22, 'Text': ' ', 'EndByte': 37, 'BeginByte': 36, 'BeginChar': 21},
                        {}
                    ]
                }
            }
        ):
            sample = samples_extractor_with_wizard(['погода в москве 25 го мая'])[0]
            extractor = create_features_extractor(
                wizard=dict(rules=('GeoAddr', 'Date'))
            )
            features = extractor([sample])[0].sparse_seq['wizard']
            assert features[0] == []
            assert features[1] == []
            assert features[2] == _wrap_sparse_list(['B-GeoAddr_City'])
            assert features[3] == _wrap_sparse_list(['B-Date'])
            assert features[4] == _wrap_sparse_list(['I-Date'])
            assert features[5] == _wrap_sparse_list(['I-Date'])

    def test_wizard_GeoAddr_unusual_delimiters(self, samples_extractor_with_wizard):
        sample = samples_extractor_with_wizard(['<censored>'])[0]
        extractor = create_features_extractor(
            wizard=dict(rules=('GeoAddr', 'IsNav'))
        )
        features = extractor([sample])[0].sparse_seq['wizard']
        assert all(len(f) == 0 for f in features)

    def test_wizard_EntityFinder(self, utts_nlu_source_items, samples_extractor_with_wizard):
        nlu_src_item = utts_nlu_source_items[1][2]

        with _wizard_mocked(
            answer={
                'markup': {
                    'GeoAddr': [{
                        'Tokens': {'Begin': 1, 'End': 3},
                        'Fields': [{
                            'Tokens': {'Begin': 2, 'End': 3},
                            'Type': 'City',
                            'Name': 'москва',
                            'Id': [213, 104116]
                        }]
                    }],
                    'Date': [{'Tokens': {'Begin': 3, 'End': 6}, 'Day': 25, 'Month': 5}],
                    'Tokens': [
                        {'EndChar': 6, 'Text': 'погода', 'EndByte': 12, 'BeginByte': 0, 'BeginChar': 0},
                        {'EndChar': 8, 'Text': 'в', 'EndByte': 15, 'BeginByte': 13, 'BeginChar': 7},
                        {'EndChar': 15, 'Text': 'москве', 'EndByte': 28, 'BeginByte': 16, 'BeginChar': 9},
                        {'EndChar': 18, 'Text': '25', 'EndByte': 31, 'BeginByte': 29, 'BeginChar': 16},
                        {'EndChar': 21, 'Text': 'го', 'EndByte': 36, 'BeginByte': 32, 'BeginChar': 19},
                        {'EndChar': 25, 'Text': 'мая', 'EndByte': 43, 'BeginByte': 37, 'BeginChar': 22}
                    ],
                    'Delimiters': [
                        {},
                        {'EndChar': 7, 'Text': ' ', 'EndByte': 13, 'BeginByte': 12, 'BeginChar': 6},
                        {'EndChar': 9, 'Text': ' ', 'EndByte': 16, 'BeginByte': 15, 'BeginChar': 8},
                        {'EndChar': 16, 'Text': ' ', 'EndByte': 29, 'BeginByte': 28, 'BeginChar': 15},
                        {'EndChar': 19, 'Text': '-', 'EndByte': 32, 'BeginByte': 31, 'BeginChar': 18},
                        {'EndChar': 22, 'Text': ' ', 'EndByte': 37, 'BeginByte': 36, 'BeginChar': 21},
                        {}
                    ]
                },
                'rules': {
                    'EntityFinder': {u'MainWinner': u'москве\t2\t3\truw71\t0.990\tgeo\tfb:book.book_subject|'
                                                    u'fb:location.administrative_division|fb:location.location|'
                                                    u'fb:location.citytown|fb:award.award_winner\t8',
                                     u'MainWinnerContentType': u'other',
                                     u'RuleResult': u'3',
                                     u'Winner': u'москве\t2\t3\truw71\t0.990\tgeo\tfb:book.book_subject|'
                                                u'fb:location.administrative_division|fb:location.location|'
                                                u'fb:location.citytown|fb:award.award_winner\t8',
                                     u'WinnerContentType': u'other'}
                }
            }
        ):
            samples = samples_extractor_with_wizard([nlu_src_item])
            extractor_onto = create_features_extractor(wizard=dict(
                rules=('EntityFinder',),
                use_onto=True,
                use_freebase=False
            ))
            extractor_fb = create_features_extractor(wizard=dict(
                rules=('EntityFinder',),
                use_onto=False,
                use_freebase=True
            ))
            extractor = create_features_extractor(wizard=dict(
                rules=('EntityFinder',),
                use_onto=True,
                use_freebase=True
            ))

            features_onto = extractor_onto(samples)[0].sparse_seq['wizard']
            features_fb = extractor_fb(samples)[0].sparse_seq['wizard']
            features = extractor(samples)[0].sparse_seq['wizard']

            assert features_fb[0] == features_onto[0] == features[0] == []
            assert features_fb[1] == features_onto[1] == features[1] == []

            assert features_onto[2] == _wrap_sparse_list(['B-EntityFinder_geo'])
            assert features_fb[2] == _wrap_sparse_list([
                'B-EntityFinder_fb:book.book_subject',
                'B-EntityFinder_fb:location.administrative_division',
                'B-EntityFinder_fb:location.location',
                'B-EntityFinder_fb:location.citytown',
                'B-EntityFinder_fb:award.award_winner'
            ])
            assert features[2] == features_onto[2] + features_fb[2]

            for i in xrange(3, len(features)):
                assert features_fb[i] == features_onto[i] == features[i] == []

    def test_wizard_Date_Fio(self, samples_extractor_with_wizard):
        with _wizard_mocked(
                answer={
                    'markup': {
                        'Date': [
                            {'Tokens': {'Begin': 2, 'End': 4}, 'Day': 25, 'Month': 10},
                            {'Tokens': {'Begin': 4, 'End': 5}, 'Day': 1, 'RelativeDay': True}
                        ],
                        'Fio': [{
                            'Tokens': {'Begin': 0, 'End': 2},
                            'LastName': 'ленин',
                            'Type': 'finame',
                            'FirstName': 'владимир'
                        }],
                        'Tokens': [
                            {'EndChar': 8, 'Text': 'владимир', 'EndByte': 16, 'BeginByte': 0, 'BeginChar': 0},
                            {'EndChar': 14, 'Text': 'ленин', 'EndByte': 27, 'BeginByte': 17, 'BeginChar': 9},
                            {'EndChar': 17, 'Text': '25', 'EndByte': 30, 'BeginByte': 28, 'BeginChar': 15},
                            {'EndChar': 25, 'Text': 'октября', 'EndByte': 45, 'BeginByte': 31, 'BeginChar': 18},
                            {'EndChar': 32, 'Text': 'завтра', 'EndByte': 58, 'BeginByte': 46, 'BeginChar': 26}
                        ],
                        'Delimiters': [{}, {'Text': ' '}, {'Text': ' '}, {'Text': ' '}, {'Text': ' '}, {}]
                    }
                }
        ):
            sample = samples_extractor_with_wizard(['владимир ленин 25 октября завтра'])[0]
            extractor = create_features_extractor(wizard=dict(
                rules=('Date', 'Fio')
            ))
            features = extractor([sample])[0].sparse_seq['wizard']
            assert features[0] == _wrap_sparse_list(['B-Fio_finame'])
            assert features[1] == _wrap_sparse_list(['I-Fio_finame'])
            assert features[2] == _wrap_sparse_list(['B-Date'])
            assert features[3] == _wrap_sparse_list(['I-Date'])
            assert features[4] == _wrap_sparse_list(['B-Date_RelativeDay'])


def test_granet_features_extractor(samples_extractor):
    form_name = 'personal_assistant.scenarios.repeat_after_me'
    granet_response = {
        'Granet': {
            'Tokens': [
                {'Text': 'повтори', 'Begin': 0, 'End': 14},
                {'Text': 'за', 'Begin': 15, 'End': 19},
                {'Text': 'мной', 'Begin': 20, 'End': 28},
                {'Text': 'привет', 'Begin': 29, 'End': 41}
            ],
            'Forms': [
                {
                    'LogProbability': -15.76832104,
                    'Name': form_name,
                    'Tags': [{'Begin': 3, 'End': 4, 'Name': 'request'}]
                }
            ],
            'Text': 'повтори за мной привет'
        }
    }

    sample = samples_extractor(['повтори за мной привет'])[0]
    sample.annotations['wizard'] = WizardAnnotation(markup={}, rules=granet_response)
    extractor = create_features_extractor(granet={})
    features = extractor([sample])[0]

    assert 'granet' in features.sparse
    assert features.sparse['granet'] == _wrap_sparse_list([form_name])


def test_case(samples_extractor):
    fex = create_features_extractor(case={})
    samples = samples_extractor(['мама моет раму'])
    features = fex(samples)[0]
    assert convert_to_sets(features.sparse_seq['case']) == _wrap_sparse_lol([['nomn'], ['UNKN'], ['accs']])
    assert not features.dense_seq


def test_classifier_charcnn(samples_extractor):
    utts = [
        'как приготовить сэндвич',
        'приготовишь сэндвич'
    ]
    samples = samples_extractor(utts)
    extractor = create_features_extractor(classifier={
        "model": 'charcnn',
        "model_file": 'resource://query_subtitles_charcnn'
    })
    features = extractor(samples)
    assert len(features[0]) == 3 and len(features[1]) == 2
    assert features[0].sparse['classifier'] == _wrap_sparse_list(['search'])
    assert features[1].sparse['classifier'] == _wrap_sparse_list(['gc'])

    extractor._extractors['classifier'].extractor.feature = 'scores'
    r = extractor(samples)
    assert extractor._extractors['classifier'].extractor.info == ['gc', 'search']
    assert not r[0].sparse and not r[1].sparse
    assert is_close(r[0].dense['classifier'], [0, 1], 0.1)
    assert is_close(r[1].dense['classifier'], [1, 0], 0.1)

    extractor._extractors['classifier'].extractor.feature = 'rankings'
    r = extractor(samples)
    assert not r[0].dense and not r[1].dense
    assert r[0].sparse['classifier'] == _wrap_sparse_list(['search>gc'])
    assert r[1].sparse['classifier'] == _wrap_sparse_list(['gc>search'])


def _assert_equal(sample_features_1, sample_features_2):
    for f1, f2 in izip(sample_features_1, sample_features_2):
        assert f1.sparse_seq == f2.sparse_seq
        assert set(f1.dense_seq).difference(f2.dense_seq) == set()
        for name in f1.dense_seq:
            assert np.array_equal(f1.dense_seq[name], f2.dense_seq[name])


def test_features_extractor_multiprocessing(features_extractor, samples_extractor, nlu_demo_data):
    items = FuzzyNLUFormat.parse_iter(sum(nlu_demo_data.itervalues(), [])).items
    samples = np.tile(samples_extractor(items), 50)

    def get_result(num_procs):
        te = time.time()
        features = features_extractor(samples, num_procs=num_procs)
        te = time.time() - te
        return te, features

    time_1proc, features_1proc = get_result(1)
    time_nproc, features_nproc = get_result(2)
    _assert_equal(features_1proc, features_nproc)


def test_features_extractor_multiprocessing_excess(features_extractor, utts_samples):
    _, samples = utts_samples
    _assert_equal(features_extractor(samples[:2], num_procs=1), features_extractor(samples[:2], num_procs=2))
    _assert_equal(features_extractor(samples[:2], num_procs=10), features_extractor(samples[:2], num_procs=2))


class TestSerp:
    @pytest.fixture(scope='class')
    def serp_resource_id(self):
        return ('resource://suggest_test/query_wizard_features/query_wizard_features.trie',
                'resource://suggest_test/query_wizard_features/query_wizard_features.data')

    @staticmethod
    def create_serp_features_extractor(params, serp_resource_id):
        params.update({
            'trie': serp_resource_id[0],
            'data': serp_resource_id[1]
        })
        return create_features_extractor(serp=params)

    @pytest.fixture(scope='class')
    def serp_fex(self, serp_resource_id):
        return TestSerp.create_serp_features_extractor({'sequence': False}, serp_resource_id)

    @pytest.fixture(scope='class')
    def serp_seq_fex(self, serp_resource_id):
        return TestSerp.create_serp_features_extractor({'sequence': True}, serp_resource_id)

    @pytest.fixture(scope='class')
    def say_weather_sample(self):
        return Sample.from_string('скажи погоду')

    @pytest.fixture(scope='class')
    def reference_answer(self):
        reference_answer = {'invlendiff': 1.0, 'longest_is_utterance': 1.0}
        # Update with images features.
        reference_answer.update({'max__avg_images': 0.3, 'max__surplus_images': 0.4,
                                 'mean__avg_images': 0.2, 'mean__surplus_images': 0.3,
                                 'wmean__avg_images': 0.225, 'wmean__surplus_images': 0.325})
        # Update with videos features.
        reference_answer.update({'max__avg_videos': 0.5, 'max__surplus_videos': 0.6,
                                 'mean__avg_videos': 0.4, 'mean__surplus_videos': 0.5,
                                 'wmean__avg_videos': 0.425, 'wmean__surplus_videos': 0.525})

        # Update with weather features.
        reference_answer.update({'max__avg_weather': 0.7, 'max__surplus_weather': 0.8,
                                 'mean__avg_weather': 0.6, 'mean__surplus_weather': 0.7,
                                 'wmean__avg_weather': 0.625, 'wmean__surplus_weather': 0.725})

        # Update with music features.
        reference_answer.update({'max__avg_music': 0.9, 'max__surplus_music': 1.0,
                                 'mean__avg_music': 0.8, 'mean__surplus_music': 0.9,
                                 'wmean__avg_music': 0.825, 'wmean__surplus_music': 0.925})

        # Update with factoid features.
        reference_answer.update({'max__avg_factoid': 1.1, 'max__surplus_factoid': 1.2,
                                 'mean__avg_factoid': 1.0, 'mean__surplus_factoid': 1.1,
                                 'wmean__avg_factoid': 1.025, 'wmean__surplus_factoid': 1.125})

        # Update with org features.
        reference_answer.update({'max__avg_org': 1.3, 'max__surplus_org': 1.4,
                                 'mean__avg_org': 1.2, 'mean__surplus_org': 1.3,
                                 'wmean__avg_org': 1.225, 'wmean__surplus_org': 1.325})

        # Update with maps features.
        reference_answer.update({'max__avg_maps': 1.5, 'max__surplus_maps': 1.6,
                                 'mean__avg_maps': 1.4, 'mean__surplus_maps': 1.5,
                                 'wmean__avg_maps': 1.425, 'wmean__surplus_maps': 1.525})

        # Update with navi features.
        reference_answer.update({'max__avg_navi': 1.7, 'max__surplus_navi': 1.8,
                                 'mean__avg_navi': 1.6, 'mean__surplus_navi': 1.7,
                                 'wmean__avg_navi': 1.625, 'wmean__surplus_navi': 1.725})

        # Update with market features.
        reference_answer.update({'max__avg_market': 1.9, 'max__surplus_market': 2.0,
                                 'mean__avg_market': 1.8, 'mean__surplus_market': 1.9,
                                 'wmean__avg_market': 1.825, 'wmean__surplus_market': 1.925})
        return reference_answer

    @pytest.fixture(scope='class')
    def say_weather_mock_answer(self):
        mock_answer = [
            {'start': 0, 'length': 5, 'normalized_fragment': 'скажи', 'fragment': 'скажи', 'features':
                [100.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8]},
            {'start': 0, 'length': 12, 'normalized_fragment': 'скажи погоду', 'fragment': 'скажи погоду', 'features':
                [200.0, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9]},
            {'start': 6, 'length': 6, 'normalized_fragment': 'погоду', 'fragment': 'погоду', 'features':
                [300.0, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0]}
        ]
        return mock_answer

    def test_serp_fex_loading_resource(self, serp_fex, say_weather_sample, say_weather_mock_answer):
        extractor = serp_fex._extractors['serp'].extractor
        features = extractor._query_wizard_features_reader.get_features(extractor._serp_input(say_weather_sample))
        for response, mock_answer in zip(features, say_weather_mock_answer):
            assert response == mock_answer

    @pytest.mark.parametrize('sample', [Sample.from_string(''), Sample.from_none()])
    def test_serp_fex_returns_empty_list_for_none_and_empty_string(self, serp_fex, sample):
        assert len(serp_fex([sample])[0]) == 0

    def test_serp_fex_num_features(self, serp_fex, serp_seq_fex, say_weather_sample):
        token_features = serp_seq_fex([say_weather_sample])[0].dense_seq['serp']
        reference = serp_seq_fex._extractors['serp'].extractor.info
        assert token_features.shape[-1] == len(reference)
        agg_features = serp_fex([say_weather_sample])[0].dense['serp']
        reference = serp_fex._extractors['serp'].extractor.info
        assert agg_features.shape[-1] == len(reference)

    def test_serp_fex_feature_calculation(self, serp_fex, say_weather_sample,
                                          reference_answer):
        tf = serp_fex([say_weather_sample])[0].dense['serp']
        feature_list = serp_fex._extractors['serp'].extractor.info
        assert set(feature_list) == set(reference_answer.keys())
        # This check is not equivalent to tf == reference_answer
        # because numpy averages, e.g. [0.2,0.3,0.4] to 0.299999999999.
        assert is_close(tf, [reference_answer[k] for k in feature_list])

    def test_serp_sequential_features(self, serp_seq_fex, say_weather_sample, say_weather_mock_answer):
        vec1 = np.array(say_weather_mock_answer[0]['features'])
        vec2 = np.array(say_weather_mock_answer[2]['features'])
        vec12 = np.array(say_weather_mock_answer[1]['features'])

        tf = serp_seq_fex([say_weather_sample])[0].dense_seq['serp']
        for i, (first, second) in enumerate([(vec1, vec12), (vec2, vec12)]):
            # weights are not modified only for this specific sentence
            weights = first[0] / (first[0] + second[0]), second[0] / (first[0] + second[0])
            assert is_close(tf[i], first[1:] * weights[0] + second[1:] * weights[1])

    def test_serp_log_features(self, serp_resource_id, say_weather_sample):
        fex = TestSerp.create_serp_features_extractor(
            {'sequence': True, 'log_frequency': True, 'log_surplus': True},
            serp_resource_id
        )
        tf = fex([say_weather_sample])[0].dense_seq['serp']
        assert is_close(tf[0, 0], np.log(0.1) * 1.0 / 3.0 + np.log(0.2) * 2.0 / 3.0)
        assert is_close(tf[0, 1], np.log(1.2 / 0.8) * 1.0 / 3.0 + np.log(1.3 / 0.7) * 2.0 / 3.0)

    def test_serp_normalization(self, serp_resource_id, say_weather_sample):
        fex = TestSerp.create_serp_features_extractor(
            {'sequence': True, 'prior_strength': 7},
            serp_resource_id
        )
        tf = fex([say_weather_sample])[0].dense_seq['serp']
        assert is_close(tf[0, 0],
                        (0.1 * 100 + 0.6 * 7) / (100 + 7) * 1.0 / 3.0 +
                        (0.2 * 200 + 0.6 * 7) / (200 + 7) * 2.0 / 3.0)
        assert is_close(tf[0, 1],
                        (0.2 * 100) / (100 + 7) * 1.0 / 3.0 +
                        (0.3 * 200) / (200 + 7) * 2.0 / 3.0)

    def test_serp_idf(self, serp_resource_id, say_weather_sample):
        freq = {'counts': {'скажи': 93, 'погоду': 3}, 'bias': 7}
        fex = TestSerp.create_serp_features_extractor(
            {'frequency_file_or_dict': freq, 'sequence': False},
            serp_resource_id
        )
        tf = fex([say_weather_sample])[0].dense['serp']
        iw = np.array([100.0 / 100 * 0.5, 200.0 / 100 * 0.5 + 200.0 / 10 * 0.5, 300 / 10 * 0.5])
        iw /= sum(iw)
        assert is_close(tf[36], sum([0.1, 0.2, 0.3] * iw))
        assert is_close(tf[37], sum([0.2, 0.3, 0.4] * iw))

    @pytest.mark.parametrize('params, non_seq, seq', [
        ({}, False, True),
        ({'sequence': False}, True, False),
        ({'sequence': True}, False, True),
    ])
    def test_serp_features_combinations(self, serp_resource_id, say_weather_sample, params, non_seq, seq):
        fex = TestSerp.create_serp_features_extractor(params, serp_resource_id)
        features = fex([say_weather_sample])[0]
        assert ('serp' in features.dense_seq) == seq
        assert ('serp' in features.dense) == non_seq

    def test_amount_of_used_features_equals_to_raw_features_list(self):
        cls = SerpFeatureExtractor
        assert (cls._USED_FEATURES_RANGE[1] - cls._USED_FEATURES_RANGE[0]) == len(cls._RAW_FEATURE_NAMES)


def test_sample_features(dummy_embeddings):
    fex = create_features_extractor(
        ngrams={'n': 1},
        embeddings={'file': dummy_embeddings},
        classifier={
            "model": "charcnn",
            "model_file": "resource://query_subtitles_charcnn",
            "feature": "scores"
        })
    sample_features = fex([
        Sample.from_string('погода на колыме'),
    ])[0]
    assert sample_features.sparse_seq
    assert sample_features.dense_seq
    assert is_close(sample_features.dense['classifier'], [0, 1], tolerance=1e-2)

    assert convert_to_sets(sample_features.sparse_seq['ngrams']) == _wrap_sparse_lol([
        ['погода'], ['на'], ['колыме']
    ])

    assert np.array_equal(sample_features.dense_seq['embeddings'], np.array([
        [1., 1., 1.],
        [0., 0., 0.],
        [0., 0., 0.]
    ]))


@pytest.fixture(scope='module')
def dssm_embeddings():
    return {'resource': 'resource://dssm_embeddings/tf_model'}


_DSSM_EMBEDDINGS_DIM = 50


@pytest.fixture(scope='module')
def feature_extractor_all_types(dummy_embeddings, parser_base):
    factory = FeaturesExtractorFactory()
    factory.register_parser(parser_base)
    features_cfg = [
        {'type': 'ngrams', 'id': 'word', 'params': {'n': 1}},
        {'type': 'ner', 'id': 'ner'},
        {'type': 'embeddings', 'id': 'embeddings', 'params': {'file': dummy_embeddings}},
        {'type': 'classifier', 'id': 'clf_scores', 'params': {
            'model': 'charcnn',
            'model_file': 'resource://query_subtitles_charcnn',
            'feature': 'scores'
        }},
        {'type': 'classifier', 'id': 'clf_labels', 'params': {
            'model': 'charcnn',
            'model_file': 'resource://query_subtitles_charcnn',
            'feature': 'label'
        }},
    ]
    for cfg in features_cfg:
        factory.add(cfg['id'], cfg['type'], **cfg.get('params', {}))

    return factory.create_extractor()


@pytest.mark.parametrize('sparse', (True, False))
@pytest.mark.parametrize('sequential', (True, False))
@pytest.mark.parametrize('select_features', [
    # sparse_seq
    ('word', 'ner'),
    # sparse
    ('clf_labels',),
    # dense_seq
    ('embeddings',),
    # dense
    ('clf_scores',),
    # sparse_seq, dense_seq
    ('word', 'ner', 'embeddings'),
    # sparse_seq, dense
    ('word', 'ner', 'clf_scores'),
    # sparse_seq, sparse
    ('word', 'ner', 'clf_labels'),
    # dense_seq, dense
    ('embeddings', 'clf_scores'),
    # dense_seq, sparse
    ('embeddings', 'clf_labels'),
    # dense, sparse
    ('clf_scores', 'clf_labels'),
    # sparse_seq, dense_seq, dense
    ('word', 'ner', 'embeddings', 'clf_scores'),
    # sparse_seq, dense_seq, sparse
    ('word', 'ner', 'embeddings', 'clf_labels'),
    # sparse_seq, dense, sparse
    ('word', 'ner', 'embeddings', 'clf_labels'),
    # dense_seq, dense, sparse
    ('embeddings', 'clf_scores', 'clf_labels'),
    # all
    ('word', 'ner', 'embeddings', 'clf_scores', 'clf_labels')
])
def test_vectorizer(feature_extractor_all_types, sparse, sequential, select_features):
    selector = SelectorFeaturesPostProcessor(select_features)
    vectorizer = VectorizerFeaturesPostProcessor(sparse=sparse, sequential=sequential)
    sample_features = selector.fit_transform(feature_extractor_all_types([
        Sample.from_string('погода в москве'),
        Sample.from_string('а ты где'),
        Sample.from_string('погода в магадане')
    ]))
    vectorizer.fit(sample_features[:2])
    result = vectorizer.transform(sample_features[2:])[0]

    # test sparse/dense output
    if sparse:
        assert isinstance(result, scipy.sparse.csr_matrix)
        result = result.toarray()
    else:
        assert isinstance(result, np.ndarray)

    # test vocabulary
    if select_features == ('word', 'ner'):
        assert vectorizer.vocabulary == [
            'ner=B-GEO', 'word=а', 'word=в', 'word=где', 'word=москве', 'word=погода', 'word=ты'
        ]
        if sequential:
            assert is_close(result, [
                [0, 0, 0, 0, 0, 1, 0],
                [0, 0, 1, 0, 0, 0, 0],
                [1, 0, 0, 0, 0, 0, 0]
            ], 1e-2)
        else:
            assert is_close(result, [
                1, 0, 1, 0, 0, 1, 0
            ], 1e-2)
    elif select_features == ('embeddings', 'clf_labels', 'clf_scores'):
        assert vectorizer.vocabulary == [
            'clf_labels=gc', 'clf_labels=search'
        ]
        if sequential:
            assert is_close(result, [
                [0, 1, 0, 0, 0, 0.01, 0.99],
                [0, 1, 0, 0, 0, 0.01, 0.99],
                [0, 1, 1, 1, 1, 0.01, 0.99],
            ], 1e-2)
        else:
            assert is_close(result, [
                0, 1, 1, 1, 1, 0, 1
            ], 1e-2)
    elif select_features == ('clf_scores', 'clf_labels'):
        if sequential:
            assert is_close(result, [
                [0, 1, 0.01, 0.99],
                [0, 1, 0.01, 0.99],
                [0, 1, 0.01, 0.99]
            ], 1e-2)
        else:
            assert is_close(result, [
                0, 1, 0.01, 0.99
            ], 1e-2)
    elif select_features == ('word', 'ner', 'clf_labels'):
        assert vectorizer.vocabulary == [
            'clf_labels=gc', 'clf_labels=search',
            'ner=B-GEO', 'word=а', 'word=в', 'word=где', 'word=москве', 'word=погода', 'word=ты'
        ]
        if sequential:
            # clf_labels, ner, word
            assert is_close(result, [
                [0, 1, 0, 0, 0, 0, 0, 1, 0],
                [0, 1, 0, 0, 1, 0, 0, 0, 0],
                [0, 1, 1, 0, 0, 0, 0, 0, 0]
            ], 1e-2)
        else:
            assert is_close(result, [
                0, 1, 1, 0, 1, 0, 0, 1, 0
            ], 1e-2)
    elif select_features == ('embeddings', 'clf_scores'):
        assert not vectorizer.vocabulary
        if sequential:
            assert is_close(result, [
                [1, 1, 1, 0.01, 0.99],
                [0, 0, 0, 0.01, 0.99],
                [0, 0, 0, 0.01, 0.99]
            ], 1e-2)
        else:
            assert is_close(result, [
                [1, 1, 1, 0.01, 0.99]
            ], 1e-2)
    elif select_features == ('word', 'ner', 'embeddings', 'clf_scores', 'clf_labels'):
        assert vectorizer.vocabulary == [
            'clf_labels=gc', 'clf_labels=search',
            'ner=B-GEO', 'word=а', 'word=в', 'word=где', 'word=москве', 'word=погода', 'word=ты',
        ]
        if sequential:
            assert is_close(result, [
                # clf_labels, ner, word, embeddings, clf_scores
                [0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0.01, 0.99],
                [0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0.01, 0.99],
                [0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.01, 0.99],
            ], 1e-2)
        else:
            assert is_close(result, [
                0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0.01, 0.99
            ], 1e-2)
    elif select_features == ('word', 'ner', 'embeddings', 'clf_scores'):
        assert vectorizer.vocabulary == [
            'ner=B-GEO', 'word=а', 'word=в', 'word=где', 'word=москве', 'word=погода', 'word=ты',
        ]
        if sequential:
            assert is_close(result, [
                # ner, word, embeddings, clf_scores
                [0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0.01, 0.99],
                [0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0.01, 0.99],
                [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.01, 0.99],
            ], 1e-2)
        else:
            assert is_close(result, [
                1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0.01, 0.99
            ], 1e-2)
    # TODO: add other parameter settings check if needed


def test_selector(dummy_embeddings, dummy_embeddings_2, dssm_embeddings):
    factory = FeaturesExtractorFactory()
    features_cfg = [
        {'type': 'ngrams', 'id': 'word', 'params': {'n': 1}},
        {'type': 'ngrams', 'id': 'bigram', 'params': {'n': 2}},
        {'type': 'embeddings', 'id': 'embeddings_1', 'params': {'file': dummy_embeddings}},
        {'type': 'embeddings', 'id': 'embeddings_2', 'params': {'file': dummy_embeddings_2}},
        {'type': 'dssm_embeddings', 'id': 'dssm_embeddings', 'params': dssm_embeddings},
        {'type': 'classifier', 'id': 'classifier', 'params': {
            'model': 'charcnn',
            'model_file': 'resource://query_subtitles_charcnn',
            'feature': 'scores'
        }}
    ]
    for cfg in features_cfg:
        factory.add(cfg['id'], cfg['type'], **cfg.get('params', {}))

    fex = factory.create_extractor()
    selector = SelectorFeaturesPostProcessor(['word', 'bigram', 'embeddings_1'])
    test_feature = fex([Sample.from_string('такси на льва толстого')])
    out = selector.transform(test_feature)[0]
    assert 'embeddings_1' in out.dense_seq and 'embeddings_2' not in out.dense_seq
    assert 'word' in out.sparse_seq and 'bigram' in out.sparse_seq
    selector = SelectorFeaturesPostProcessor(['dssm_embeddings', 'classifier'])
    out = selector.transform(test_feature)[0]
    assert 'classifier' in out.dense and 'dssm_embeddings' in out.dense
    selector = SelectorFeaturesPostProcessor(['bigram', 'dssm_embeddings', 'classifier'])
    out = selector.transform(test_feature)[0]
    assert 'word' not in out.sparse_seq and 'bigram' in out.sparse_seq
    assert 'classifier' in out.dense
    assert 'dssm_embeddings' in out.dense


def test_selector_dropout(parser):
    factory = FeaturesExtractorFactory()
    factory.register_parser(parser)
    features_cfg = [
        {'type': 'ner', 'id': 'ner'}
    ]
    for cfg in features_cfg:
        factory.add(cfg['id'], cfg['type'], **cfg.get('params', {}))

    fex = factory.create_extractor()
    test_feature = fex([Sample.from_string('включи сплин')])

    selector = SelectorFeaturesPostProcessor(['ner'])
    out = selector.transform(test_feature)[0]
    assert 'ner' in out.sparse_seq
    assert 'B-ARTIST' in {val.value for val in sum(out.sparse_seq['ner'], [])}

    selector = SelectorFeaturesPostProcessor(['ner'], dropout_config={'sparse_seq': {'ner': {'artist': 1.0}}})
    selector._fit_params['do_dropout'] = True
    out = selector.transform(test_feature)[0]
    assert 'ner' in out.sparse_seq
    assert 'B-ARTIST' not in {val.value for val in sum(out.sparse_seq['ner'], [])}

    selector = SelectorFeaturesPostProcessor(['ner'], dropout_config={'sparse_seq': {'ner': {'artist': 0.5}}})
    hits = 0

    for _ in range(1000):
        selector._fit_params['do_dropout'] = True
        out = selector.transform(test_feature)[0]
        assert 'ner' in out.sparse_seq
        if 'B-ARTIST' in {val.value for val in sum(out.sparse_seq['ner'], [])}:
            hits += 1

    assert abs(hits - 500) < 30


def test_dssm_embeddings(dummy_embeddings, dssm_embeddings):
    factory = FeaturesExtractorFactory()
    features_cfg = [
        {'type': 'ngrams', 'id': 'word', 'params': {'n': 1}},
        {'type': 'embeddings', 'id': 'embeddings', 'params': {'file': dummy_embeddings}},
        {'type': 'dssm_embeddings', 'id': 'dssm_embeddings', 'params': dssm_embeddings}
    ]
    for cfg in features_cfg:
        factory.add(cfg['id'], cfg['type'], **cfg.get('params', {}))

    fex = factory.create_extractor()
    output = fex([Sample.from_string('улица погода')])[0]
    assert 'dssm_embeddings' in output.dense
    emb = output.dense['dssm_embeddings']
    assert emb.shape[0] == _DSSM_EMBEDDINGS_DIM
    assert is_close(np.dot(emb.T, emb), 1, 1e-4)


@pytest.fixture(scope='module')
def dssm_embeddings_features_extractor(dssm_embeddings):
    return create_features_extractor(dssm_embeddings=dssm_embeddings)


@pytest.mark.parametrize('anchor, positive, negative', [
    ('добрый день', 'привет', 'пока'),
    ('мне нужно проснуться', 'разбуди меня', 'спокойной ночи'),
    ('до новых встреч', 'пока', 'привет')
])
def test_dssm_embeddings_capture_semantics(dssm_embeddings_features_extractor, anchor, positive, negative):

    def get_vec(text):
        sample_feature = dssm_embeddings_features_extractor([Sample.from_string(text)])[0]
        return sample_feature.dense['dssm_embeddings']

    positive_score = np.dot(get_vec(anchor), get_vec(positive))
    negative_score = np.dot(get_vec(anchor), get_vec(negative))
    assert positive_score > 1.3 * negative_score


@pytest.mark.parametrize(
    'fe_class, fe_name',
    [
        (NGramFeatureExtractor, 'word'),
        (NerFeatureExtractor, 'ner'),
        (EmbeddingsFeaturesExtractor, 'embeddings')
    ]
)
def test_features_extractor_error_meta(features_extractor, utts_samples, fe_class, fe_name):
    vins_response = VinsResponse()
    with mock.patch.object(fe_class, '__call__') as m:
        m.side_effect = Exception()
        features_extractor(utts_samples[1], response=vins_response)
    assert FeaturesExtractorErrorMeta(
        type='error',
        error_type='features_extractor_error',
        features_extractor=fe_name,
    ) in vins_response.meta


@pytest.mark.parametrize('ngram, input, output', [
    (3, 'мама мыла раму', (['<s> <s> мама'], ['<s> мама мыла'], ['мама мыла раму </s>'])),
    (1, 'мама мыла раму', (['мама'], ['мыла'], ['раму'])),
    (2, '', None),
])
def test_ngram_feature_extractor(ngram, input, output):
    fex = NGramFeatureExtractor(n=ngram)
    if output:
        output = _wrap_sparse_lol(output, sets_mode=False)

    assert fex(Sample.from_string(input))[0].data == output


@pytest.mark.parametrize('ngram, input, output', [
    (2, 'мама', _wrap_sparse_list(['<s>м', 'ма', 'ма</s>', 'ам'])),
    (1, 'мама', _wrap_sparse_list(['м', 'а'])),
    (3, '', None)
])
def test_bagofchar(ngram, input, output):
    fex = create_features_extractor(bagofchar={'n': ngram})
    result = fex([Sample.from_string(input)])[0]
    if output:
        assert set(result.sparse['bagofchar']) == set(output)
    else:
        assert 'bagofchar' not in result.sparse


def _equal_sample_features_lists(x, y):
    if len(x) != len(y):
        return False
    for sx, sy in izip(x, y):
        sxf = sx.sample_features
        syf = sy.sample_features
        if any((
            sxf.sparse != syf.sparse,
            sxf.sparse_seq != syf.sparse_seq,
            not all(is_close(sxf.dense[key], syf.dense[key]) for key in sxf.dense),
            not all(is_close(sxf.dense_seq[key], syf.dense_seq[key]) for key in sxf.dense_seq),
        )):
            return False
    return True


@pytest.fixture(scope='module')
def items_with_trainable_classifiers(utts):
    return FuzzyNLUFormat.parse_iter(utts, trainable_classifiers=('test',)).items


@pytest.fixture(scope='module', params=('one embeddings', 'two embeddings'))
def features_extractor_2_embeddings(request, dummy_embeddings, dummy_embeddings_2, custom_ner_currency_parser):
    factory = FeaturesExtractorFactory()
    factory.register_parser(custom_ner_currency_parser)
    features_cfg = [
        {'type': 'ngrams', 'id': 'word', 'params': {'n': 1}},
        {'type': 'ngrams', 'id': 'bigram', 'params': {'n': 2}},
        {'type': 'ner', 'id': 'ner'},
        {'type': 'postag', 'id': 'postag'},
        {'type': 'lemma', 'id': 'lemma'},
        {'type': 'embeddings', 'id': 'embeddings', 'params': {'file': dummy_embeddings}},
    ]
    if request.param == 'two embeddings':
        features_cfg.append({'type': 'embeddings', 'id': 'embeddings_2', 'params': {'file': dummy_embeddings_2}})
    for cfg in features_cfg:
        factory.add(cfg['id'], cfg['type'], **cfg.get('params', {}))

    return factory.create_extractor()


@pytest.fixture(params=[PickleCache, SampleFeaturesCache, YtProtobufFeatureCache])
def feature_cache(
    request, tmpdir_factory, items_with_trainable_classifiers, features_extractor_2_embeddings, samples_extractor
):
    cache_class = request.param
    tempdir = tmpdir_factory.mktemp('feature_cache')
    feature_cache_file = str(tempdir.join('feature_cache.pkl'))
    empty_feature_cache = cache_class(feature_cache_file)
    if isinstance(empty_feature_cache, SampleFeaturesCache):
        empty_feature_cache.check_consistency(features_extractor_2_embeddings)

    extractor = FeatureExtractorFromItem(
        samples_extractor, features_extractor_2_embeddings, empty_feature_cache, trainable_classifiers=('test',)
    )
    extractor(items_with_trainable_classifiers)
    # Yt cache do not save state between reruns
    # It is feature of YtProtobufFeatureCache
    if cache_class == YtProtobufFeatureCache:
        yield empty_feature_cache
    else:
        full_feature_cache = cache_class(feature_cache_file)
        yield full_feature_cache
    tempdir.remove()


def test_different_feature_sets(feature_cache):
    if isinstance(feature_cache, SampleFeaturesCache):
        assert feature_cache._schema_initialized

        factory = FeaturesExtractorFactory()
        factory.add('этого id нет в features_extractor_2_embeddings', 'ngrams', n=1)
        features_extractor = factory.create_extractor()

        with pytest.raises(ValueError):
            feature_cache.check_consistency(features_extractor)


def test_cached_features_equal_to_original(
    feature_cache, samples_extractor, features_extractor_2_embeddings, items_with_trainable_classifiers
):
    extractor = FeatureExtractorFromItem(
        samples_extractor, features_extractor_2_embeddings, feature_cache=None, trainable_classifiers=('test',)
    )
    features = extractor(items_with_trainable_classifiers)
    cached_features = [feature_cache[item] for item in items_with_trainable_classifiers]
    cached_features = feature_cache.update(items_with_trainable_classifiers, cached_features)
    assert _equal_sample_features_lists(features, cached_features)


@pytest.mark.parametrize('num_procs', (1, 2, 10))
def test_with_feature_cache(
    mocker, items_with_trainable_classifiers, features_extractor_2_embeddings, feature_cache, num_procs,
    samples_extractor
):
    dummy_non_cached = 'dummy non cached'
    get_sample_features_mock = mocker.patch.object(
        vins_core.nlu.features_extractor.FeaturesExtractor, 'get_sample_features',
        return_value=features_extractor_2_embeddings([Sample.from_string(dummy_non_cached)])[0]
    )
    num_new_samples = 3
    items = copy.deepcopy(items_with_trainable_classifiers)
    items.extend(FuzzyNLUFormat.parse_iter(
        ['фраза, которой нет в кеше %d' % num_procs] * num_new_samples, trainable_classifiers=('test',)
    ).items)
    extractor = FeatureExtractorFromItem(
        samples_extractor, features_extractor_2_embeddings, feature_cache, trainable_classifiers=('test',)
    )
    features = extractor(items=items, num_procs=num_procs)
    assert len(features) == len(items_with_trainable_classifiers) + num_new_samples
    if num_procs == 1:
        assert get_sample_features_mock.call_count == num_new_samples
    assert Counter(feature.sample_features.sample.text for feature in features)[dummy_non_cached] == num_new_samples


@pytest.fixture
def regexp_feature_file(tmpdir):
    return 'vins_core/test/test_data/test.regexp.feature.json'


@pytest.fixture
def regexp_feature_extractor(regexp_feature_file):
    return create_features_extractor(regexp={'data_or_filepath': regexp_feature_file})


@pytest.mark.parametrize('input, output', [
    ('включи музыку пожалуйста',
     [[SparseFeatureValue('B-re:music')], [SparseFeatureValue('I-re:music')], []]),
    ('пожалуйста включи музыку',
     [[], [SparseFeatureValue('B-re:music')], [SparseFeatureValue('I-re:music')]]),
    ('включи музыку пожалуйста включай музыку',
     [[SparseFeatureValue('B-re:music')], [SparseFeatureValue('I-re:music')], [],
      [SparseFeatureValue('B-re:music')], [SparseFeatureValue('I-re:music')]]),
    ('включай видео',
     [[SparseFeatureValue('B-re:video')], [SparseFeatureValue('I-re:video')]]),
    ('включай музыку включи видео',
     [[SparseFeatureValue('B-re:music')], [SparseFeatureValue('I-re:music')],
      [SparseFeatureValue('B-re:video')], [SparseFeatureValue('I-re:video')]]),
    ('включи мозги', [[], []])
])
def test_regexp(samples_extractor, regexp_feature_extractor, input, output):
    samples = samples_extractor([input])
    features = regexp_feature_extractor(samples)[0].sparse_seq['regexp']
    assert features == output


def test_ner_dense_extractor(samples_extractor, parser):
    # this test depends on the contents of the geo and num FST, and might be changed if the FST changes
    ner_ext = create_features_extractor(parser=parser, ner={'dense_features': ['GEO', 'NUM', 'FOO BAR']})
    utt = 'улица Льва Толстого 16'
    sample = samples_extractor([NluSourceItem(utt)])
    features = ner_ext(sample)[0]
    assert is_close(features.dense_seq['ner'], np.array([[1, 0, 0], [1, 0, 0], [1, 0, 0], [1, 1, 0]]))

    expected_sparse_ner_subsets = [
        {'B-GEO'}, {'I-GEO'}, {'I-GEO'}, {'I-GEO', 'B-NUM'}
    ]
    for true_token, expected_token in zip(features.sparse_seq['ner'], expected_sparse_ner_subsets):
        assert expected_token.difference({feature.value for feature in true_token}) == set()


def test_ner_dense_extractor_with_weight(samples_extractor, parser):
    # this test depends on the contents of the music FST, and might be changed if the FST changes
    ner_ext = create_features_extractor(parser=parser, ner={'dense_features': ['ARTIST', 'ALBUM', 'TRACK']})
    utt = 'включи земфиру'
    sample = samples_extractor([NluSourceItem(utt)])
    features = ner_ext(sample)[0]
    assert is_close(features.dense_seq['ner'][1], np.array([0.88, 0.11, 0.01]), 0.05)


@pytest.mark.parametrize('text', [
    'добрый день', 'привет', 'пока',
    'включи музыку пожалуйста',
    'поставь будильника на 8 утра',
])
def test_sample_features_to_bytes_and_from_bytes(text, samples_extractor, feature_extractor_all_types):
    samples = samples_extractor([text])
    features = feature_extractor_all_types(samples)[0]
    bytes_sf = features.to_bytes()
    features_copy = SampleFeatures.from_bytes(bytes_sf)
    for key in features.dense:
        assert features_copy.dense[key].tostring() == features.dense[key].tostring()
    assert dict(features.sparse) == dict(features_copy.sparse)


def test_sample_features_classification_scores_to_bytes_and_from_bytes(samples_extractor):
    scores = {
        'stage1': [IntentScore(name='intent1', score=1.0), IntentScore(name='intent2', score=0.5)],
        'stage2': [IntentScore(name='intent1', score=0.8), IntentScore(name='intent2', score=0.6)]
    }

    samples = samples_extractor(['text'])
    features = SampleFeatures(sample=samples[0], classification_scores=scores)
    bytes_sf = features.to_bytes()
    features_copy = SampleFeatures.from_bytes(bytes_sf)
    for score, expected_score in izip(features_copy.classification_scores, scores):
        assert score == expected_score


def test_sample_features_tagger_scores_to_bytes_and_from_bytes(samples_extractor):
    slots = [TaggerSlot(start=0, end=1, is_continuation=False, value='some_slot1'),
             TaggerSlot(start=1, end=2, is_continuation=False, value='some_slot2')]
    scores = {
        'stage1': [TaggerScore(intent='intent1', score=1.0, slots=slots),
                   TaggerScore(intent='intent2', score=0.5, slots=slots)],
        'stage2': [TaggerScore(intent='intent1', score=0.8, slots=slots),
                   TaggerScore(intent='intent2', score=0.6, slots=slots)]
    }

    samples = samples_extractor(['text'])
    features = SampleFeatures(sample=samples[0], tagger_scores=scores)
    bytes_sf = features.to_bytes()
    features_copy = SampleFeatures.from_bytes(bytes_sf)
    for score, expected_score in izip(features_copy.tagger_scores, scores):
        assert score == expected_score


def test_music_features_extractor_with_trivial_annotation():
    wizard_annotation = {'rules': {}}
    feature_extractor = MusicFeaturesExtractor()
    sample = Sample.from_string('metallica nothing else matters')
    sample.annotations['wizard'] = WizardAnnotation.from_dict(wizard_annotation)
    features = feature_extractor(sample)
    assert len(features) == 1
    assert np.all(features[0].data == np.array([
        0, 0.0, 0.0, -1, -1,
        0, 0.0, 0.0, -1, -1,
        0, 0.0, 0.0, -1, -1
    ]))


def test_music_features_extractor_with_multiple_occurrences():
    wizard_annotation = {
        'rules': {
            'MusicFeatures': {
                'Tokens': ['metallica', 'nothing', 'else', 'matters'],
                'Occurrences': [
                    {
                        "ArtistPopularity": 55456,
                        "AlbumPopularity": 0,
                        "TrackPopularity": 468,
                        "Begin": 0,
                        "End": 1
                    },
                    {
                        "ArtistPopularity": 60,
                        "AlbumPopularity": 0,
                        "TrackPopularity": 2144,
                        "Begin": 1,
                        "End": 2
                    },
                    {
                        "ArtistPopularity": 0,
                        "AlbumPopularity": 0,
                        "TrackPopularity": 1050,
                        "Begin": 1,
                        "End": 3
                    },
                    {
                        "ArtistPopularity": 4140,
                        "AlbumPopularity": 0,
                        "TrackPopularity": 0,
                        "Begin": 2,
                        "End": 3
                    },
                    {
                        "ArtistPopularity": 0,
                        "AlbumPopularity": 0,
                        "TrackPopularity": 55452,
                        "Begin": 1,
                        "End": 4
                    },
                    {
                        "ArtistPopularity": 0,
                        "AlbumPopularity": 0,
                        "TrackPopularity": 11,
                        "Begin": 3,
                        "End": 4
                    }
                ]
            }
        }
    }
    feature_extractor = MusicFeaturesExtractor()
    sample = Sample.from_string('metallica nothing else matters')
    sample.annotations['wizard'] = WizardAnnotation.from_dict(wizard_annotation)
    features = feature_extractor(sample)
    assert len(features) == 1
    assert np.allclose(features[0].data, np.array([
        55456, 0.25, 13864.0,  0,  3,  # noqa: E241
        0,      0.0,     0.0, -1, -1,  # noqa: E241
        55452, 0.75, 41589.0,  1,  0   # noqa: E241
    ]))


def test_music_features_extractor_with_no_occurrences():
    wizard_annotation = {
        'rules': {
            'MusicFeatures': {
                'Tokens': ['metallica']
            }
        }
    }
    feature_extractor = MusicFeaturesExtractor()
    sample = Sample.from_string('metallica')
    sample.annotations['wizard'] = WizardAnnotation.from_dict(wizard_annotation)
    features = feature_extractor(sample)
    assert len(features) == 1
    assert np.all(features[0].data == np.array([
        0, 0.0, 0.0, -1, -1,  # noqa: E241
        0, 0.0, 0.0, -1, -1,  # noqa: E241
        0, 0.0, 0.0, -1, -1   # noqa: E241
    ]))
