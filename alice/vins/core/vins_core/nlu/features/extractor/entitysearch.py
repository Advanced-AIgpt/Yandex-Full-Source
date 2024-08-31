# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from vins_core.nlu.features.extractor.base import BaseFeatureExtractor, SparseFeatures, SparseFeatureValue

logger = logging.getLogger(__name__)


class EntitySearchFeatureExtractor(BaseFeatureExtractor):
    DEFAULT_NAME = 'entitysearch'

    def _call(self, sample, **kwargs):
        if 'entitysearch' not in sample.annotations:
            return [SparseFeatures([])]

        entity_features = sample.annotations['entitysearch'].entity_features

        sparse_feature_list = []
        self._add_sparse_features(entity_features.tags, 'tag', sparse_feature_list)
        self._add_sparse_features(entity_features.site_ids, 'id', sparse_feature_list)
        self._add_sparse_features(entity_features.types, 'type', sparse_feature_list)
        self._add_sparse_features(entity_features.subtypes, 'subtype', sparse_feature_list)

        if entity_features.has_music_info:
            sparse_feature_list.append(SparseFeatureValue('music_info'))

        return [SparseFeatures(sparse_feature_list)]

    @staticmethod
    def _add_sparse_features(features, type, output):
        for feature in features:
            output.append(SparseFeatureValue(type + '_' + feature))

    @property
    def _features_cls(self):
        return SparseFeatures
