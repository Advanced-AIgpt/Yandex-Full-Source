# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from vins_core.nlu.anaphora.features.basic import BasicFeatureExtractor
from vins_core.nlu.anaphora.features.phrase_level import PhraseLevelFeaturesExtractor
from vins_core.nlu.anaphora.matcher.rule_based import SimpleAnaphoraMatcher
from vins_core.nlu.anaphora.matcher.catboost_based import CatboostAnaphoraMatcher
from vins_core.nlu.anaphora.resolver import AnaphoraResolver

logger = logging.getLogger(__name__)


class AnaphoraResolverFactory(object):
    def __init__(self):
        self._matchers = {
            'simple': SimpleAnaphoraMatcher,
            'catboost': CatboostAnaphoraMatcher
        }
        self._feature_extractors = {
            'simple': [
                {
                    'type': BasicFeatureExtractor,
                    'params': {}
                }
            ],
            'catboost': [
                {
                    'type': BasicFeatureExtractor,
                    'params': {}
                },
                {
                    'type': PhraseLevelFeaturesExtractor,
                    'params': {}
                }
            ]
        }
        self._samples_extractor = None

    def set_samples_extractor(self, samples_extractor):
        self._samples_extractor = samples_extractor

    def create_resolver(self, params):
        matcher_params = params.get('matcher', {})
        matcher_name = matcher_params.get('name')
        if matcher_name not in self._matchers:
            raise ValueError('Unknown anaphora model name {}'.format(matcher_name))
        feature_extractors = self._create_feature_extractors(matcher_name)
        matcher = self._create_matcher(matcher_name, feature_extractors, matcher_params)

        resolver_params = params.get('resolver', {})
        return AnaphoraResolver(
            matcher=matcher,
            samples_extractor=self._samples_extractor,
            **resolver_params
        )

    def _create_matcher(self, model_name, feature_extractors, params):
        params['feature_extractors'] = feature_extractors
        return self._matchers[model_name](**params)

    def _create_feature_extractors(self, model_name):
        return [extractor['type'](**extractor['params']) for extractor in self._feature_extractors[model_name]]
