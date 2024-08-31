# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import pytest
import numpy as np

from collections import Counter

from vins_core.common.annotations import WizardAnnotation
from vins_core.common.sample import Sample
from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.nlu.token_tagger import create_token_tagger
from vins_core.nlu.granet_token_tagger import align_sequences
from vins_core.nlu.features_extractor import FeaturesExtractorFactory
from vins_core.nlu.features.base import SampleFeatures
from vins_core.nlu.features.extractor.base import SparseFeatureValue
from vins_core.nlu.nlu_data_cache import NluDataCache
from vins_core.dm.request import create_request
from vins_core.utils.misc import gen_uuid_for_tests


@pytest.fixture(scope='module')
def data_taxi_alarm(samples_extractor):
    return {
        'alarm': samples_extractor(FuzzyNLUFormat.parse_iter([
            'поставь будильник на "6 утра"(when)',
            'разбуди меня в "9 вечера"(when)',
        ]).items),
        'taxi': samples_extractor(FuzzyNLUFormat.parse_iter([
            'поехали до "проспекта Маршала Жукова 3"(location_to)',
            'машину от "тверской улицы"(location_from) в "без 15 6 вечера"(when) не дороже "500 рублей"(price)',
        ]).items)
    }


@pytest.fixture(scope='module')
def features_extractor(parser):
    factory = FeaturesExtractorFactory()
    factory.register_parser(parser)
    features_cfg = [
        {'type': 'ngrams', 'id': 'word', 'params': {'n': 1}},
        {'type': 'ngrams', 'id': 'bigram', 'params': {'n': 2}},
        {'type': 'ner', 'id': 'ner'},
        {'type': 'postag', 'id': 'postag'},
        {'type': 'lemma', 'id': 'lemma'},
        {'type': 'granet', 'id': 'granet'}
    ]
    for cfg in features_cfg:
        factory.add(cfg['id'], cfg['type'], **cfg.get('params', {}))

    return factory.create_extractor()


@pytest.fixture(scope='module')
def features_taxi_alarm(data_taxi_alarm, features_extractor):
    return {intent: features_extractor(samples) for intent, samples in data_taxi_alarm.iteritems()}


@pytest.fixture(scope='module')
def some_data(features_taxi_alarm):
    return sum(features_taxi_alarm.itervalues(), [])


def test_crf(features_taxi_alarm, features_extractor):
    # In this example dropout removes 'Маршала Жукова три' and related features
    tagger = create_token_tagger('crf', select_features=('word', 'bigram', 'postag'))
    tagger.train(features_taxi_alarm['taxi'])

    utt = 'до столешникова переулка 11'
    x = features_extractor([Sample(utt.split())])
    y, p = tagger.predict(x)

    assert y[0][0] == ['O', 'B-location_to', 'I-location_to', 'I-location_to']


def test_remove_duplicated_tags(features_taxi_alarm, features_extractor):
    tagger = create_token_tagger('crf', nbest=30, select_features=('word', 'lemma', 'ner'))
    tagger.train(features_taxi_alarm['taxi'])
    utt = 'поехали москва астрахань'
    x = features_extractor([Sample(utt.split())])
    y, p = tagger.predict(x)
    for path in y[0]:
        tag_counts = Counter(path)
        assert tag_counts['location_from'] <= 1
        assert tag_counts['location_to'] <= 1


def test_crossvalidation(some_data):
    assert create_token_tagger('crf').crossvalidation(some_data, average='micro') > 0.8
    assert create_token_tagger('crf').crossvalidation(some_data, average='macro') > 0.6


def test_gridsearch(some_data):
    tagger = create_token_tagger('crf')
    report = tagger.gridsearch(some_data, dict(
        c2=[0.001, 1000],
        window_size=[1, 5],
    ), n_jobs=1)
    assert report.loc[report.rank_test_score.idxmin(), 'param_crfmodel__c2'] == 0.001
    assert report.loc[report.rank_test_score.idxmin(), 'param_splicerfeaturespostprocessor__window_size'] == 1


