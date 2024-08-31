# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import logging
import numpy as np
import attr

from copy import deepcopy
from collections import OrderedDict
from enum import Enum

from vins_core.common.sample import Sample
from vins_core.nlu.features.extractor.base import (
    SparseFeatures, SparseSeqFeatures, DenseFeatures,
    DenseSeqFeatures, EmptyFeatures, DenseSeqIdFeatures
)
from vins_core.schema import features_pb2
from vins_core.schema.interface import PbSerializable
from vins_core.utils.strings import smart_utf8, smart_unicode, fix_protobuf_string


logger = logging.getLogger(__name__)


class Features(Enum):
    SPARSE = 'sparse'
    SPARSE_SEQ = 'sparse_seq'
    DENSE = 'dense'
    DENSE_SEQ = 'dense_seq'
    DENSE_SEQ_IDS = 'dense_seq_ids'


FEATURE_CLASS_TO_NAME = {
    SparseFeatures: Features.SPARSE,
    SparseSeqFeatures: Features.SPARSE_SEQ,
    DenseFeatures: Features.DENSE,
    DenseSeqFeatures: Features.DENSE_SEQ,
    DenseSeqIdFeatures: Features.DENSE_SEQ_IDS
}


@attr.s
class FeatureExtractorResult(object):
    item = attr.ib()
    sample_features = attr.ib()


@attr.s
class IntentScore(PbSerializable):
    _PB_CLS = features_pb2.IntentScore

    name = attr.ib(validator=attr.validators.instance_of(basestring))
    score = attr.ib(default=1.0)

    def to_dict(self):
        return attr.asdict(self)

    def to_pb(self):
        return features_pb2.IntentScore(name=self.name, score=self.score)

    @classmethod
    def from_pb(cls, obj):
        return cls(name=fix_protobuf_string(obj.name), score=obj.score)


@attr.s
class TaggerSlot(PbSerializable):
    _PB_CLS = features_pb2.TaggerSlot

    start = attr.ib()
    end = attr.ib()
    is_continuation = attr.ib()
    value = attr.ib()

    def to_dict(self):
        return attr.asdict(self)

    def to_pb(self):
        return features_pb2.TaggerSlot(
            start=self.start,
            end=self.end,
            is_continuation=self.is_continuation,
            value=self.value
        )

    @classmethod
    def from_pb(cls, obj):
        return cls(
            start=obj.start,
            end=obj.end,
            is_continuation=obj.is_continuation,
            value=obj.value
        )


@attr.s
class TaggerScore(PbSerializable):
    _PB_CLS = features_pb2.TaggerScore

    intent = attr.ib(validator=attr.validators.instance_of(basestring))
    score = attr.ib(default=0.0)
    slots = attr.ib(default=attr.Factory(list))

    def to_dict(self):
        return attr.asdict(self)

    def to_pb(self):
        return features_pb2.TaggerScore(
            intent=self.intent,
            score=self.score,
            slots=[slot.to_pb() for slot in self.slots]
        )

    @classmethod
    def from_pb(cls, obj):
        return cls(
            intent=obj.intent,
            score=obj.score,
            slots=[TaggerSlot.from_pb(slot) for slot in obj.slots]
        )


