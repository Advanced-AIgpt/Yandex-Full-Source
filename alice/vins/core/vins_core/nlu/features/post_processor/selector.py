# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging
import random

from copy import deepcopy

from vins_core.nlu.features.base import SampleFeatures
from vins_core.nlu.features.post_processor.base import BaseFeaturesPostProcessor


logger = logging.getLogger(__name__)


class SelectorFeaturesPostProcessor(BaseFeaturesPostProcessor):
    """
    Selects specified subset of input features
    """
    def __init__(self, features, dropout_config=None, copy=False):
        super(SelectorFeaturesPostProcessor, self).__init__()
        self.features = set(features)
        self._dropout_cfg = dropout_config or {}
        self._copy = copy

    @staticmethod
    def _match_dropout(seq_feature_value, dropout_suffix):
        return seq_feature_value.value.lower().endswith(dropout_suffix)

    DROPOUT_MODE_SCAN = 0
    DROPOUT_MODE_DROP = 1
    DROPOUT_MODE_SKIP = 2

    @staticmethod
    def _collect_dropouts(sequence_of_feature_values, value_dropout_cfg):
        """
        This method iterates through the sequence of feature-value groups (each group represents
        one token in the input utterance) and decides which values must be dropped:
            1. For each value which ends with one of predefined suffixes the dropping attempt is
               made with certain probablity. Both suffixes and probabilities are defined in
               the value_dropout_cfg dictionary.
            2. If the dropping attempt is succesfull then each value which ends with the corresponding suffix
               and belong to the adjacent token is dropped too. This rule is transitive. Thus this
               method always drops continuous segments of feature values ending with the same suffix.
        Method returns a list of groups of suffixes (one group for each token) indicating which values must
        be actually dropped (this information is latter used by _apply_dropouts() method).
        """
        dropout_result = [[] for _ in xrange(len(sequence_of_feature_values))]

        for dropout_suffix, dropout_prob in value_dropout_cfg.items():
            i = 0
            mode = SelectorFeaturesPostProcessor.DROPOUT_MODE_SCAN

            while i < len(sequence_of_feature_values):
                feature_values = sequence_of_feature_values[i]
                value_found = any(
                    SelectorFeaturesPostProcessor._match_dropout(seq_feature_value, dropout_suffix)
                    for seq_feature_value in feature_values
                )

                if mode == SelectorFeaturesPostProcessor.DROPOUT_MODE_SCAN:
                    if value_found:
                        do_dropout = random.random() < dropout_prob
                        if do_dropout:
                            dropout_result[i].append(dropout_suffix)
                            mode = SelectorFeaturesPostProcessor.DROPOUT_MODE_DROP
                        else:
                            mode = SelectorFeaturesPostProcessor.DROPOUT_MODE_SKIP
                elif mode == SelectorFeaturesPostProcessor.DROPOUT_MODE_DROP:
                    if value_found:
                        dropout_result[i].append(dropout_suffix)
                    else:
                        mode = SelectorFeaturesPostProcessor.DROPOUT_MODE_SCAN
                elif mode == SelectorFeaturesPostProcessor.DROPOUT_MODE_SKIP:
                    if not value_found:
                        mode = SelectorFeaturesPostProcessor.DROPOUT_MODE_SCAN

                i += 1

        return dropout_result if any(x for x in dropout_result) else None

    @staticmethod
    def _apply_dropouts(sequence_of_feature_values, dropouts):
        result = []
        for feature_values, dropout_values in zip(sequence_of_feature_values, dropouts):
            new_feature_values = []
            for v in feature_values:
                if not any(
                    SelectorFeaturesPostProcessor._match_dropout(v, dropout_suffix)
                    for dropout_suffix in dropout_values
                ):
                    new_feature_values.append(v)
            result.append(new_feature_values)

        return result

    def _get_subdict(self, input_dict, feature_type, do_dropout):
        result = {}
        for key, value in input_dict.iteritems():
            if key not in self.features:
                continue

            new_value = None
            if do_dropout:
                value_dropout_cfg = self._dropout_cfg.get(feature_type, {}).get(key, {})
                if value_dropout_cfg and feature_type != 'sparse_seq':
                    raise ValueError('dropout is currently not supported for %s features' % feature_type)

                dropouts = self._collect_dropouts(value, value_dropout_cfg)

                if dropouts:
                    new_value = self._apply_dropouts(value, dropouts)

            if not new_value:
                new_value = deepcopy(value) if self._copy else value

            result[key] = new_value

        return result

    def _call(self, batch_features, do_dropout=False, **kwargs):
        if not self.features:
            return batch_features
        out = []
        for sample_features in batch_features:
            out.append(SampleFeatures(
                sample=sample_features.sample,
                sparse=self._get_subdict(sample_features.sparse, 'sparse', do_dropout),
                sparse_seq=self._get_subdict(sample_features.sparse_seq, 'sparse_seq', do_dropout),
                dense=self._get_subdict(sample_features.dense, 'dense', do_dropout),
                dense_seq=self._get_subdict(sample_features.dense_seq, 'dense_seq', do_dropout),
                dense_seq_ids=self._get_subdict(sample_features.dense_seq_ids, 'dense_seq_ids', do_dropout)
            ))
        return out
