# coding: utf-8
from __future__ import unicode_literals

import pytest

from vins_core.common.entity import Entity
from vins_core.dm.request import create_request
from vins_core.nlu.features_extractor import FeaturesExtractorFactory
from vins_core.nlu.token_tagger import create_token_tagger
from vins_core.utils.misc import gen_uuid_for_tests


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


def semantic_frame_with_one_intent():
    return [{
        'slots': [{
            'type': 'string',
            'value': 'какое-то значение',
            'name': 'query',
        }, {
            'type': 'site',
            'value': {
                'serp': {
                    'url': 'значение',
                },
            },
            'name': 'search_results',
        }, {
            'type': 'string',
            'value': {
                'data': {
                    'ключ': 'значение',
                }
            },
            'name': 'unicode_data',
        }, {
            'type': 'num',
            'value': 42,
            'name': 'number',
        }],
        'name': 'personal_assistant.scenarios.search',
    }]


def semantic_frame_with_two_intent():
    return [{
        'slots': [{
            'type': 'string',
            'value': 'какое-то значение',
            'name': 'query',
        }, {
            'type': 'site',
            'value': {
                'serp': {
                    'url': 'значение',
                },
            },
            'name': 'search_results',
        }, {
            'type': 'string',
            'value': {
                'data': {
                    'ключ': 'значение',
                }
            },
            'name': 'unicode_data',
        }, {
            'type': 'num',
            'value': 42,
            'name': 'number',
        }],
        'name': 'personal_assistant.scenarios.search',
    }, {
        'slots': [{
            'type': 'string',
            'value': 'какое-то значение',
            'name': 'query',
        }, {
            'type': 'site',
            'value': {
                'serp': {
                    'url': 'значение',
                },
            },
            'name': 'search_results',
        }, {
            'type': 'string',
            'value': {
                'data': {
                    'ключ': 'значение',
                }
            },
            'name': 'unicode_data',
        }, {
            'type': 'num',
            'value': 42,
            'name': 'number',
        }],
        'name': 'personal_assistant.scenarios.web',
    }]


@pytest.mark.parametrize('utt, intent, semantic_frames, slots, entities, scores', [
    (
        'some text',
        'personal_assistant.scenarios.search',
        semantic_frame_with_one_intent(),
        [[{
            'unicode_data': [{
                'start': 0,
                'end': 1,
                'entities': [
                    Entity(start=0, end=1, type='string', value={'data': {'ключ': 'значение'}}, substr='', weight=None),
                ],
                'substr': '',
                'is_continuation': False,
            }],
            'query': [{
                'start': 0,
                'end': 1,
                'entities': [Entity(start=0, end=1, type='string', value='какое-то значение', substr='', weight=None)],
                'substr': 'какое-то значение',
                'is_continuation': False,
            }],
            'search_results': [{
                'start': 0,
                'end': 1,
                'entities': [
                    Entity(start=0, end=1, type='site', value={'serp': {'url': 'значение'}}, substr='', weight=None),
                ],
                'substr': '',
                'is_continuation': False,
            }],
            'number': [{
                'start': 0,
                'end': 1,
                'entities': [Entity(start=0, end=1, type='num', value=42, substr='', weight=None)],
                'substr': '',
                'is_continuation': False,
            }]
        }]],
        [[[]]],
        [[1]],
    ),
    (
        'some text',
        'personal_assistant.scenarios.web',
        semantic_frame_with_one_intent(),
        [],
        [],
        [],
    ),
    (
        'some text',
        'personal_assistant.scenarios.web',
        semantic_frame_with_two_intent(),
        [[{
            'unicode_data': [{
                'start': 0,
                'end': 1,
                'entities': [
                    Entity(start=0, end=1, type='string', value={'data': {'ключ': 'значение'}}, substr='', weight=None)
                ],
                'substr': '',
                'is_continuation': False,
            }],
            'query': [{
                'start': 0,
                'end': 1,
                'entities': [Entity(start=0, end=1, type='string', value='какое-то значение', substr='', weight=None)],
                'substr': 'какое-то значение',
                'is_continuation': False,
            }],
            'search_results': [{
                'start': 0,
                'end': 1,
                'entities': [
                    Entity(start=0, end=1, type='site', value={'serp': {'url': 'значение'}}, substr='', weight=None)
                ],
                'substr': '',
                'is_continuation': False,
            }],
            'number': [{
                'start': 0,
                'end': 1,
                'entities': [Entity(start=0, end=1, type='num', value=42, substr='', weight=None)],
                'substr': '',
                'is_continuation': False,
            }]
        }]],
        [[[]]],
        [[1]],
    ),
    (
        'some text',
        'personal_assistant.scenarios.web',
        [],
        [],
        [],
        [],
    ),
    (
        'some text',
        'personal_assistant.scenarios.web',
        [{
            'slots': [],
            'name': 'personal_assistant.scenarios.web',
        }],
        [[{}]],
        [[[]]],
        [[1]],
    ),
])
def test_regex_token_tagger(utt, intent, semantic_frames, slots, entities, scores,
                            features_extractor, samples_extractor):
    token_tagger = create_token_tagger(model='protocol_semantic_frame', matching_score=1)

    req_info = create_request(uuid=gen_uuid_for_tests(), utterance='intent text', semantic_frames=semantic_frames)
    samples = samples_extractor([utt])
    features = features_extractor(samples)

    batch_slots_list, batch_entities_list, batch_score_list = token_tagger.predict_slots(
        batch_samples=samples, batch_features=features, batch_entities=[], intent=intent, req_info=req_info,
    )

    assert batch_slots_list == slots
    assert batch_entities_list == entities
    assert batch_score_list == scores