# TODO(dan-anastasev): Move req_info into the SampleFeatures class
class SampleFeatures(PbSerializable):
    _PB_CLS = features_pb2.SampleFeatures

    def __init__(self, sample, sparse=None, dense=None, sparse_seq=None, dense_seq=None, dense_seq_ids=None,
                 classification_scores=None, tagger_scores=None):
        """
        :param sample: Sample instance
        """

        self.sample = deepcopy(sample)
        # Remove all sample annotations in order to reduce disk & memory consumption.
        self.sample.annotations.clear()

        self._features = {
            Features.SPARSE: sparse or {},
            Features.SPARSE_SEQ: sparse_seq or {},
            Features.DENSE: dense or OrderedDict(),
            Features.DENSE_SEQ: dense_seq or OrderedDict(),
            Features.DENSE_SEQ_IDS: dense_seq_ids or {}
        }

        self.classification_scores = classification_scores or OrderedDict()
        self.tagger_scores = tagger_scores or OrderedDict()

    @staticmethod
    def from_features(sample, features):
        sample_features = SampleFeatures(sample)
        sample_features._features = features

        return sample_features

    def __len__(self):
        return len(self.sample)

    def empty(self):
        return not any(self._features.itervalues())

    def copy(self):
        return deepcopy(self)

    @property
    def sparse(self):
        return self._features[Features.SPARSE]

    @property
    def sparse_seq(self):
        return self._features[Features.SPARSE_SEQ]

    @property
    def dense(self):
        return self._features[Features.DENSE]

    @property
    def dense_seq(self):
        return self._features[Features.DENSE_SEQ]

    @property
    def dense_seq_ids(self):
        return self._features[Features.DENSE_SEQ_IDS]

    @property
    def features(self):
        return self._features

    @staticmethod
    def _dense_matrix(input_dict, keys_order=None):
        if len(input_dict) == 1:
            return input_dict.values()[0]
        elif len(input_dict) > 1:
            if not keys_order:
                return np.hstack(input_dict.itervalues())
            else:
                assert len(input_dict) >= len(keys_order)
                return np.hstack(input_dict[k] for k in keys_order)
        else:
            return np.array([])

    def get_dense_seq_ids(self):
        features_num = len(self.dense_seq_ids)

        if features_num == 1:
            return self.dense_seq_ids.values()[0]
        elif features_num > 1:
            raise NotImplementedError('Currently only single source of DenseSeqIdFeatures supported')
        else:
            return None

    def dense_seq_matrix(self, keys_order=None):
        return self._dense_matrix(self.dense_seq, keys_order)

    def dense_matrix(self, keys_order=None):
        return self._dense_matrix(self.dense, keys_order)

    def _add(self, features, feature_id):
        container = self._features[FEATURE_CLASS_TO_NAME[type(features)]]

        if feature_id in container:
            logger.warning('Duplicated feature id %s found, adding newer' % feature_id)

        container[feature_id] = features.data

    def add(self, features, feature_id):
        if isinstance(features, EmptyFeatures):
            return
        elif len(self) == 0:
            logger.warning('Trying to add non-empty features %r to the empty sample, skipped.', features)
            return
        elif isinstance(features, tuple(FEATURE_CLASS_TO_NAME.keys())):
            self._add(features, feature_id)
        else:
            raise ValueError(
                'Cannot add feature of type %r, only %s are supported' % (
                    type(features), ', '.join(map(str, FEATURE_CLASS_TO_NAME.keys()))
                )
            )

    def add_classification_scores(self, stage, scores):
        self.classification_scores[stage] = scores

    def add_tagger_scores(self, stage, scores):
        self.tagger_scores[stage] = scores

    def clear_scores(self):
        self.classification_scores = OrderedDict()
        self.tagger_scores = OrderedDict()

    def __repr__(self):
        out = b''
        out += b'Sample:\n' + self.sample.__repr__() + b'\n'
        if self.sparse_seq:
            out += b'\nSPARSE SEQ:\n'
            for feature_name, list_of_feature_values in self.sparse_seq.iteritems():
                strings_per_token = []
                for i, feature_values in enumerate(list_of_feature_values):
                    values_string = ', '.join(map(unicode, feature_values))
                    strings_per_token.append('%d: %s' % (i, values_string))
                out += smart_utf8('%s: %s\n' % (feature_name, '; '.join(strings_per_token)))
        if self.sparse:
            out += b'\nSPARSE:\n'
            for feature_name, feature_values in self.sparse.iteritems():
                values_string = ', '.join(map(unicode, feature_values))
                out += smart_utf8('%s: %s\n' % (feature_name, values_string))
        if self.dense_seq:
            out += b'\nDENSE SEQ:\n'
            for feature_name, features in self.dense_seq.iteritems():
                out += smart_utf8('%s: %s\n' % (feature_name, features))
        if self.dense:
            out += b'\nDENSE:\n'
            for feature_name, features in self.dense.iteritems():
                out += smart_utf8('%s: %s\n' % (feature_name, features))
        if self.dense_seq_ids:
            out += b'\nDENSE SEQ IDS:\n'
            for feature_name, features in self.dense_seq_ids.iteritems():
                out += smart_utf8('%s: %s\n' % (feature_name, features))
        return out

    def __unicode__(self):
        return smart_unicode(self.__repr__())

    def to_pb(self):
        def _features_dict_to_pb(features_dict, value_type):
            result = {}
            for feature_id, feature_data in features_dict.iteritems():
                result[feature_id] = value_type(data=feature_data).to_pb()
            return result if result else None

        sparse_features = _features_dict_to_pb(self.sparse, SparseFeatures)
        sparse_seq_features = _features_dict_to_pb(self.sparse_seq, SparseSeqFeatures)
        dense_features = _features_dict_to_pb(self.dense, DenseFeatures)
        dense_seq_features = _features_dict_to_pb(self.dense_seq, DenseSeqFeatures)
        dense_seq_id_features = _features_dict_to_pb(self.dense_seq_ids, DenseSeqIdFeatures)

        classification_scores = [
            features_pb2.ClassificationStage(name=stage, scores=[score.to_pb() for score in scores])
            for stage, scores in self.classification_scores.iteritems()
        ]

        tagger_scores = [
            features_pb2.TaggerStage(name=stage, scores=[score.to_pb() for score in scores])
            for stage, scores in self.tagger_scores.iteritems()
        ]

        return features_pb2.SampleFeatures(
            sample=self.sample.to_pb(),
            sparse_features=sparse_features,
            sparse_seq_features=sparse_seq_features,
            dense_features=dense_features,
            dense_seq_features=dense_seq_features,
            dense_seq_id_features=dense_seq_id_features,
            classification_scores=classification_scores,
            tagger_scores=tagger_scores,
        )

    @classmethod
    def from_pb(cls, pb_obj):
        def _features_dict_from_pb(pb_dict, item_type):
            result = {}
            for feature_id, features_pb_data in dict(pb_dict).iteritems():
                result[fix_protobuf_string(feature_id)] = item_type.from_pb(features_pb_data).data
            return result if result else None

        sparse = _features_dict_from_pb(pb_obj.sparse_features, SparseFeatures)
        sparse_seq = _features_dict_from_pb(pb_obj.sparse_seq_features, SparseSeqFeatures)
        dense = _features_dict_from_pb(pb_obj.dense_features, DenseFeatures)
        dense_seq = _features_dict_from_pb(pb_obj.dense_seq_features, DenseSeqFeatures)
        dense_seq_ids = _features_dict_from_pb(pb_obj.dense_seq_id_features, DenseSeqIdFeatures)

        classification_scores = OrderedDict()
        for stage in pb_obj.classification_scores:
            classification_scores[fix_protobuf_string(stage.name)] = [IntentScore.from_pb(score) for score in stage.scores]

        tagger_scores = OrderedDict()
        for stage in pb_obj.tagger_scores:
            tagger_scores[fix_protobuf_string(stage.name)] = [TaggerScore.from_pb(score) for score in stage.scores]

        return cls(
            sample=Sample.from_pb(pb_obj.sample),
            sparse=sparse,
            sparse_seq=sparse_seq,
            dense=dense,
            dense_seq=dense_seq,
            dense_seq_ids=dense_seq_ids,
            classification_scores=classification_scores,
            tagger_scores=tagger_scores,
        )
