# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import pytest
import os
from itertools import izip

from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.features_extractor import FeaturesExtractorFactory

from ..data_loaders import extract_features
from ..dataset import VinsDatasetBuilder, VinsDataset


@pytest.fixture(scope='module')
def samples_extractor():
    return SamplesExtractor.from_config({
        "pipeline": [
            {
                "name": "normalizer",
                "normalizer": "normalizer_ru"
            },
            {
                "name": "wizard"
            }
        ]
    })


@pytest.fixture(scope='module')
def features_extractor():
    feature_extractor_configs = [
        {
            "id": "word",
            "type": "ngrams",
            "n": 1
        },
        {
            "id": "wizard",
            "type": "wizard",
            "use_onto": "true",
            "use_freebase": "true"
        },
        {
            "id": "dssm",
            "type": "dssm_embeddings",
            "resource": "resource://dssm_embeddings/tf_model"
        },
        {
            "id": "serp",
            "type": "serp",
            "sequence": False,
            "prior_strength": 10,
            "log_frequency": True,
            "log_surplus": True,
            "frequency_file_or_dict": "personal_assistant://data/serp/alice_requests_popular_words.json",
            "data": "resource://suggest/query_wizard_features/query_wizard_features.data",
            "trie": "resource://suggest/query_wizard_features/query_wizard_features.trie"
        },
        {
            "id": "alice_requests_emb",
            "type": "embeddings",
            "resource": "resource://req_embeddings"
        }
    ]
    feature_extractor_factory = FeaturesExtractorFactory()
    for conf in feature_extractor_configs:
        feature_extractor_factory.add(**conf)

    return feature_extractor_factory.create_extractor()


@pytest.fixture(scope='module')
def scenarios_nlu_source_items():
    return {
        'alarm': FuzzyNLUFormat.parse_iter([
            'поставь будильник на "6 утра"(when)',
            'разбуди меня в "9 вечера"(when)',
        ], trainable_classifiers=('scenarios',)).items,
        'taxi': FuzzyNLUFormat.parse_iter([
            'поехали до "проспекта Маршала Жукова 3"(location_to)',
            'машину от "Льва Толстого 16"(location_from) в "без 15 6 вечера"(when) не дороже "500 рублей"(price)',
        ], trainable_classifiers=('scenarios',)).items
    }


@pytest.fixture(scope='module')
def metric_nlu_source_items():
    return {
        'reminder': FuzzyNLUFormat.parse_iter([
            'напомнить "сходить погулять"(what)',
            'создай напоминание на "9 вечера"(time)',
        ], trainable_classifiers=('metric',)).items,
        'taxi': FuzzyNLUFormat.parse_iter([
            'поехали до "проспекта Маршала Жукова 3"(location_to)',
            'закажи мне такси',
        ], trainable_classifiers=('metric',)).items
    }


def _are_sample_features_equal(first, second):
    if first.sample.text != second.sample.text:
        return False
    if first.sample.weight != second.sample.weight:
        return False

    first_features = set(first.dense.keys() + first.dense_seq.keys() +
                         first.sparse.keys() + first.dense_seq.keys())
    second_features = set(second.dense.keys() + second.dense_seq.keys() +
                          second.sparse.keys() + second.dense_seq.keys())
    if first_features != second_features:
        return False
    if any((first.dense[key] != second.dense[key]).any() for key in first.dense):
        return False
    if any((first.dense_seq[key] != second.dense_seq[key]).any() for key in first.dense_seq):
        return False
    if first.sparse != second.sparse:
        return False
    if any(set(first_list) != set(second_list)
           for first_list, second_list in izip(first.sparse_seq, second.sparse_seq)):
        return False
    return True


def _collect_features(samples_extractor, features_extractor, nlu_source_items, classifiers_list):
    results = extract_features(
        intent_to_source_data=nlu_source_items,
        samples_extractor=samples_extractor,
        features_extractor=features_extractor,
        custom_entity_parsers={},
        classifiers_list=classifiers_list,
    )

    initial_features = {
        intent: [res.sample_features for res in feature_extraction_results]
        for intent, feature_extraction_results in results.iteritems()
    }

    return results, initial_features


def _compare_features(initial_features, restored_features):
    assert set(initial_features.keys()) == set(restored_features.keys())

    for intent in initial_features:
        assert len(initial_features[intent]) == len(restored_features[intent])

        for feat in initial_features[intent]:
            assert any(_are_sample_features_equal(feat, restored_feat) for restored_feat in restored_features[intent])


def _to_intent_result_pairs(intent_to_results):
    return [(intent, result, None) for intent, results in intent_to_results.iteritems() for result in results]