@pytest.mark.parametrize('model', ('crf',))
@pytest.mark.parametrize('intent_conditioned', (True, False))
def test_intent_conditioned(features_taxi_alarm, model, intent_conditioned):
    tagger = create_token_tagger(
        model=model,
        intent_conditioned=intent_conditioned,
        nb_epoch=100,
        select_features=('word', 'lemma', 'ner')
    )
    tagger.train(features_taxi_alarm)
    test_sample = features_taxi_alarm['taxi'][1]
    tag_pred, prob_pred = tagger.predict([test_sample], intent='alarm')
    tag_pred, prob_pred = tag_pred[0], prob_pred[0]
    assert len(tag_pred) > 0 and len(prob_pred) > 0
    all_tags_found = set(sum(tag_pred, []))
    if intent_conditioned:
        assert all_tags_found.issubset({'O', 'B-when', 'I-when'})
    else:
        assert 'B-location_from' in all_tags_found


def test_duplicate_paths(features_extractor):
    tagger = create_token_tagger('crf', nbest=30)
    utt = 'поехали москва астрахань'
    x = features_extractor([Sample(utt.split())])
    tagger.train(x)
    y, p = tagger.predict(x)
    assert np.shape(y) == (1, 1, 3)


def test_weighted_features():
    tagger = create_token_tagger('crf', select_features=('ner',))
    features1 = SampleFeatures(
        Sample.from_nlu_source_item(FuzzyNLUFormat.parse_one("включи 'X'(artist)")),
        sparse_seq={
            'ner': [
                [], [
                    SparseFeatureValue('B-ARTIST', 0.9),
                    SparseFeatureValue('B-TRACK', 0.1),
                    SparseFeatureValue('B-ALBUM', 0.1)
                ]
            ]
        }
    )
    features2 = SampleFeatures(
        Sample.from_nlu_source_item(FuzzyNLUFormat.parse_one("включи 'X'(track)")),
        sparse_seq={
            'ner': [
                [], [
                    SparseFeatureValue('B-ARTIST', 0.1),
                    SparseFeatureValue('B-TRACK', 0.9),
                    SparseFeatureValue('B-ALBUM', 0.1)
                ]
            ]
        }
    )
    features3 = SampleFeatures(
        Sample.from_nlu_source_item(FuzzyNLUFormat.parse_one("включи 'X'(album)")),
        sparse_seq={
            'ner': [
                [], [
                    SparseFeatureValue('B-ARTIST', 0.1),
                    SparseFeatureValue('B-TRACK', 0.1),
                    SparseFeatureValue('B-ALBUM', 0.9)
                ]
            ]
        }
    )

    tagger_input = {
        'music_play': [
            features1,
            features2,
            features3
        ]
    }

    tagger.train(tagger_input)

    tag_pred, prob_pred = tagger.predict([features1], intent='music_play')
    assert len(tag_pred) > 0 and len(prob_pred) > 0
    assert tag_pred[0][0] == ['O', 'B-artist']

    tag_pred, prob_pred = tagger.predict([features2], intent='music_play')
    assert len(tag_pred) > 0 and len(prob_pred) > 0
    assert tag_pred[0][0] == ['O', 'B-track']

    tag_pred, prob_pred = tagger.predict([features3], intent='music_play')
    assert len(tag_pred) > 0 and len(prob_pred) > 0
    assert tag_pred[0][0] == ['O', 'B-album']


