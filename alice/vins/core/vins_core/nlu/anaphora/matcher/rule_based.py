# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from vins_core.nlu.anaphora.matcher.base import BaseAnaphoraMatcher
from vins_core.nlu.anaphora.mention import Mention, Match

logger = logging.getLogger(__name__)


class SimpleAnaphoraMatcher(BaseAnaphoraMatcher):
    def __init__(self, feature_extractors, **kwargs):
        super(SimpleAnaphoraMatcher, self).__init__(**kwargs)
        self._feature_extractors = feature_extractors

    def match(self, anaphoric_context):
        features = self._extract_features(anaphoric_context)

        matches = []
        for phrase_antecedents, phrase_features in zip(anaphoric_context.antecedents, features):
            for antecedent, antecedent_features in zip(phrase_antecedents, phrase_features):
                if not antecedent_features['is_rule_based_condition']:
                    continue
                score = 1
                if antecedent.type == Mention.NOUN_PHRASE_TYPE:
                    score = 2
                if antecedent_features['geo_advpro']:
                    score += 2
                if antecedent_features['ant_animate']:
                    score += 2.5
                if antecedent_features['antecedent_source'] == Mention.ES_SOURCE_TYPE:
                    score *= 1.5
                score += antecedent_features['antecedent_word_cnt'] / 1.1
                matches.append(Match(
                    anaphor=anaphoric_context.anaphor,
                    antecedent=antecedent,
                    score=score
                ))
        return matches
