# coding: utf-8

from alice.nlu.py_libs.utils.sample import Sample

import attr
import numpy as np


@attr.s
class SparseFeatureWrapper(object):
    value = attr.ib()
    weight = attr.ib()


@attr.s
class SampleFeaturesWrapper(object):
    sample = attr.ib()
    dense_seq = attr.ib()
    sparse_seq = attr.ib()

    @classmethod
    def from_json(cls, sample_features):
        sample = Sample(tokens=sample_features['sample']['tokens'], tags=sample_features['sample'].get('tags', []))

        dense_seq = {
            feature['key']: cls._convert_to_matrix(feature['value'])
            for feature in sample_features['dense_seq_features']
        }
        token_count = len(sample_features['sample']['tokens'])
        sparse_seq = {
            feature['key']: cls._convert_sparse_feature(feature['value'], token_count)
            for feature in sample_features['sparse_seq_features']
        }
        return cls(sample=sample, dense_seq=dense_seq, sparse_seq=sparse_seq)

    @staticmethod
    def _convert_to_matrix(feature_value):
        return np.array(feature_value['data']).reshape(feature_value['shape_x'], feature_value['shape_y'])

    @staticmethod
    def _convert_sparse_feature(data, token_count):
        if not data['data']:
            return [[] for _ in range(token_count)]

        assert len(data['data']) == token_count
        return [
            [SparseFeatureWrapper(**val) for val in feature_list.get('data', [])]
            for feature_list in data['data']
        ]

    def __len__(self):
        return len(self.sample.tokens)

    def as_dict(self):
        return {
            'sample': attr.asdict(self.sample),
            'dense_seq': {k: v.tolist() for k, v in self.dense_seq.items()},
            'sparse_seq': {k: [[attr.asdict(y) for y in x] for x in v] for k, v in self.sparse_seq.items()}
        }
