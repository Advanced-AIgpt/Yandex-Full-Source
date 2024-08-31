# -*- coding: utf-8 -*-

import argparse
import logging
import os
import shutil
import tempfile

from collections import defaultdict

# Imports used only for typing
from typing import Mapping, AnyStr, List, NoReturn  # noqa: UnusedImport
from vins_core.nlu.samples_extractor import SamplesExtractor  # noqa: UnusedImport
from vins_core.nlu.features_extractor import FeaturesExtractor  # noqa: UnusedImport
from vins_core.nlu.base_nlu import FeatureExtractorResult, IntentInfo  # noqa: UnusedImport

from vins_core.nlu.flow_nlu_factory.transition_model import create_transition_model
from vins_core.nlu.token_classifier import create_token_classifier
from vins_core.nlu.neural.metric_learning.metric_learning import TrainMode as MetricTrainMode

from data_loaders import load_extractors
from dataset_building import load_intent_to_source_data, collect_features
from train_utils import check_feature_cache_consistency
from vins_config_tools import open_model_archive, load_classifier_config, get_intent_config_paths, collect_intent_infos
from vins_core.config.app_config import load_app_config
from vins_core.nlu.flow_nlu_factory.transition_model import register_transition_model
from personal_assistant.transition_model import create_pa_transition_model

# by default, personal_assistant is not imported so the transition model is not registered
register_transition_model('personal_assistant', create_pa_transition_model)

logger = logging.getLogger(__name__)


def _train_pipeline(app_name, load_custom_entities, feature_cache, custom_intents):
    # type: (AnyStr, bool, AnyStr, List[AnyStr]) -> NoReturn

    classifier_name = 'scenarios'

    app_conf = load_app_config(app_name)
    check_feature_cache_consistency(feature_cache, app_conf)

    logger.info('Collecting source data')
    intent_to_source_data = load_intent_to_source_data(
        source_data_path=None,
        source_data_type='configs',
        app_name=app_name,
        is_metric_learning_dataset=True,
        target_classifier=classifier_name,
        intent_configs=custom_intents
    )

    logger.info('Extracting features')
    intent_to_feature_extractor_results = collect_features(
        intent_to_source_data=intent_to_source_data,
        app_conf=app_conf,
        samples_extractor_list=None,
        features_extractor_list=None,
        features_from_classifier=None,
        target_classifier=classifier_name,
        feature_cache=feature_cache,
        taggers=False,
        load_custom_entities=load_custom_entities
    )

    logger.info('Starting metric learning training')
    classifier = None
    try:
        classifier = _train_metric(
            app_name, app_conf, classifier_name, intent_to_feature_extractor_results, custom_intents
        )
    except KeyboardInterrupt:
        logger.info('Metric training was interrupted (hope you gave the beast a chance to converge)')

    if classifier is not None:
        logger.info('Saving updated model to archive')
        _save_classifier_to_tar_archive(app_name, classifier_name, classifier)


def _train_metric(app_name, app_config, classifier_name, intent_to_feature_extractor_results, intent_configs=None):
    classifier_conf = load_classifier_config(app_config, classifier_name)
    if intent_configs is None:
        intent_configs = get_intent_config_paths(app_name, is_metric_learning=True)
    intent_infos, intents = collect_intent_infos(app_config, intent_configs)

    intent_to_features = _convert_to_intent_to_features(
        intent_to_feature_extractor_results, intent_infos, classifier_name
    )

    # todo: make sure the feature extractor is correct (the same as for application)
    samples_extractor, features_extractor, _ = load_extractors(
        app_config, features_extractor_list=['emb_ids']
    )

    params = classifier_conf.get('params', {})
    params['metric_learning'] = MetricTrainMode.METRIC_LEARNING_FROM_SCRATCH

    classifier = _create_classifier(
        classifier_conf, intent_infos=intent_infos, samples_extractor=samples_extractor,
        features_extractor=features_extractor, **params
    )

    transition_model_config = app_config.nlu.get('transition_model', dict())
    transition_model = create_transition_model(
        intents=intents, **transition_model_config)

    classifier.train(intent_to_features=intent_to_features,
                     transition_model=transition_model and transition_model.model,
                     intent_infos=intent_infos)
    return classifier


def _convert_to_intent_to_features(intent_to_feature_extractor_results, intent_infos, classifier):
    intent_to_features = defaultdict(list)

    for intent_name, intent_info in intent_infos.iteritems():
        for result in intent_to_feature_extractor_results.get(intent_name, ()):
            if not isinstance(result, FeatureExtractorResult):
                continue
            if any(item_classifier == classifier for item_classifier in result.item.trainable_classifiers):
                intent_to_features[intent_name].append(result.sample_features)

    return intent_to_features


def _create_classifier(config, intent_infos=None, samples_extractor=None, features_extractor=None, **kwargs):
    # type: (Mapping, Mapping[AnyStr, IntentInfo], SamplesExtractor, FeaturesExtractor, ...) -> TokenClassifier

    classifier = create_token_classifier(
        model=config['model'],
        select_features=config.get('features', ()),
        samples_extractor=samples_extractor,
        intent_infos=intent_infos,
        features_extractor_info=features_extractor.classifiers_info if features_extractor is not None else None,
        **kwargs
    )
    assert classifier is not None, "Unable to find classifier {}".format(config['model'])
    return classifier


def _save_classifier_to_tar_archive(app_name, classifier_name, classifier):
    # type: (AnyStr, AnyStr, TokenClassifier) -> NoReturn

    logger.info('Saving the model')
    temp_dir = None
    try:
        temp_dir = tempfile.mkdtemp()
        app_config = load_app_config(app_name)
        with open_model_archive(app_config) as archive:
            archive.extract_all(temp_dir)

        with open_model_archive(app_config, mode='w') as archive:
            for subdir_name in os.listdir(temp_dir):
                if subdir_name != 'classifiers':
                    archive.add(subdir_name, os.path.join(temp_dir, subdir_name))
                    continue
                with archive.nested('classifiers') as classifiers_archive:
                    for subsubdir_name in os.listdir(os.path.join(temp_dir, subdir_name)):
                        if subsubdir_name == classifier_name:
                            classifier.save(classifiers_archive, classifier_name)
                            continue
                        classifiers_archive.add(subsubdir_name, os.path.join(temp_dir, subdir_name, subsubdir_name))
    finally:
        if temp_dir and os.path.isdir(temp_dir):
            shutil.rmtree(temp_dir)


def do_main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--app-name', default='personal_assistant',
                        help="The name of the app to compile models for",)
    parser.add_argument('--load-custom-entities', action='store_true', default=False,
                        help='Use this flag for loading custom entities from archive')
    parser.add_argument('--feature-cache', type=str, default=None,
                        help='File path to store / retrieve precomputed train features')
    parser.add_argument('--custom-intents', dest='custom_intents', nargs='+',
                        help='Filepath with VinsProject-like formatted custom intents')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    logging.getLogger('requests').setLevel(logging.INFO)
    logging.getLogger('vins_core.utils.decorators').setLevel(logging.INFO)
    logging.getLogger('vins_core.ext.base').setLevel(logging.WARNING)
    logging.getLogger('vins_core.utils.metrics').setLevel(logging.WARNING)
    logging.getLogger('vins_core.utils.lemmer').setLevel(logging.WARNING)

    _train_pipeline(app_name=args.app_name,
                    load_custom_entities=args.load_custom_entities,
                    feature_cache=args.feature_cache,
                    custom_intents=args.custom_intents)


if __name__ == '__main__':
    do_main()
