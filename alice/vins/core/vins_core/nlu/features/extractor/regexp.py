# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import re

from collections import Mapping

from vins_core.utils.data import load_data_from_file
from vins_core.nlu.features.extractor.base import (
    BaseFeatureExtractor, SparseSeqFeatures, SparseFeatureValue
)


class RegexpFeatureExtractor(BaseFeatureExtractor):

    DEFAULT_NAME = 'regexp'

    def __init__(self, data_or_filepath):
        """
        :param data_or_filepath: dict-like data {feature_name: regexp} or path to json-formatted file
        :return:
        """
        super(RegexpFeatureExtractor, self).__init__()
        data = self._load_data(data_or_filepath)
        self._patterns = {feature: re.compile(pattern) for feature, pattern in data.iteritems()}

    def _load_data(self, data_or_filepath):
        if isinstance(data_or_filepath, basestring):
            return load_data_from_file(data_or_filepath)
        elif isinstance(data_or_filepath, Mapping):
            return data_or_filepath

    def _call(self, sample, **kwargs):
        idx_to_token = {}
        curr_index = 0
        out = [[] for _ in xrange(len(sample))]
        for tok_idx, tok in enumerate(sample.tokens):
            for char_idx in xrange(len(tok)):
                idx_to_token[curr_index + char_idx] = tok_idx
            curr_index += len(tok) + 1

        for feature, pattern in self._patterns.iteritems():
            for match in pattern.finditer(sample.text):
                prefix_bio = 'B-re:'
                for t in xrange(idx_to_token[match.start()], idx_to_token[match.end() - 1] + 1):
                    value = prefix_bio + feature
                    out[t].append(SparseFeatureValue(value))
                    prefix_bio = 'I-re:'

        return [SparseSeqFeatures(out)]

    @property
    def _features_cls(self):
        return SparseSeqFeatures