def test_combine_scores_token_tagger(features_extractor, features_taxi_alarm):
    taggers = [
        {'model': 'crf', 'features': ['word'], 'params': {'nbest': 5}},
        {'model': 'crf', 'features': ['word', 'lemma', 'ner'], 'params': {'nb_epoch': 50, 'nbest': 5}}
    ]
    combine_scores_tagger = create_token_tagger('combine_scores', taggers=taggers)
    combine_scores_tagger.train(features_taxi_alarm['taxi'])

    utt = 'поехали до тверской улицы'
    x = features_extractor([Sample(utt.split())])
    y, p = combine_scores_tagger.predict(x)

    assert len(y[0]) == 10
    assert len(p[0]) == 10

    assert len([1 for tags in y[0] if tags == ['O', 'O', 'B-location_from', 'I-location_from']]) == 2


@pytest.fixture
def nlu_sources_data(scope='module'):
    nlu_content_1 = [
        "сколько будет 'пять'(percent) процентов от '100'(value)?",
        "в каком 'городе'(town) живет 'Баба-яга'(person)?",
        "назови три самых популярных мемчика"
    ]

    nlu_sources_data = NluDataCache()
    nlu_sources_data.add('intent1', FuzzyNLUFormat.parse_iter(nlu_content_1).items)

    return nlu_sources_data


@pytest.mark.parametrize('utt, exact_matched_intents, intent, tags, score', [
    ('сколько будет пять процентов от 100?', ['intent1'], 'intent1',
     [['O', 'O', 'B-percent', 'O', 'O', 'B-value']], [1.0]),
    ('сколько будет пять процентов от 100?', [], 'intent1',
     [], []),
    ('сколько будет пять процентов от 100?', ['intent1'], 'intent2',
     [], []),
    ('в каком городе живет Баба-яга?', ['intent1'], 'intent1',
     [['O', 'O', 'B-town', 'O', 'B-person']], [1.0]),
    ('назови три самых популярных мемчика', ['intent1'], 'intent1',
     [['O', 'O', 'O', 'O', 'O']], [1.0]),
])
def test_exact_match_token_tagger(utt, exact_matched_intents, intent, tags, score, features_extractor,
                                  samples_extractor, nlu_sources_data):
    exact_match_token_tagger = create_token_tagger(model='nlu_exact_matching',
                                                   samples_extractor=samples_extractor,
                                                   exact_matched_intents=exact_matched_intents,
                                                   nlu_sources_data=nlu_sources_data)

    x = features_extractor(samples_extractor([utt]))
    y, p = exact_match_token_tagger.predict(x, intent)

    assert y[0] == tags
    assert p[0] == score


@pytest.mark.parametrize('utt, intent, tags, score', [
    ('давай поболтаем', 'gc', [['O', 'B-skill_id']], [1]),
    ('давай поболтаем', 'music', [], []),
    ('пожалуйста включи песню любите девушки на яндекс музыке', 'music',
     [['O', 'B-action', 'B-search_text', 'I-search_text', 'I-search_text', 'O', 'O', 'O']], [1]),
    ('поехали из митино до тушино', 'route', [['B-route_type', 'O', 'B-what_from', 'O', 'B-what_to']], [1]),
    ('поехали до митино из тушино', 'route', [['B-route_type', 'O', 'B-what_to', 'O', 'B-what_from']], [1]),
    ('давай поговорим о чем-нибудь', 'gc', [['O', 'B-skill_id', 'O', 'B-request']], [1]),
    pytest.param(
        'давай поговорим о вечном', 'gc',
        [['O', 'B-skill_id', 'O', 'B-request'], ['O', 'B-skill_id', 'I-skill_id', 'I-skill_id']], [1, 1],
        marks=pytest.mark.skip()),
    ('включи смешные видео про котиков', 'video',
     [['B-action', 'B-search_text', 'B-content_type', 'B-search_text', 'I-search_text']], [1]),
    ('включи music', 'bad_regex', [['O', 'O']], [1]),
])
def test_regex_token_tagger(utt, intent, tags, score, features_extractor, samples_extractor):
    token_tagger = create_token_tagger(
        model='regex',
        source={
            'gc': [
                '^давай (?P<skill_id>поболтаем)$',
                '^давай (?P<skill_id>поговорим) о (?P<request>.*)$',
                '^давай (?P<skill_id>поговорим о вечном)$'
            ],
            'music': [
                '(.* )?(?P<action>включи) (?P<search_text>песню ((?!на яндекс музыке).)*)( на яндекс музыке)?$'
            ],
            'route': [
                '^(?P<route_type>(поехали|едем|поезжай)) (от|из) (?P<what_from>[а-я]+( [0-9]+)?) '
                '(до|в|к) (?P<what_to>[а-я]+( [0-9]+)?)$',
                '^(?P<route_type>(поехали|едем|поезжай)) (до|в|к) (?P<what_to>[а-я]+( [0-9]+)?) '
                '(от|из) (?P<what_from>[а-я]+( [0-9]+)?)$'
            ],
            'video': [
                '(?P<action>включи) (?P<search_text>смешные) (?P<content_type>видео) (?P<search_text>про котиков)'
            ],
            'bad_regex': [
                '.*(?P<search_text>mus).*'
            ]
        }
    )
    x = features_extractor(samples_extractor([utt]))
    y, p = token_tagger.predict(x, intent)

    assert y[0] == tags
    assert p[0] == score


