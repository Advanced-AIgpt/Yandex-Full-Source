# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from vins_core.nlu.features.extractor.base import (
    BaseFeatureExtractor, SparseSeqFeatures, SparseFeatureValue, DenseSeqFeatures
)
import numpy as np


class NerFeatureExtractor(BaseFeatureExtractor):
    """
    Encode all entities found by a NluFstParser as SparseSeqFeatures, and optionally as DenseSeqFeatures
    """

    DEFAULT_NAME = 'ner'

    def __init__(self, parser, dense_features=None):
        """
        :param parser: valid FST parser name
        :param dense_features: list of ner types that must be coded in a dense way
        """
        super(NerFeatureExtractor, self).__init__()
        self._dense_feature_indices = {feature: i for i, feature in enumerate(dense_features or [])}
        self._parser = parser
        self.label = self.DEFAULT_NAME + ':'
        if hasattr(self._parser, 'label'):
            self.label += self._parser.label
        else:
            self.label += self._parser.__class__.__name__

    def _call(self, sample, **kwargs):
        if 'ner' not in sample.annotations:
            objects = self._parser(sample, **kwargs)
        else:
            objects = sample.annotations['ner'].entities
        return self._get_sparse_features(sample, objects) + self._get_dense_features(sample, objects)

    def _get_sparse_features(self, sample, objects):
        out = [[] for _ in xrange(len(sample))]
        for ner in objects:
            prefix_bio = 'B-'
            for t in xrange(ner.start, ner.end):
                weight = ner.weight
                value = prefix_bio + ner.type
                if weight is None:
                    out[t].append(SparseFeatureValue(value))
                else:
                    out[t].append(SparseFeatureValue(value, weight))
                prefix_bio = 'I-'
        return [SparseSeqFeatures(out)]

    def _get_dense_features(self, sample, objects):
        if not self._dense_feature_indices:
            return []
        out = np.zeros([len(sample), len(self._dense_feature_indices)])
        for ner in objects:
            position = self._dense_feature_indices.get(ner.type)
            if position is not None:
                weight = ner.weight if ner.weight is not None else 1
                for t in xrange(ner.start, ner.end):
                    out[t, position] = weight
        return [DenseSeqFeatures(out)]

    @property
    def features_clss(self):
        if self._dense_feature_indices:
            return DenseSeqFeatures, SparseSeqFeatures
        else:
            return SparseSeqFeatures,
