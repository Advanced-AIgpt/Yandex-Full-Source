# -*- coding: utf-8 -*-

import argparse
import logging
import os
import shutil
import tempfile

# Imports used only for typing
from typing import Mapping, List, Tuple, Dict, AnyStr, NoReturn  # noqa: F401
from vins_core.nlu.samples_extractor import SamplesExtractor  # noqa: F401
from vins_core.nlu.features_extractor import FeaturesExtractor  # noqa: F401
from vins_core.nlu.base_nlu import IntentInfo  # noqa: F401
from token_classifier import TokenClassifier  # noqa: F401

from vins_core.nlu.flow_nlu_factory.transition_model import create_transition_model
from token_classifier import create_token_classifier
from vins_core.nlu.token_classifier import create_token_classifier as create_token_classifier_old
from vins_core.nlu.neural.metric_learning.metric_learning import TrainMode as MetricTrainMode
from vins_core.utils.data import TarArchive

from dataset import VinsDataset
from data_loaders import load_extractors
from vins_config_tools import (open_model_archive, update_checksum, load_classifier_config,
                               get_intent_config_paths, collect_intent_infos)
from vins_core.config.app_config import load_app_config

logger = logging.getLogger(__name__)


def create_classifier(config, intent_infos=None, samples_extractor=None, features_extractor=None, **kwargs):
    # type: (Mapping, Mapping[AnyStr, IntentInfo], SamplesExtractor, FeaturesExtractor, ...) -> TokenClassifier

    def create(creator):
        return creator(
            model=config['model'],
            select_features=config.get('features', ()),
            samples_extractor=samples_extractor,
            intent_infos=intent_infos,
            features_extractor_info=(
                features_extractor.classifiers_info if features_extractor is not None else None
            ),
            **kwargs
        )

    classifier = create(create_token_classifier)
    if classifier is None:
        classifier = create(create_token_classifier_old)
    assert classifier is not None, "Unable to find classifier {}".format(config['model'])
    return classifier


def _train_scenarios_classifier(app_name, classifier_name, dataset, is_metric_learning=False, load_model=False):
    # type: (AnyStr, AnyStr, VinsDataset, bool, bool) -> TokenClassifier

    app_config = load_app_config(app_name)
    classifier_conf = load_classifier_config(app_config, classifier_name)
    intent_infos, intents = collect_intent_infos(app_config, get_intent_config_paths(app_name, is_metric_learning))

    samples_extractor, features_extractor, _ = load_extractors(
        app_config, features_extractor_list=['emb_ids']
    )

    params = classifier_conf.get('params', {})
    if is_metric_learning:
        params['metric_learning'] = MetricTrainMode.METRIC_LEARNING_FROM_SCRATCH
    else:
        params['metric_learning'] = MetricTrainMode.NO_METRIC_LEARNING

    classifier = create_classifier(
        classifier_conf, intent_infos=intent_infos, samples_extractor=samples_extractor,
        features_extractor=features_extractor, **params
    )
    if load_model:
        with open_model_archive(app_config) as archive:
            with archive.nested('classifiers') as arch:
                try:
                    logger.debug('Loading from archive...')
                    classifier.load(arch, classifier_name)
                except Exception as e:
                    logger.error('Data for %s is not loaded. Reason: %s', classifier_name, e.message)

    transition_model_config = app_config.nlu.get('transition_model', dict())
    transition_model = create_transition_model(
        intents=intents, **transition_model_config)

    target_classifier_name = classifier_name + '_metric' if is_metric_learning else classifier_name
    logger.info('Collecting samples from dataset for classifier %s', target_classifier_name)
    intent_to_features = dataset.to_intent_to_sample_features(target_classifier_name)

    logger.info('Training classifier')
    classifier.train(intent_to_features=intent_to_features,
                     transition_model=transition_model and transition_model.model,
                     intent_infos=intent_infos)
    return classifier


def _save_classifier_to_tar_archive(app_name, classifier_name, classifier):
    # type: (AnyStr, AnyStr, TokenClassifier) -> NoReturn

    logger.info('Saving the model')
    temp_dir = None
    try:
        temp_dir = tempfile.mkdtemp()
        app_config = load_app_config(app_name)
        with open_model_archive(app_config, TarArchive) as archive:
            archive.extract_all(temp_dir)

        with open_model_archive(app_config, TarArchive, 'w') as archive:
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

    new_sha256 = update_checksum(app_name)
    logger.info('updated sha256sum is "%s"', new_sha256)


def train_scenarios_classifier(app_name, classifier_name, dataset_path, train_metric=False):
    # type: (AnyStr, AnyStr, AnyStr, bool) -> NoReturn

    logger.info('Loading dataset from ' + dataset_path)
    dataset = VinsDataset.restore(dataset_path)

    if train_metric:
        logger.info('Metric training is started')
        try:
            _train_scenarios_classifier(app_name=app_name, classifier_name=classifier_name,
                                        is_metric_learning=True, dataset=dataset)
        except KeyboardInterrupt:
            logger.info('Metric training was interrupted (hope you gave the beast a chance to converge)')

    logger.info('Building the KNN index is started')
    classifier = _train_scenarios_classifier(
        app_name=app_name, classifier_name=classifier_name, is_metric_learning=False, dataset=dataset
    )

    # TODO: does anyone want an ability to save the classifier to other types of archive?
    _save_classifier_to_tar_archive(app_name, classifier_name, classifier)


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--train-dataset-path', type=str, required=True, help='Path to dataset')
    parser.add_argument('--train-metric', action='store_true', default=False, help='Whether to train metric before KNN')
    parser.add_argument('--app-name', default='personal_assistant.app', help='Application name')
    parser.add_argument('--classifier-name', type=str, default='scenarios', help='Use intents for this classifier')

    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    train_scenarios_classifier(app_name=args.app_name, classifier_name=args.classifier_name,
                               dataset_path=args.train_dataset_path, train_metric=args.train_metric)


if __name__ == '__main__':
    main()
