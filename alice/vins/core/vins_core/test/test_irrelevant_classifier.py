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


def _clf_on_text(clf, text, req_info=None):
    return clf(SampleFeatures(Sample.from_string(text)), req_info=req_info)


@pytest.mark.parametrize('scenario_id, matching_intent, matching_score, semantic_frames, result', (
    ('foo', 'bar', 1, [{'name': 'kek'}], {'bar': 1}),
    (None, 'bar', 1, [{'name': 'kek'}], dict()),
    ('foo', 'bar', 1, [], dict()),
    (None, 'bar', 1, [], dict()),
    ('foo', 'bar', 0.8, [{'name': 'bar'}], {'bar': 0.8}),
    (None, 'bar', 0.8, [{'name': 'bar'}], dict()),
    ('foo', 'bar', 0.8, [], dict()),
    (None, 'bar', 0.8, [], dict()),
    ('foo', 'bar', 0, [{'name': 'bar'}], {'bar': 0}),
    (None, 'bar', 0, [{'name': 'bar'}], dict()),
    ('foo', 'bar', 0, [], dict()),
    (None, 'bar', 0, [], dict()),
))
def test_protocol_semantic_frame_classifier(scenario_id, matching_score, matching_intent, semantic_frames, result):
    classifier = create_token_classifier(
        model='irrelevant',
        matching_intent=matching_intent,
        matching_score=matching_score,
    )

    req_info = create_request(
        uuid=gen_uuid_for_tests(),
        utterance='intent text',
        semantic_frames=semantic_frames,
        scenario_id=scenario_id,
    )

    scores = _clf_on_text(classifier, 'intent text', req_info=req_info)
    assert scores == result
