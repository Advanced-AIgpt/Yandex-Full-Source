import logging
import numpy as np

from vins_core.nlu.features.post_processor.base import BaseFeaturesPostProcessor
from vins_core.nlu.features.base import SampleFeatures


logger = logging.getLogger(__name__)


class StdNormalizerFeaturesPostProcessor(BaseFeaturesPostProcessor):

    def __init__(self, **kwargs):
        super(StdNormalizerFeaturesPostProcessor, self).__init__(**kwargs)

        self._dims = {}
        self._mean = {}
        self._std = {}

    def fit(self, batch_features, y=None, **kwargs):
        for sample_features in batch_features:
            for feature_name, feature in sample_features.dense.iteritems():
                if self._dims.get(feature_name) is None:
                    self._dims[feature_name] = len(feature)
                    self._mean[feature_name] = np.zeros(len(feature))
                    self._std[feature_name] = np.zeros(len(feature))
                else:
                    assert self._dims[feature_name] == len(feature), "Feature {} has width {} instead of {}".format(
                        feature_name, len(feature), self._dims[feature_name]
                    )
                self._mean[feature_name] += feature
                self._std[feature_name] += np.square(feature)
        n = float(len(batch_features))
        for feature_name in self._dims:
            self._mean[feature_name] /= n
            self._std[feature_name] = np.maximum(
                np.sqrt(self._std[feature_name] / n - np.square(self._mean[feature_name])), 1e-16
            )
            logger.debug('"%s" mean:\n%r', feature_name, self._mean[feature_name])
            logger.debug('"%s" std:\n%r', feature_name, self._std[feature_name])
        return self

    def transform(self, batch_features):
        out = []
        for sample_features in batch_features:
            normalized_dense = {}
            for feature_name, feature in sample_features.dense.iteritems():
                assert self._dims[feature_name] == len(feature)
                normalized_dense[feature_name] = (feature - self._mean[feature_name]) / self._std[feature_name]
            out.append(SampleFeatures(
                sample=sample_features.sample,
                sparse_seq=sample_features.sparse_seq,
                sparse=sample_features.sparse,
                dense_seq=sample_features.dense_seq,
                dense_seq_ids=sample_features.dense_seq_ids,
                dense=normalized_dense
            ))
        return out
