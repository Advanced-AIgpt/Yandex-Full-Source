# coding: utf-8
from __future__ import unicode_literals

from collections import Mapping
from vins_core.nlu.features.base import SampleFeatures


class Classifier(object):
    """
    General callable that gets input sample and returns distribution over some entities
    """
    def __init__(self, name=None, normalize=True, fallback_threshold=None, default_fallback_threshold=0,
                 fixlist_score=None, **kwargs):
        super(Classifier, self).__init__()
        self._name = name
        self._features = None
        self._need_normalize = normalize
        self._fallback_threshold = fallback_threshold or default_fallback_threshold
        self._fixlist_score = fixlist_score

    def __call__(self, feature, skip_classifiers=(), **kwargs):
        """
        :param feature: SampleFeatures instance
        :param skip_classifiers: set of classifier names, which predictions should be skipped
        :param kwargs: parameters passed to classifier on inference
        :return: pandas Series with index=labels, data=scores
        """
        if skip_classifiers and self._name in skip_classifiers:
            return {}

        assert feature is None or isinstance(feature, SampleFeatures)
        predictions = self._process(feature, skip_classifiers=skip_classifiers, **kwargs)
        if not self._validate(predictions):
            raise ValueError("Predictor %r returns wrong result" % self.__class__)
        return predictions

    def _process(self, features, **kwargs):
        raise NotImplementedError()

    @classmethod
    def _validate(cls, predictions):
        return isinstance(predictions, Mapping)

    def load(self, archive, name, **kwargs):
        raise NotImplementedError()

    def save(self, archive, name):
        raise NotImplementedError()

    @property
    def default_score(self):
        raise NotImplementedError()

    @property
    def classes(self):
        raise NotImplementedError()

    @property
    def features(self):
        return self._features

    @property
    def name(self):
        return self._name

    @name.setter
    def name(self, value):
        self._name = value

    @property
    def need_normalize(self):
        return self._need_normalize

    @property
    def fallback_threshold(self):
        return self._fallback_threshold

    @property
    def trainable_classifiers(self):
        return {self._name: self}

    def fixlist_score(self, skip_classifiers=()):
        return self._fixlist_score
