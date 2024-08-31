# coding: utf-8

from __future__ import unicode_literals

import logging

import attr
import numpy as np

from vins_core.common.sample import Sample
from vins_core.schema import features_pb2
from vins_core.schema.interface import PbSerializable
from vins_core.utils.strings import smart_utf8, smart_unicode


logger = logging.getLogger(__name__)


class Features(PbSerializable):
    data = None


@attr.s(frozen=True)
class EmptyFeatures(Features):
    _PB_CLS = features_pb2.EmptyFeatures

    def to_pb(self):
        return features_pb2.EmptyFeatures()

    @classmethod
    def from_pb(cls, pb_obj):
        return cls()


@attr.s(frozen=True, repr=False, cmp=True)
class SparseFeatureValue(PbSerializable):
    _PB_CLS = features_pb2.SparseFeatureValue

    value = attr.ib()
    weight = attr.ib(default=1.0, converter=float)

    def __repr__(self):
        return smart_utf8(
            "'%s:%s'" % (self.value, self.weight)
            if self.weight != 1.0 else "'%s'" % self.value
        )

    def __unicode__(self):
        return smart_unicode(self.__repr__())

    def __str__(self):
        return self.__repr__()

    @value.validator
    def value_check(self, attribute, value):
        assert isinstance(value, basestring)

    @weight.validator
    def weight_check(self, attribute, value):
        assert isinstance(value, float)

    def to_pb(self):
        feature_value_obj = features_pb2.SparseFeatureValue()
        feature_value_obj.value = smart_utf8(self.value)
        feature_value_obj.weight = self.weight
        return feature_value_obj

    @classmethod
    def from_pb(cls, pb_obj):
        return cls(value=smart_unicode(pb_obj.value), weight=pb_obj.weight)


@attr.s(frozen=True)
class SparseFeatures(Features):
    _PB_CLS = features_pb2.SparseFeatures

    data = attr.ib()

    @data.validator
    def check(self, attribute, value):
        assert isinstance(value, list)
        assert all(isinstance(item, SparseFeatureValue) for item in value)

    def to_pb(self):
        pb_data = [sfv.to_pb() for sfv in self.data]
        return features_pb2.SparseFeatures(data=pb_data)

    @classmethod
    def from_pb(cls, pb_obj):
        data = [SparseFeatureValue.from_pb(sfv) for sfv in pb_obj.data]
        return cls(data=data)


@attr.s(frozen=True)
class SparseSeqFeatures(Features):
    _PB_CLS = features_pb2.SparseSeqFeatures

    data = attr.ib()

    @data.validator
    def check(self, attribute, value):
        assert isinstance(value, list)
        for token_feature_values in value:
            assert isinstance(token_feature_values, list)
            assert all(isinstance(value, SparseFeatureValue) for value in token_feature_values)

    def to_pb(self):
        pb_data = [SparseFeatures(data=sf_data).to_pb() for sf_data in self.data]
        return features_pb2.SparseSeqFeatures(data=pb_data)

    @classmethod
    def from_pb(cls, pb_obj):
        data = [SparseFeatures.from_pb(sf).data for sf in pb_obj.data]
        return cls(data=data)


def _ensure_float32(data):
    if data.dtype != np.float32:
        data = data.astype(np.float32)
    return data


@attr.s(frozen=True)
class DenseFeatures(Features):
    _PB_CLS = features_pb2.DenseFeatures

    data = attr.ib(converter=_ensure_float32)

    @data.validator
    def check(self, attribute, value):
        assert isinstance(value, np.ndarray)
        assert value.ndim == 1

    def to_pb(self):
        return features_pb2.DenseFeatures(data=self.data.tolist())

    @classmethod
    def from_pb(cls, pb_obj):
        return cls(data=np.array(pb_obj.data))


@attr.s(frozen=True)
class DenseSeqFeatures(Features):
    _PB_CLS = features_pb2.DenseSeqFeatures

    data = attr.ib(converter=_ensure_float32)

    @data.validator
    def check(self, attribute, value):
        assert isinstance(value, np.ndarray)
        assert value.ndim == 2

    def to_pb(self):
        feature_obj = features_pb2.DenseSeqFeatures()
        shape_x, shape_y = self.data.shape
        feature_obj.shape_x = shape_x
        feature_obj.shape_y = shape_y
        feature_obj.data.extend(self.data.flatten())
        return feature_obj

    @classmethod
    def from_pb(cls, pb_obj):
        x = pb_obj.shape_x
        arr = np.reshape(pb_obj.data, (x, -1))
        return cls(data=arr)


@attr.s(frozen=True)
class DenseSeqIdFeatures(Features):
    _PB_CLS = features_pb2.DenseSeqIdFeatures

    data = attr.ib()

    @data.validator
    def check(self, attribute, value):
        assert isinstance(value, list)

    def to_pb(self):
        return features_pb2.DenseSeqIdFeatures(data=self.data)

    @classmethod
    def from_pb(cls, pb_obj):
        return cls(data=list(pb_obj.data))


class BaseFeatureExtractor(object):
    """
    Features extractor takes one input Sample
    and returns list of TokenFeatures objects
    """
    def __init__(self):
        self._info = None
        self.label = self.__class__.__name__

    def __call__(self, sample, **kwargs):
        self._validate_input(sample)
        features = self._call(sample, **kwargs)
        self._validate_output(features)
        return features

    def _call(self, sample, **kwargs):
        raise NotImplementedError()

    def _validate_input(self, sample):
        assert isinstance(sample, Sample)

    def _validate_output(self, features):
        assert isinstance(features, list)
        assert all(isinstance(feature, self.features_clss + (EmptyFeatures,)) for feature in features)

    @property
    def info(self):
        return self._info

    def get_default_features(self, sample=None):
        return [EmptyFeatures()]

    def get_classifiers_info(self):
        return {}

    @property
    def _features_cls(self):
        raise NotImplementedError

    @property
    def features_clss(self):
        return self._features_cls,
