# -*- coding: utf-8 -*-

"""Builds dataset from specified paths to source data"""

import argparse
import logging
import cPickle as pickle

from vins_core.dm.formats import NluSourceItem
from vins_core.nlu.base_nlu import FeatureExtractorResult

from dataset import VinsDatasetBuilder, VinsDataset
from vins_config_tools import get_intent_config_paths, load_classifier_config, load_tagger_config
from vins_core.config.app_config import load_app_config
from data_loaders import load_source_data_from_tsv, load_source_data_by_config, load_extractors, extract_features


logger = logging.getLogger(__name__)


def _setup_environment_for_extractors():
    # Few settings copy-pasted from compile_app_model
    import os
    import tensorflow as tf
    tf.logging.set_verbosity(tf.logging.ERROR)

    os.environ['VINS_SKILLS_ENV_TYPE'] = 'beta'
    os.environ['VINS_LOAD_TF_ON_CALL'] = '1'
    os.environ['VINS_MISSPELL_TIMEOUT'] = '0.5'

    logging.getLogger('requests').setLevel(logging.INFO)
    logging.getLogger('vins_core.utils.decorators').setLevel(logging.INFO)
    logging.getLogger('vins_core.ext.base').setLevel(logging.WARNING)

    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')


def _setup_args_parser():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--app-name', default='personal_assistant.app')
    parser.add_argument('--source-data-path', default=None,
                        help='Path to source data, None in case of loading by config')
    parser.add_argument('--source-data-type', choices=['configs', 'tsv'], type=str.lower)
    parser.add_argument('--build-scenarios-dataset', action='store_true', default=False,
                        help='Whether to collect source data from metric learning configs.\n'
                             'WARNING: use scenarios_classifier_trainer to build scenarios dataset properly')

    parser.add_argument('--extract-features', action='store_true', default=False,
                        help='Whether to extract features on NluSourceItems')
    parser.add_argument('--samples-extractor-list', nargs='+', type=str,
                        help='List of SamplesExtractor')
    parser.add_argument('--features-extractor-list', nargs='+', type=str,
                        help='List of FeaturesExtractor')
    parser.add_argument('--features-from-classifier', type=str, default=None,
                        help='Specifies list of FeaturesExtractor using the classifiers config'
                             ' (overrides --features-extractor-list param).')
    parser.add_argument('--target-classifier', type=str, default=None,
                        help='Name of classifier which would be trained on the features.')
    parser.add_argument('--features-from-tagger', action='store_true', default=False,
                        help='Collect features to train taggers.')

    parser.add_argument('--feature-cache', default=None)

    parser.add_argument('--convert-to-dataset', action='store_true', default=False,
                        help='Whether to convert features to dataset'
                             ' (if selected extract-features param is considered to be True)')
    parser.add_argument('--add-to-dataset', default=None, help='Path to dataset which features should be merge to')

    parser.add_argument('--result-path', type=str, help='Path where the collected data should be saved')

    return parser


def load_intent_to_source_data(source_data_path, source_data_type, app_name, is_metric_learning_dataset,
                               target_classifier, intent_configs=None):
    if source_data_type == 'configs':
        if intent_configs is None:
            intent_configs = get_intent_config_paths(app_name, is_metric_learning_dataset)
        return load_source_data_by_config(app_name, intent_configs)
    elif source_data_type == 'tsv':
        return load_source_data_from_tsv(path=source_data_path, trainable_classifiers=(target_classifier,))
    raise NotImplementedError


def collect_features(intent_to_source_data, app_conf, samples_extractor_list, features_extractor_list,
                     features_from_classifier, target_classifier, feature_cache, taggers, load_custom_entities=True):
    if features_from_classifier is not None:
        classifier_conf = load_classifier_config(app_conf, features_from_classifier)
        features_extractor_list = classifier_conf['features']
    else:
        if features_extractor_list is None:
            features_extractor_list = [config['id'] for config in app_conf.nlu.get('feature_extractors', {})]

    if taggers:
        tagger_conf = load_tagger_config(app_conf)
        updated_features_extractor_list = set(features_extractor_list)
        updated_features_extractor_list.update(tagger_conf.get('features', []))
        features_extractor_list = list(updated_features_extractor_list)

    samples_extractor, features_extractor, custom_entity_parsers = load_extractors(
        app_conf, samples_extractor_list=samples_extractor_list,
        features_extractor_list=features_extractor_list, load_custom_entities=load_custom_entities
    )

    return extract_features(intent_to_source_data, samples_extractor, features_extractor, custom_entity_parsers,
                            [target_classifier], feature_cache, taggers)


