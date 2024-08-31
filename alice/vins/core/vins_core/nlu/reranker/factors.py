# -*- coding: utf-8 -*-

import os
import cPickle as pickle
import numpy as np

from operator import itemgetter

from vins_core.nlu.reranker.factor_calcer import Factor
from vins_core.utils.data import get_resource_full_path, open_resource_file

from vins_models_tf import TfEncoder as TfModelApplier

# Imports used only for typing
from typing import AnyStr, NoReturn, Dict, List  # noqa: UnusedImport
from vins_core.nlu.reranker.factor_calcer import FactorCalcerContext  # noqa: UnusedImport


class WordNNFactor(Factor):
    NAME = 'word_nn'

    def __init__(self, model_name, model_applier, class_to_indices, model_output_size,
                 embeddings_name, return_distribution=False, batch_first=False):
        # type: (AnyStr, TfModelApplier, Dict[AnyStr, List[int]], int, AnyStr, bool, bool) -> NoReturn

        self._model_name = model_name
        self._model = model_applier
        # mapping from class names to list of corresponding indices in the predicted distribution
        self._class_to_predicted_indices = class_to_indices
        self._model_output_size = model_output_size
        self._embeddings_name = embeddings_name
        self._return_distribution = return_distribution
        self._batch_dim = 0 if batch_first else 1

    def append_factor_values(self, context, factor_values):
        # type: (FactorCalcerContext, List[np.ndarray]) -> NoReturn

        inputs = self._get_inputs(context.sample_features)

        if inputs is not None:
            predictions = self._model.encode(inputs)[0]
        else:
            predictions = np.zeros(self._model_output_size)

        # whether to return scores for candidates or for all intents
        if self._return_distribution:
            # in this case each candidate receives similar features - distribution on unmapped classes
            # reranker can learn the mapping from training data and use these features appropriately
            factor_values.append(np.tile(predictions, (len(context.classes), 1)))
        else:
            class_scores = [predictions[self._class_to_predicted_indices[class_]].sum() for class_ in context.classes]
            class_scores = np.array(class_scores)
            factor_values.append(np.expand_dims(class_scores, 1))

    def _get_inputs(self, sample_features):
        if self._embeddings_name not in sample_features.dense_seq:
            return None
        return {
            'dense_seq': np.expand_dims(sample_features.dense_seq[self._embeddings_name], self._batch_dim)
        }

    def get_factor_value_names(self):
        # type: () -> List[AnyStr]
        return [self._model_name]

    @classmethod
    def from_config(cls, config):
        model_path = get_resource_full_path(config['model_resource'])
        desc_path = get_resource_full_path(config['desc_resource'])
        labels_path = get_resource_full_path(config['labels_resource'])

        model_dir = os.path.dirname(model_path)

        assert model_dir == os.path.dirname(desc_path), 'Model and description should be stored in a single dir'

        model_applier = TfModelApplier(model_dir)
        with open(labels_path) as f:
            class_to_indices, model_output_size = pickle.load(f)

        return cls(model_name=config['model_name'],
                   model_applier=model_applier,
                   class_to_indices=class_to_indices,
                   model_output_size=model_output_size,
                   embeddings_name=config['embeddings_name'],
                   return_distribution=config.get('return_distribution', False),
                   batch_first=config.get('batch_first', False))


class KNNFactor(Factor):
    NAME = 'scenarios'

    def append_factor_values(self, context, factor_values):
        # type: (FactorCalcerContext, List[np.ndarray]) -> NoReturn
        scores = np.array(context.scenarios_scores)
        factor_values.append(np.expand_dims(scores, 1))

    def get_factor_value_names(self):
        # type: () -> List[AnyStr]
        return [self.NAME]

    @classmethod
    def from_config(cls, config):
        return cls()


class FallbackFactor(Factor):
    NAME = 'fallback_classifier'

    def append_factor_values(self, context, factor_values):
        # type: (FactorCalcerContext, List[np.ndarray]) -> NoReturn
        score = max(context.sample_features.dense['gc_search'])
        factor_values.append(np.tile(score, (len(context.classes), 1)))

    def get_factor_value_names(self):
        # type: () -> List[AnyStr]
        return [self.NAME]

    @classmethod
    def from_config(cls, config):
        return cls()


