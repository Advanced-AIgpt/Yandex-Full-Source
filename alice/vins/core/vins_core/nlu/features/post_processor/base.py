# coding: utf-8
from __future__ import unicode_literals

from sklearn.base import BaseEstimator, TransformerMixin


class BaseFeaturesPostProcessor(BaseEstimator, TransformerMixin):
    """
    Post processors are sklearn-compatible transformers
    """
    def __init__(self):
        self._fit_params = {}

    def __call__(self, batch_features, **kwargs):
        return self._call(batch_features, **kwargs)

    def _call(self, batch_features, **kwargs):
        raise NotImplementedError()

    def fit(self, x, y=None, **kwargs):
        # TransformerMixin guarantees that transform() will be called immediately after fit()
        self._fit_params = kwargs
        return self

    def transform(self, batch_features):
        result = self(batch_features, **self._fit_params)
        self._fit_params = {}
        return result