@pytest.fixture(params=[
    None, {'granet_tagger': 'personal_assistant.scenarios.repeat_after_me'}
])
def req_info(request):
    return create_request(gen_uuid_for_tests(), experiments=request.param)


def test_granet_token_tagger(samples_extractor, features_extractor, req_info):
    token_tagger = create_token_tagger(model='granet')

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
    features = features_extractor([sample])

    tags, scores = token_tagger.predict(
        features, batch_samples=[sample], intent=form_name, req_info=req_info
    )

    tags, scores = tags[0], scores[0]
    if req_info.experiments['granet_tagger'] is not None:
        assert len(tags) == len(scores) == 1
        assert tags[0] == ['O', 'O', 'O', 'O', 'O', 'B-request']
        assert scores[0] == 1.
    else:
        assert len(tags) == len(scores) == 0


def test_granet_token_tagger_empty_granet_response(samples_extractor, features_extractor, req_info):
    token_tagger = create_token_tagger(model='granet')

    form_name = 'personal_assistant.scenarios.repeat_after_me'

    utterance = 'алиса повтори-ка за мной алиса'
    sample = samples_extractor([utterance])[0]
    sample.annotations['wizard'] = WizardAnnotation(markup={}, rules={})

    features = features_extractor([sample])

    tags, scores = token_tagger.predict(
        features, batch_samples=[sample], intent=form_name, req_info=req_info
    )

    tags, scores = tags[0], scores[0]
    assert len(tags) == len(scores) == 0


def test_align_sequences():
    alignment = align_sequences(['повтори-ка', 'за', 'мной', 'блин'], ['алиса', 'повтори', 'ка', 'за', 'мной', 'блин'])
    assert np.all(alignment == [-1, 3, 4, 5])

    alignment = align_sequences(['повтори', 'за', 'мной'], ['алиса', 'повтори', 'за', 'мной', 'алиса'])
    assert np.all(alignment == [1, 2, 3])

    alignment = align_sequences(['повтори', 'пожалуйста', 'за', 'мной'], ['повтори', 'за', 'мной'])
    assert np.all(alignment == [0, -1, 1, 2])

    alignment = align_sequences(['повтори', 'пожалуйста', 'за', 'мной'], ['алиса', 'включи'])
    assert np.all(alignment == [-1, -1, -1, -1])

    alignment = align_sequences([], ['повтори', 'за', 'мной'])
    assert np.all(alignment == [])

    alignment = align_sequences(['повтори', 'пожалуйста', 'за', 'мной'], [])
    assert np.all(alignment == [-1, -1, -1, -1])
    alignment = align_sequences([], [])
    assert np.all(alignment == [])