@pytest.mark.parametrize('build_from_pairs', [False, True])
def test_dataset_building(samples_extractor, features_extractor, scenarios_nlu_source_items, build_from_pairs):
    results, initial_features = _collect_features(
        samples_extractor, features_extractor, scenarios_nlu_source_items, classifiers_list=('scenarios',)
    )

    if not build_from_pairs:
        dataset = VinsDatasetBuilder(intent_to_results=results).build()
    else:
        dataset = VinsDatasetBuilder(sample_infos=_to_intent_result_pairs(results)).build()

    restored_features = dataset.to_intent_to_sample_features('scenarios')

    _compare_features(initial_features, restored_features)


@pytest.mark.parametrize('build_from_pairs', [False, True])
def test_dataset_merge(samples_extractor, features_extractor, scenarios_nlu_source_items,
                       metric_nlu_source_items, build_from_pairs):
    scenarios_results, scenarios_intent_to_features = _collect_features(
        samples_extractor, features_extractor, scenarios_nlu_source_items, classifiers_list=('scenarios',)
    )

    if not build_from_pairs:
        dataset = VinsDatasetBuilder(intent_to_results=scenarios_results).build()
    else:
        dataset = VinsDatasetBuilder(sample_infos=_to_intent_result_pairs(scenarios_results)).build()

    metric_results, metric_intent_to_features = _collect_features(
        samples_extractor, features_extractor, metric_nlu_source_items, classifiers_list=('metric',)
    )

    if not build_from_pairs:
        dataset = VinsDatasetBuilder(intent_to_results=metric_results).merge_to(dataset)
    else:
        dataset = VinsDatasetBuilder(sample_infos=_to_intent_result_pairs(metric_results)).merge_to(dataset)

    for name, initial_features in [('scenarios', scenarios_intent_to_features), ('metric', metric_intent_to_features)]:
        restored_features = dataset.to_intent_to_sample_features(name)

        _compare_features(initial_features, restored_features)


@pytest.mark.parametrize('build_from_pairs', [False, True])
def test_dataset_split(samples_extractor, features_extractor, scenarios_nlu_source_items,
                       metric_nlu_source_items, build_from_pairs):
    scenarios_results, scenarios_intent_to_features = _collect_features(
        samples_extractor, features_extractor, scenarios_nlu_source_items, classifiers_list=('scenarios',)
    )

    if not build_from_pairs:
        dataset = VinsDatasetBuilder(intent_to_results=scenarios_results).build()
    else:
        dataset = VinsDatasetBuilder(sample_infos=_to_intent_result_pairs(scenarios_results)).build()

    metric_results, metric_intent_to_features = _collect_features(
        samples_extractor, features_extractor, metric_nlu_source_items, classifiers_list=('metric',)
    )

    if not build_from_pairs:
        dataset = VinsDatasetBuilder(intent_to_results=metric_results).merge_to(dataset)
    else:
        dataset = VinsDatasetBuilder(sample_infos=_to_intent_result_pairs(metric_results)).merge_to(dataset)

    train_dataset, val_dataset = VinsDataset.split(dataset, holdout_size=0.5, holdout_classifiers=('scenarios',))

    assert not val_dataset.to_intent_to_sample_features('metric'), 'All metric items should be in train_dataset'

    train_intent_to_features = train_dataset.to_intent_to_sample_features('scenarios')
    val_intent_to_features = val_dataset.to_intent_to_sample_features('scenarios')

    for intent in train_intent_to_features:
        assert len(train_intent_to_features[intent]) == len(val_intent_to_features[intent])

    restored_intent_to_index = {
        intent: train_intent_to_features[intent] + val_intent_to_features[intent]
        for intent in train_intent_to_features
    }
    _compare_features(scenarios_intent_to_features, restored_intent_to_index)
    _compare_features(metric_intent_to_features, train_dataset.to_intent_to_sample_features('metric'))


@pytest.mark.parametrize('build_from_pairs', [False, True])
def test_dataset_save_restore(samples_extractor, features_extractor, scenarios_nlu_source_items,
                              tmpdir, build_from_pairs):
    results, initial_features = _collect_features(
        samples_extractor, features_extractor, scenarios_nlu_source_items, classifiers_list=('scenarios',)
    )

    if not build_from_pairs:
        dataset = VinsDatasetBuilder(intent_to_results=results).build()
    else:
        dataset = VinsDatasetBuilder(sample_infos=_to_intent_result_pairs(results)).build()

    save_dir = str(tmpdir.mkdir('dataset'))
    dataset_path = os.path.join(save_dir, 'dataset.tar')
    dataset.save(dataset_path)

    restored_features = VinsDataset.restore(dataset_path).to_intent_to_sample_features('scenarios')
    _compare_features(initial_features, restored_features)
