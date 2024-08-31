# coding: utf-8
from __future__ import unicode_literals

import pytest

from vins_core.common.sample import Sample
from vins_core.dm.request import create_request
from vins_core.nlu.features.base import SampleFeatures
from vins_core.nlu.features_extractor import create_features_extractor
from vins_core.nlu.token_classifier import create_token_classifier
from vins_core.utils.misc import gen_uuid_for_tests


@pytest.fixture(scope='module')
def feature_extractor():
    return create_features_extractor(
        ngrams={'n': 1},
        ner={'parser': 'general_ru_base'},
        postag={},
        lemma={},
        case={}
    )


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


def _clf_on_text(clf, text, req_info=None):
    return clf(SampleFeatures(Sample.from_string(text)), req_info=req_info)


@pytest.mark.parametrize('matching_score, semantic_frames, result', (
    (1, semantic_frame_with_one_intent(), {'personal_assistant.scenarios.search': 1}),
    (1, semantic_frame_with_two_intent(), {
        'personal_assistant.scenarios.search': 1, 'personal_assistant.scenarios.web': 1
    }),
    (1, {}, dict()),
    (0.8, semantic_frame_with_one_intent(), {'personal_assistant.scenarios.search': 0.8}),
    (0.8, semantic_frame_with_two_intent(), {
        'personal_assistant.scenarios.search': 0.8, 'personal_assistant.scenarios.web': 0.8
    }),
    (0, semantic_frame_with_one_intent(), {'personal_assistant.scenarios.search': 0}),
    (0, semantic_frame_with_two_intent(), {
        'personal_assistant.scenarios.search': 0, 'personal_assistant.scenarios.web': 0
    }),
))
def test_protocol_semantic_frame_classifier(matching_score, semantic_frames, result):
    classifier = create_token_classifier(model='protocol_semantic_frame', matching_score=matching_score)
    req_info = create_request(uuid=gen_uuid_for_tests(), utterance='intent text', semantic_frames=semantic_frames)

    scores = _clf_on_text(classifier, 'intent text', req_info=req_info)
    assert scores == result
