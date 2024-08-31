# -*- coding: utf-8 -*-
""" Reranker trainer.
Intended use-case - conversion of VinsDataset object to .tsv file
 which can be used for Catboost model training on Nirvana.
"""

import os
import pandas as pd
import numpy as np
import cPickle as pickle

from catboost import Pool, CatBoost

# Imports used only for typing
from typing import List, AnyStr, NoReturn  # noqa: F401


class RerankerModel(object):
    _MODEL_FILE_NAME = 'reranker.cbm'
    _MODEL_DESC_FILE_NAME = 'reranker.desc'

    _SCENARIOS_FEAT_NAME = 'scenarios'
    # features which values depend on pair (intent, sample)
    _INTENT_FEAT_NAMES = [_SCENARIOS_FEAT_NAME, 'scenarios_lstm-model', 'toloka_lstm-model_to_scenarios']
    _SETUP_FEATURES = ['music_sites_count',
                       'video_sites_count',
                       'music_in_top_1',
                       'video_in_top_1',
                       'music_in_top_3',
                       'video_in_top_3',
                       'music_in_top_5',
                       'video_in_top_5',
                       'first_music_site_pos',
                       'first_music_siterelevance',
                       'first_music_siterelevpredict',
                       'first_video_site_pos',
                       'first_video_siterelevance',
                       'first_video_siterelevpredict',
                       'video_kinopoisk_data_pos',
                       'video_kinopoisk_datarelevance',
                       'video_kinopoisk_datarelevpredict',
                       'video_ivi_data_pos',
                       'video_ivi_datarelevance',
                       'video_ivi_datarelevpredict',
                       'video_hdlava_data_pos',
                       'video_hdlava_datarelevance',
                       'video_hdlava_datarelevpredict',
                       'video_onlinemultfilmy_data_pos',
                       'video_onlinemultfilmy_datarelevance',
                       'video_onlinemultfilmy_datarelevpredict',
                       'video_filmshd_data_pos',
                       'video_filmshd_datarelevance',
                       'video_filmshd_datarelevpredict',
                       'video_rutube_data_pos',
                       'video_rutube_datarelevance',
                       'video_rutube_datarelevpredict',
                       'music_zvooq_data_pos',
                       'music_zvooq_datarelevance',
                       'music_zvooq_datarelevpredict',
                       'music_zaycev_data_pos',
                       'music_zaycev_datarelevance',
                       'music_zaycev_datarelevpredict',
                       'music_drivemusic_data_pos',
                       'music_drivemusic_datarelevance',
                       'music_drivemusic_datarelevpredict',
                       'music_megapesni_data_pos',
                       'music_megapesni_datarelevance',
                       'music_megapesni_datarelevpredict',
                       'music_muzmo_data_pos',
                       'music_muzmo_datarelevance',
                       'music_muzmo_datarelevpredict',
                       'music_yandex_data_pos',
                       'music_yandex_datarelevance',
                       'music_yandex_datarelevpredict',
                       'music_muzofond_data_pos',
                       'music_muzofond_datarelevance',
                       'music_muzofond_datarelevpredict',
                       'youtube_data_pos',
                       'youtube_datarelevance',
                       'youtube_datarelevpredict',
                       'wikipedia_data_pos',
                       'wikipedia_datarelevance',
                       'wikipedia_datarelevpredict',
                       'music_wizard_present',
                       'music_wizard_show',
                       'music_wizard_relevance',
                       'music_wizard_pos',
                       'video_wizard_present',
                       'video_wizard_show',
                       'video_wizard_yandex_video',
                       'video_wizard_pos',
                       'entity_search_present',
                       'entity_search_found',
                       'entity_search_anim',
                       'entity_search_auto',
                       'entity_search_band',
                       'entity_search_chemical_compound',
                       'entity_search_device',
                       'entity_search_drugs',
                       'entity_search_event',
                       'entity_search_film',
                       'entity_search_food',
                       'entity_search_geo',
                       'entity_search_hum',
                       'entity_search_hum1',
                       'entity_search_music',
                       'entity_search_org',
                       'entity_search_picture',
                       'entity_search_site',
                       'entity_search_soft',
                       'entity_search_text']
    _USE_INTENT_BASED_FEATURES = True
    _USE_SETUP_FEATURES = True
    _USE_PREV_INTENT = False
    _USE_ENTITY_FEATURES = True
    # features which values depend on sample only
    _SAMPLE_FEAT_NAMES = ['ner_feature', 'wizard_feature']

    @classmethod
    def required_feature_list(cls):
        # type: () -> List[AnyStr]
        return cls._SAMPLE_FEAT_NAMES + cls._INTENT_FEAT_NAMES + ['scenarios', 'prev_intent', 'entity',
                                                                  'video_play_setup']

    def __init__(self, intent_to_index, intent_based_model, dataset, **kwargs):
        self._intent_to_index = intent_to_index

        self._cat_feature_indices = self.get_cat_features_indices(intent_based_model, dataset)

        params = {
            'loss_function': 'QuerySoftMax',
            'iterations': 1000,
            'task_type': 'GPU',
            'random_seed': 0,
        }
        params.update(**kwargs)

        self._model = CatBoost(params)

    @classmethod
    def get_features(cls, configs_base_path='apps/personal_assistant/personal_assistant/data/post_classifier/'):
        expected_features = []
        expected_features.extend(cls._INTENT_FEAT_NAMES)

        if cls._USE_INTENT_BASED_FEATURES:
            with open(os.path.join(configs_base_path, 'intent_info_known_intents.txt')) as f:
                expected_features.extend(line.strip() for line in f)

        if cls._USE_ENTITY_FEATURES:
            with open(os.path.join(configs_base_path, 'entitysearch_entities.txt')) as f:
                expected_features.extend(line.strip() for line in f)

        for feature in cls._SAMPLE_FEAT_NAMES:
            feature_path = feature.replace('feature', 'entities.txt')
            with open(os.path.join(configs_base_path, feature_path)) as f:
                expected_features.extend(line.strip() for line in f)

        if cls._USE_SETUP_FEATURES:
            expected_features.extend(cls._SETUP_FEATURES)

        return expected_features

    @classmethod
    def get_cat_features_indices(cls, intent_based_model, dataset):
        cat_feature_indices = []
        shift = len(cls._INTENT_FEAT_NAMES)
        if cls._USE_INTENT_BASED_FEATURES:
            shift += len(intent_based_model.get_features_desc())
        if cls._USE_ENTITY_FEATURES:
            shift += len(dataset._classifier_feature_mappings['entity'])
        if cls._USE_PREV_INTENT:
            cat_feature_indices.append(shift)
            shift += 1
        if 'device_state_feature' in cls._SAMPLE_FEAT_NAMES:
            cat_feature_indices.extend((shift, shift + 1, shift + 2))
            shift += 3
        if cls._USE_SETUP_FEATURES:
            shift += len(cls._SETUP_FEATURES)
        return cat_feature_indices

    def fit(self, train_data, train_groups, train_labels, val_data=None):
        train_pool = Pool(
            data=train_data[:, 1:], label=train_labels, group_id=train_groups,
            cat_features=self._cat_feature_indices
        )
        if val_data:
            val_pool = Pool(
                data=val_data[0][:, 1:], label=val_data[2], group_id=val_data[1],
                cat_features=self._cat_feature_indices
            )
        else:
            val_pool = None
        self._model.fit(train_pool, eval_set=val_pool, use_best_model=True)

        return self._model.get_feature_importance(train_pool)

    def predict(self, data, groups):
        variants = data[:, 0].astype(np.int32)
        pool = Pool(data=data[:, 1:], group_id=groups, cat_features=self._cat_feature_indices)
        predictions = self._model.predict(pool)

        return np.array(self.convert_predictions(predictions, groups, variants)), predictions

    def convert_predictions(self, predictions, group_ids, variant_ids):
        def _iterate_sample_predictions(preds, group_ids, variant_ids):
            cur_group_begin_pos, cur_group_id = 0, 0
            for pos, group_id in enumerate(group_ids):
                if group_id != cur_group_id:
                    cur_group_preds = preds[cur_group_begin_pos: pos]
                    if len(cur_group_preds) > 0:
                        yield variant_ids[cur_group_begin_pos + np.argmax(cur_group_preds)]

                    cur_group_begin_pos, cur_group_id = pos, group_id

            last_group_preds = preds[cur_group_begin_pos:]
            yield variant_ids[cur_group_begin_pos + np.argmax(last_group_preds)]

        return list(_iterate_sample_predictions(predictions, group_ids, variant_ids))

    def save(self, dir_path):
        # type: (AnyStr) -> NoReturn

        if not os.path.isdir(dir_path):
            os.makedirs(dir_path)

        self._model.save_model(os.path.join(dir_path, self._MODEL_FILE_NAME))

        known_intents = set(self._intent_to_index.keys())
        with open(os.path.join(dir_path, self._MODEL_DESC_FILE_NAME), 'wb') as f:
            pickle.dump((known_intents, self._cat_feature_indices), f, pickle.HIGHEST_PROTOCOL)

    def load(self, model_path):
        self._model.load_model(model_path)

    @classmethod
    def convert_dataset(cls, dataset, intent_based_model, use_labels, skip_ones, skip_all_negatives,
                        skip_intents, forced_intents, intent_to_index=None, threshold=0.65, fallback_top_size=10):

        # converting dataset to the map from feature names to their matrices
        data = dataset[:]

        known_intent_indices = {
            intent_to_index[intent] for intent in intent_based_model.known_intents if intent in intent_to_index
        }

        labels = np.array([intent_to_index.get(intent, len(intent_to_index)) for intent in data['intent']])

        forced_intent_indices = [intent_to_index[intent] for intent in forced_intents] if forced_intents else []

        skip_intent_indices = []
        if skip_intents:
            skip_intent_indices = [intent_to_index[intent] for intent in skip_intents if intent in intent_to_index]

        ranking_groups, ranking_variants = cls._collect_sample_indices(
            data=data, labels=labels, use_labels=use_labels, known_intent_indices=known_intent_indices,
            skip_ones=skip_ones, skip_all_negatives=skip_all_negatives, skip_intent_indices=skip_intent_indices,
            forced_intents=forced_intent_indices, threshold=threshold, fallback_top_size=fallback_top_size
        )

        has_all_elements_from_dataset = np.max(ranking_groups) + 1 == len(np.unique(ranking_groups)) == len(dataset)
        assert skip_ones or skip_all_negatives or has_all_elements_from_dataset

        ranking_data = []
        ranking_data.append(np.expand_dims(ranking_variants, 1))

        for feat_type in cls._INTENT_FEAT_NAMES:
            mapping = dataset.map_classifiers(feat_type, cls._SCENARIOS_FEAT_NAME)

            preds = np.concatenate((data[feat_type], np.zeros((data[feat_type].shape[0], 1))), -1)
            variants = [mapping.get(ind, preds.shape[1] - 1) for ind in ranking_variants]

            ranking_data.append(np.expand_dims(preds[ranking_groups, variants], 1))

        if cls._USE_INTENT_BASED_FEATURES:
            ranking_data.append(intent_based_model(ranking_variants))

        if cls._USE_PREV_INTENT:
            ranking_data.append(data['prev_intent'][ranking_groups])

        if cls._USE_ENTITY_FEATURES:
            ranking_data.append(data['entity'][ranking_groups])

        for feat_type in cls._SAMPLE_FEAT_NAMES:
            ranking_data.append(data[feat_type][ranking_groups])

        if cls._USE_SETUP_FEATURES:
            if 'video_play_setup' in data:
                ranking_data.append(data['video_play_setup'][ranking_groups])
            else:
                ranking_data.append(np.zeros((len(ranking_groups), len(cls._SETUP_FEATURES))))

        ranking_data = np.concatenate(ranking_data, axis=1)

        assert ranking_data.shape[1] - 1 == len(cls.get_features())

        ranking_labels = ranking_variants == labels[ranking_groups] if labels is not None else None
        return ranking_data, ranking_groups, ranking_labels

    @classmethod
    def _collect_sample_indices(cls, data, labels, known_intent_indices, use_labels, skip_ones, skip_all_negatives,
                                skip_intent_indices, forced_intents, threshold, fallback_top_size):
        intent_predictions = np.array(data[cls._SCENARIOS_FEAT_NAME])
        sorted_intent_indices = np.argsort(-intent_predictions, axis=-1)

        ranking_groups, ranking_variants = [], []
        for sample_ind in xrange(len(intent_predictions)):
            if skip_all_negatives and labels is not None and labels[sample_ind] in skip_intent_indices:
                continue

            variants_mask = intent_predictions[sample_ind] > threshold
            if skip_ones:
                variants_mask *= intent_predictions[sample_ind] != 1.

            variants = list(set(variants_mask.nonzero()[0]) & known_intent_indices)
            if use_labels or len(variants) >= 1:
                for variant in forced_intents:
                    if variant not in variants:
                        variants.append(variant)

                if labels[sample_ind] not in variants:
                    if skip_all_negatives:
                        continue
                    variants = sorted_intent_indices[sample_ind, :fallback_top_size]
            if not use_labels:
                if len(variants) == 0:
                    variants = sorted_intent_indices[sample_ind, :fallback_top_size]
                variants = list(variants)
                for variant in forced_intents:
                    if variant not in variants:
                        variants.append(variant)

            if len(variants) > 1 or (not use_labels and len(variants) == 1):
                assert all(variant in variants for variant in forced_intents)

                ranking_groups.extend((sample_ind for _ in variants))
                ranking_variants.extend((variant_ind for variant_ind in variants))

        return np.array(ranking_groups), np.array(ranking_variants)

    @classmethod
    def dump_dataset_to_tsv(cls, dataset, ranking_data, ranking_groups, ranking_labels, ranking_variants,
                            sample_labels, sample_texts, intent_based_model, path, cd_path):
        sample_labels = sample_labels[ranking_groups]
        sample_texts = sample_texts[ranking_groups]

        ranking_labels = pd.DataFrame(np.expand_dims(ranking_labels, 1).astype(np.int))
        ranking_groups = pd.DataFrame(np.expand_dims(ranking_groups, 1))
        sample_texts = pd.DataFrame(np.expand_dims(sample_texts, 1))
        sample_labels = pd.DataFrame(np.expand_dims(sample_labels, 1))
        ranking_variants = pd.DataFrame(np.expand_dims(ranking_variants, 1))
        ranking_data = pd.DataFrame(ranking_data)

        data = pd.concat(
            (ranking_labels, ranking_groups, sample_texts, sample_labels, ranking_variants, ranking_data), axis=1
        )

        data.to_csv(path, encoding='utf-8', sep='\t', header=False, index=False)

        with open(cd_path, 'w') as f:
            def add_column_description(col_index, col_type, col_desc=None):
                if col_desc:
                    f.write('{}\t{}\t{}\n'.format(col_index, col_type, col_desc))
                else:
                    f.write('{}\t{}\n'.format(col_index, col_type))

            add_column_description(col_index=0, col_type='Label')
            add_column_description(col_index=1, col_type='GroupId')
            add_column_description(col_index=2, col_type='Auxiliary', col_desc='Query text')
            add_column_description(col_index=3, col_type='Auxiliary', col_desc='True label')
            add_column_description(col_index=4, col_type='Auxiliary', col_desc='Predicted label')

            feature_indices_shift = 5

            features = cls.get_features()
            cat_feature_indices = cls.get_cat_features_indices(intent_based_model, dataset)

            for ind, feature in enumerate(features):
                col_type = 'Categ' if ind in cat_feature_indices else 'Num'
                add_column_description(col_index=ind + feature_indices_shift, col_type=col_type, col_desc=feature)
