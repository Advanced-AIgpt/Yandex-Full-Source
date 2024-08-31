# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import catboost
import itertools
import logging

from vins_core.nlu.anaphora.matcher.base import BaseAnaphoraMatcher
from vins_core.nlu.anaphora.mention import Match
from vins_core.utils.data import get_resource_full_path


logger = logging.getLogger(__name__)


class CatboostModel(object):
    def __init__(self, resource_id, prediction_type):
        self._model = catboost.CatBoostClassifier(thread_count=1)
        model_path = self._load_resources(resource_id)
        self._model.load_model(model_path)
        self._prediction_type = prediction_type

    def apply_model(self, features):
        if not features:
            return []
        predictions = self._model.predict(features, prediction_type=str(self._prediction_type))

        if self._prediction_type == 'Probability':
            predictions = predictions[:, 1]
        return predictions

    def _load_resources(self, model_id):
        model_path = get_resource_full_path(model_id)
        logger.info('Catboost_model file downloaded to %s', model_path)
        return model_path


class CatboostAnaphoraMatcher(BaseAnaphoraMatcher):

    def __init__(self, rank_border, border, rank_model_id, border_model_id, feature_extractors, **kwargs):
        super(CatboostAnaphoraMatcher, self).__init__(**kwargs)
        self._feature_extractors = feature_extractors
        self._rank_border = rank_border
        self._border = border
        self._rank_model = CatboostModel(rank_model_id, b'RawFormulaVal')
        self._border_model = CatboostModel(border_model_id, b'Probability')

    def match(self, anaphoric_context):
        features = self._extract_features(anaphoric_context)
        catboost_ready_features = self._reshape_features_to_matrix(features)
        antecedents = itertools.chain(*anaphoric_context.antecedents)

        border_scores = self._border_model.apply_model(catboost_ready_features)
        rank_scores = self._rank_model.apply_model(catboost_ready_features)
        matches = []
        for antecedent, border_score, rank_score in zip(antecedents, border_scores, rank_scores):
            logger.debug('For %s got %s border score, %f rank score', antecedent.text, border_score, rank_score)
            if self._validate_scores(border_score, rank_score):
                matches.append(Match(
                    anaphor=anaphoric_context.anaphor,
                    antecedent=antecedent,
                    score=rank_score
                ))
        return matches

    def _validate_scores(self, border_score, rank_score):
        return border_score >= self._border and rank_score >= self._rank_border

    @staticmethod
    def _reshape_features_to_matrix(features):
        catboost_ready_features = []
        for match_features in itertools.chain(*features):
            catboost_ready_features.append([match_features[x] for x in match_features])
        return catboost_ready_features