def _rename_classifier(res, old_name, new_name):
    metric_learning_trainable_classifiers = tuple(
        new_name if classifier == old_name else classifier for classifier in res.item.trainable_classifiers
    )

    res.item = NluSourceItem(text=res.item.text, original_text=res.item.original_text, slots=res.item.slots,
                             can_use_to_train_tagger=res.item.can_use_to_train_tagger,
                             trainable_classifiers=metric_learning_trainable_classifiers)
    return res


def _build_dataset(app_name, source_data_path, source_data_type, is_metric_learning_dataset,
                   convert_to_sample_features, samples_extractor_list, features_extractor_list,
                   features_from_classifier, feature_cache, target_classifier,
                   convert_to_dataset, add_to_dataset, target_classifier_renaming=None,
                   taggers=False):

    logger.info('Collecting source data')
    intent_to_source_data = load_intent_to_source_data(source_data_path, source_data_type, app_name,
                                                       is_metric_learning_dataset, target_classifier)

    app_conf = load_app_config(app_name)

    if convert_to_sample_features or convert_to_dataset:
        assert target_classifier is not None, 'You should specify a target classifier'

        logger.info('Extracting features')
        intent_to_features = collect_features(intent_to_source_data, app_conf, samples_extractor_list,
                                              features_extractor_list, features_from_classifier,
                                              target_classifier, feature_cache, taggers)

        if target_classifier_renaming:
            renamed_intent_to_features = {}
            for intent, results in intent_to_features.iteritems():
                renamed_intent_to_features[intent] = [
                    _rename_classifier(res, target_classifier, target_classifier_renaming)
                    for res in results if isinstance(res, FeatureExtractorResult)
                ]
            intent_to_features = renamed_intent_to_features

        if convert_to_dataset:
            logger.info('Building VinsDataset')
            return VinsDatasetBuilder(intent_to_features).build()
        elif add_to_dataset is not None:
            logger.info('Merging VinsDataset to %s', add_to_dataset)
            initial_dataset = VinsDataset.restore(add_to_dataset)
            return VinsDatasetBuilder(intent_to_features).merge_to(initial_dataset)

        return intent_to_features

    return intent_to_source_data


def main():
    _setup_environment_for_extractors()

    parser = _setup_args_parser()
    args = parser.parse_args()

    assert args.result_path, 'You should provide a result path'

    if args.build_scenarios_dataset:
        source_data_type = 'configs'

        logger.info('Building metric dataset')
        metric_dataset = _build_dataset(app_name=args.app_name, target_classifier='scenarios',
                                        source_data_path=args.source_data_path, source_data_type=source_data_type,
                                        is_metric_learning_dataset=True, convert_to_sample_features=True,
                                        samples_extractor_list=args.samples_extractor_list,
                                        features_extractor_list=args.features_extractor_list,
                                        features_from_classifier='scenarios', feature_cache=args.feature_cache,
                                        convert_to_dataset=True, add_to_dataset=None,
                                        target_classifier_renaming='scenarios_metric',
                                        taggers=args.features_from_tagger)
        metric_dataset.save(args.result_path)

        logger.info('Building knn dataset')
        knn_dataset = _build_dataset(app_name=args.app_name, target_classifier='scenarios',
                                     source_data_path=args.source_data_path, source_data_type=source_data_type,
                                     is_metric_learning_dataset=False, convert_to_sample_features=True,
                                     samples_extractor_list=args.samples_extractor_list,
                                     features_extractor_list=args.features_extractor_list,
                                     features_from_classifier='scenarios', feature_cache=args.feature_cache,
                                     convert_to_dataset=False, add_to_dataset=args.result_path,
                                     taggers=args.features_from_tagger)
        knn_dataset.save(args.result_path)
    else:
        result_data = _build_dataset(app_name=args.app_name, target_classifier=args.target_classifier,
                                     source_data_path=args.source_data_path, source_data_type=args.source_data_type,
                                     is_metric_learning_dataset=False, convert_to_sample_features=args.extract_features,
                                     samples_extractor_list=args.samples_extractor_list,
                                     features_extractor_list=args.features_extractor_list,
                                     features_from_classifier=args.features_from_classifier,
                                     feature_cache=args.feature_cache, convert_to_dataset=args.convert_to_dataset,
                                     add_to_dataset=args.result_path, taggers=args.features_from_tagger)

        if result_data is not None:
            if isinstance(result_data, VinsDataset):
                result_data.save(args.result_path)
            else:
                with open(args.result_path, 'wb') as f:
                    pickle.dump(result_data, f, pickle.HIGHEST_PROTOCOL)


if __name__ == '__main__':
    main()
