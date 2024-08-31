# coding: utf-8
from __future__ import unicode_literals

import pytest

from vins_core.nlu.token_classifier import create_token_classifier
from vins_core.nlu.features_extractor import create_features_extractor
from vins_core.common.sample import Sample
from vins_core.nlu.features.base import SampleFeatures


@pytest.fixture(scope='module')
def feature_extractor():
    return create_features_extractor(
        ngrams={'n': 1},
        ner={'parser': 'general_ru_base'},
        postag={},
        lemma={},
        case={}
    )


def combine_scores_classifier_config():
    params = {
        "method": "multiply",
        "classifiers": [
            {
                "model": "data_lookup",
                "name": "cls1",
                "params": {
                    "regexp": True,
                    "matching_score": 0.8,
                    "intent_texts": {
                        "intent1": [
                            "intent text"
                        ]
                    }
                }
            },
            {
                "model": "data_lookup",
                "name": "cls2",
                "params": {
                    "regexp": True,
                    "matching_score": 0.7,
                    "intent_texts": {
                        "intent1": [
                            "intent text"
                        ]
                    }
                }
            }
        ]
    }
    return params


def combine_scores_classifier_3_config():
    params = {
        "method": "multiply",
        "classifiers": [
            {
                "model": "data_lookup",
                "name": "cls1",
                "params": {
                    "regexp": True,
                    "matching_score": 0.8,
                    "intent_texts": {
                        "intent1": [
                            "intent text"
                        ]
                    }
                }
            },
            {
                "model": "data_lookup",
                "name": "cls2",
                "params": {
                    "regexp": True,
                    "matching_score": 0.7,
                    "intent_texts": {
                        "intent1": [
                            "intent text"
                        ]
                    }
                }
            },
            {
                "model": "data_lookup",
                "name": "cls3",
                "params": {
                    "regexp": True,
                    "matching_score": 0.6,
                    "intent_texts": {
                        "intent1": [
                            "intent text"
                        ]
                    }
                }
            }
        ]
    }
    return params


def _clf_on_text(clf, text):
    return clf(SampleFeatures(Sample.from_string(text)))


@pytest.mark.parametrize('config, method, result', (
    (combine_scores_classifier_config(), 'multiply', 0.8 * 0.7),
    (combine_scores_classifier_3_config(), 'multiply', 0.8 * 0.7 * 0.6),
    (combine_scores_classifier_config(), 'best_score', 0.8),
    (combine_scores_classifier_3_config(), 'best_score', 0.8)
))
def test_combine_scores_multiply_scores(config, method, result):
    config['method'] = method
    intent_infos = {intent: [None] for clf in config['classifiers'] for intent in clf['params']['intent_texts'].keys()}
    classifier = create_token_classifier(model='combine_scores', intent_infos=intent_infos, **config)
    scores = _clf_on_text(classifier, 'intent text')
    assert 'intent1' in scores
    assert scores['intent1'] == pytest.approx(result)
