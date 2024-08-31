# -*- coding: utf-8 -*-

import logging

from vins_core.nlu.features.extractor.base import (
    BaseFeatureExtractor, SparseFeatures, SparseFeatureValue
)

logger = logging.getLogger(__name__)


class GranetFeatureExtractor(BaseFeatureExtractor):
    DEFAULT_NAME = 'granet'

    def _call(self, sample, **kwargs):
        if 'wizard' not in sample.annotations or 'Granet' not in sample.annotations['wizard'].rules:
            return self.get_default_features(sample)

        granet_response = sample.annotations['wizard'].rules['Granet']
        forms = granet_response.get('Forms', {})

        return [SparseFeatures([SparseFeatureValue(form['Name']) for form in forms])]

    def get_default_features(self, sample=None):
        if sample is None:
            raise ValueError(
                'Unable to get default feature size: %s requires "sample" arg in get_default_features() method',
                self.__class__.__name__
            )
        return [SparseFeatures([])]

    @property
    def _features_cls(self):
        return SparseFeatures
