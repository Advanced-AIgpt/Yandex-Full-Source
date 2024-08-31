# -*- coding: utf-8 -*-
""" Model for features that depends only on intents, like intent indices or is handcrafted flag info.
Converts to IntentBasedFactor on inference.
"""

import os
import cPickle as pickle
import numpy as np

# Imports used only for typing
from typing import AnyStr, NoReturn  # noqa: F401


class IntentBasedModel(object):
    _MODEL_FILE_NAME = 'data_info.pkl'

    def __init__(self, intent_to_index, train_intents):
        self._intent_to_index = intent_to_index
        self._index_to_intent = [intent for intent, _ in sorted(intent_to_index.iteritems(), key=lambda x: x[1])]
        self._train_intents = train_intents
        self._precomputed_features_matrix = self._calc_intent_based_features(intent_to_index, self._train_intents)

    @property
    def known_intents(self):
        return set(self._train_intents)

    def _calc_intent_based_features(self, intent_to_index, train_intents):
        features = np.zeros((len(intent_to_index), len(self.get_features_desc())))
        for intent, index in intent_to_index.iteritems():
            if intent in train_intents:
                features[index, train_intents[intent]] = 1.

        return features

    def __call__(self, intent_indices):
        # type: (np.array) -> np.array
        return self._precomputed_features_matrix[intent_indices]

    def get_features_desc(self):
        return [intent for intent, _ in sorted(self._train_intents.iteritems(), key=lambda x: x[1])]

    def save(self, dir_path):
        # type: (AnyStr) -> NoReturn

        if not os.path.isdir(dir_path):
            os.makedirs(dir_path)

        with open(os.path.join(dir_path, self._MODEL_FILE_NAME), 'wb') as f:
            pickle.dump((self._intent_to_index, self._precomputed_features_matrix), f, pickle.HIGHEST_PROTOCOL)
