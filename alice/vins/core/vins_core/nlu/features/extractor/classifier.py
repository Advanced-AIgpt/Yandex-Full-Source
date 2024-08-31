# coding: utf-8
from __future__ import unicode_literals

import numpy as np

from operator import itemgetter

from vins_core.nlu.features.extractor.base import (
    BaseFeatureExtractor, SparseFeatures, SparseFeatureValue, DenseFeatures
)
from vins_core.nlu.features.base import SampleFeatures


class ClassifierFeatureExtractor(BaseFeatureExtractor):

    _FEATURES = {
        'label': SparseFeatures,
        'scores': DenseFeatures,
        'rankings': SparseFeatures
    }

    def __init__(self, token_classifier, feature='label', **params):
        assert feature in self._FEATURES
        super(ClassifierFeatureExtractor, self).__init__()
        self._token_classifier = token_classifier
        self._feature = feature
        self._info = list(self._token_classifier.classes)

    @property
    def feature(self):
        return self._feature

    @feature.setter
    def feature(self, value):
        assert value in self._FEATURES
        self._feature = value

    def _call(self, sample, **kwargs):
        prediction = self._token_classifier(SampleFeatures(sample))
        if self._feature == 'label':
            return [SparseFeatures([SparseFeatureValue(max(prediction, key=prediction.get))])]
        elif self._feature == 'scores':
            return [DenseFeatures(np.array([prediction[label] for label in self._info]))]
        elif self._feature == 'rankings':
            return [SparseFeatures([
                SparseFeatureValue(
                    '>'.join(map(itemgetter(0), sorted(prediction.iteritems(), key=itemgetter(1), reverse=True)))
                )
            ])]

    @property
    def _features_cls(self):
        return self._FEATURES[self.feature]