def _extract_keys_from_index(index):
    return [key for key, _ in sorted(index.iteritems(), key=itemgetter(1))]


class IntentBasedFactor(Factor):
    NAME = 'intent_info'

    def __init__(self, intent_index, intent_features):
        self._intent_index = intent_index
        self._intent_features = intent_features

    def append_factor_values(self, context, factor_values):
        # type: (FactorCalcerContext, List[np.ndarray]) -> NoReturn

        intent_indices = [self._intent_index.get(intent, len(self._intent_index)) for intent in context.classes]
        original_indices = [self._intent_index.get(intent, len(self._intent_index)) for intent in
                            context.original_classes]

        factor_values.append(self._intent_features[intent_indices] + self._intent_features[original_indices])

    def get_factor_value_names(self):
        # type: () -> List[AnyStr]
        return _extract_keys_from_index(self._intent_index)

    @classmethod
    def from_config(cls, config):
        with open_resource_file(config['known_intents_path']) as f:
            intent_index = {line.rstrip(): index for index, line in enumerate(f)}

        return cls(
            intent_index=intent_index,
            intent_features=cls._calc_intent_features(intent_index)
        )

    @classmethod
    def _calc_intent_features(cls, intent_index):
        return np.eye(len(intent_index) + 1, len(intent_index))


class BagOfEntitiesFactor(Factor):
    NAME = 'bag_of_entities'

    def __init__(self, entity_type_to_index, feature_name, apply_to_tag_sequence):
        self._entity_type_to_index = entity_type_to_index
        self._feature_name = feature_name
        self._apply_to_tag_sequence = apply_to_tag_sequence

    def append_factor_values(self, context, factor_values):
        # type: (FactorCalcerContext, List[np.ndarray]) -> NoReturn

        factor_value = self._calc_factor_value(context.sample_features)
        factor_value = np.tile(factor_value, (len(context.classes), 1))
        factor_values.append(factor_value)

    def _calc_factor_value(self, sample_feature):
        factor_value = np.zeros(len(self._entity_type_to_index), dtype=np.bool)

        for entity_type in self._iterate_entities(sample_feature):
            entity_type_index = self._entity_type_to_index.get(entity_type, -1)
            if entity_type_index != -1:
                factor_value[entity_type_index] = True

        return factor_value

    def _iterate_entities(self, sample_feature):
        if self._apply_to_tag_sequence:
            for token_tags in sample_feature.sparse_seq.get(self._feature_name, []):
                for token_tag in token_tags:
                    yield token_tag.value[2:]
        else:
            for entity_feature in sample_feature.sparse.get(self._feature_name, []):
                yield entity_feature.value

    def get_factor_value_names(self):
        # type: () -> List[AnyStr]
        return _extract_keys_from_index(self._entity_type_to_index)

    @classmethod
    def from_config(cls, config):
        feature_name = config['feature_name']

        with open_resource_file(config['known_entities_path']) as f:
            entity_type_to_index = {line.rstrip(): index for index, line in enumerate(f)}

        return cls(
            entity_type_to_index=entity_type_to_index,
            feature_name=feature_name,
            apply_to_tag_sequence=config['apply_to_tag_sequence']
        )


class DenseFeatureFactor(Factor):
    NAME = 'dense_feature'

    def __init__(self, feature_name, feature_list):
        self._feature_name = feature_name
        self._feature_list = feature_list
        self._default_feature_value = np.zeros(len(feature_list))

    def append_factor_values(self, context, factor_values):
        # type: (FactorCalcerContext, List[np.ndarray]) -> NoReturn

        feature_value = context.sample_features.dense.get(self._feature_name, self._default_feature_value)

        assert feature_value.shape == self._default_feature_value.shape

        factor_value = np.tile(feature_value, (len(context.classes), 1))
        factor_values.append(factor_value)

    def get_factor_value_names(self):
        # type: () -> List[AnyStr]
        return self._feature_list

    @classmethod
    def from_config(cls, config):
        with open_resource_file(config['feature_list_path']) as f:
            feature_list = [line.rstrip() for line in f]

        return cls(config['feature_name'], feature_list)
