# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from itertools import izip

from vins_core.nlu.features.post_processor.base import BaseFeaturesPostProcessor
from vins_core.nlu.features.extractor.base import SparseFeatureValue


class SplicerFeaturesPostProcessor(BaseFeaturesPostProcessor):
    """
    Expands contextual features for any current sparse sequential features
    Feature names are augmented with (t=±k) strings
    """
    def __init__(self, window_size=1):
        """
        :param window_size: target context size, k = -window_size/2,...,window_size/2
        :return:
        """
        super(SplicerFeaturesPostProcessor, self).__init__()
        self.window_size = window_size

    def _context_iter(self, feature_values_for_tokens):
        lc = self.window_size / 2
        rc = lc if self.window_size % 2 == 0 else lc + 1
        ntokens = len(feature_values_for_tokens) if feature_values_for_tokens else 0
        for i in xrange(ntokens):
            start = max(i - lc, 0)
            end = min(i + rc, len(feature_values_for_tokens))
            context = feature_values_for_tokens[start:end]
            values = []
            for feature_values, t in izip(context, xrange(start, end)):
                for feature_value in feature_values:
                    weight = feature_value.weight
                    values.append(SparseFeatureValue('%s(t=%d)' % (feature_value.value, t - i), weight))
            yield values

    def _call(self, batch_features, **kwargs):
        out = []
        for sample_features in batch_features:
            new_features = sample_features.copy()
            for feature_name, list_of_feature_values in sample_features.sparse_seq.iteritems():
                new_features.sparse_seq[feature_name] = list(self._context_iter(list_of_feature_values))
            out.append(new_features)
        return out
