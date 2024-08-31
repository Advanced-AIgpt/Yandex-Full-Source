# -*- coding: utf-8 -*-

import argparse
import logging
import os
import shutil
import tempfile
import re
import pickle

# Imports used only for typing
from typing import Mapping, List, Tuple, Dict, AnyStr, NoReturn  # noqa: F401
from vins_core.nlu.token_tagger import TokenTagger  # noqa: F401
from vins_core.nlu.base_nlu import FeatureExtractorResult  # noqa: F401

from vins_core.utils.data import TarArchive

from dataset import VinsDataset
from vins_config_tools import (open_model_archive, update_checksum, load_train_config)
from vins_core.config.app_config import load_app_config

from vins_core.nlu.token_tagger import RnnTaggerTrainer

logger = logging.getLogger(__name__)


# Tagger trainer factory
_tagger_trainer_factories = {}


def register_token_tagger_trainer_type(tagger_type, name):
    _tagger_trainer_factories[name] = tagger_type


def create_token_tagger_trainer(model, **kwargs):
    assert model is not None, 'A model must be specified in the token tagger config'
    if model not in _tagger_trainer_factories:
        raise ValueError('Unknown tagger model: %s' % model)

    return _tagger_trainer_factories[model](**kwargs)


register_token_tagger_trainer_type(RnnTaggerTrainer, 'rnn_new')


def _train_single_tagger(model, features):
    # type: (AnyStr, List[SampleFeatures]) -> TokenTagger

    trainer = create_token_tagger_trainer(model=model)
    if isinstance(features, Mapping):  # intent-to-samples map is given
        features = sum(features.itervalues(), [])
    trainer.fit(features, None)

    return trainer.convert_to_applier()


def _train_taggers(model, intent_to_fer, intents_to_train=None):
    # type: (AnyStr, Dict[AnyStr, FeatureExtractorResult], AnyStr) -> Dict[AnyStr, TokenTagger]

    logger.info('Training taggers')
    taggers = {}

    compiled_re = re.compile(intents_to_train) if intents_to_train else None
    if compiled_re:
        logger.info('Try to find intents matched to "%s"', intents_to_train)

    for intent, feature_extractor_results in intent_to_fer.iteritems():
        if compiled_re and not compiled_re.match(intent):
            logger.info('Skip intent %s', intent)
            continue

        logger.info('Start fitting tagger for intent %s', intent)

        features = []
        for result in feature_extractor_results:
            if result.item.can_use_to_train_tagger:
                features.append(result.sample_features)

        taggers[intent] = _train_single_tagger(model, features)

    return taggers


def _save_taggers_to_tar_archive(app_name, name, taggers, from_scratch=False):
    # type: (AnyStr, AnyStr, Dict[AnyStr, TokenTagger]) -> NoReturn

    logger.info('Saving taggers to model archive')
    temp_dir = None
    try:
        temp_dir = tempfile.mkdtemp()
        app_config = load_app_config(app_name)
        with open_model_archive(app_config, TarArchive) as archive:
            archive.extract_all(temp_dir)

        with open_model_archive(app_config, TarArchive, 'w') as archive:
            for subdir_name in os.listdir(temp_dir):
                if subdir_name != name:
                    archive.add(subdir_name, os.path.join(temp_dir, subdir_name))
                    continue

            with archive.nested(name) as tagger_archive:
                tmp = tagger_archive.get_tmp_dir()
                for intent, model in taggers.iteritems():
                    logger.info('Saving tagger for intent %s', intent)
                    model.save(os.path.join(tmp, intent))

                if not from_scratch:
                    taggers_data_dir = os.path.join(temp_dir, name, name + '.data')
                    for tagger_name in os.listdir(taggers_data_dir):
                        if tagger_name not in taggers.keys():
                            logger.info('Keep existing tagger for intent %s', tagger_name)
                            shutil.move(os.path.join(taggers_data_dir, tagger_name), tmp)

                tagger_archive.add(name + '.data', tmp)

                intent_conditioned = True
                tagger_meta_info_path = os.path.join(tmp, 'meta.pkl')
                with open(tagger_meta_info_path, 'wb') as f:
                    pickle.dump(intent_conditioned, f, pickle.HIGHEST_PROTOCOL)

                tagger_archive.add(name + '.info', tagger_meta_info_path)
    finally:
        if temp_dir and os.path.isdir(temp_dir):
            shutil.rmtree(temp_dir)

    new_sha256 = update_checksum(app_name)
    logger.info('updated sha256sum is "%s"', new_sha256)


def train_taggers(config_path, app_name, dataset_path, intents_to_train, from_scratch):
    # type: (AnyStr, AnyStr, AnyStr, AnyStr, AnyStr, bool) -> NoReturn

    logger.info('Loading dataset from ' + dataset_path)
    dataset = VinsDataset.restore(dataset_path)

    config = load_train_config(config_path, 'utterance_tagger')
    intent_to_fer = dataset.convert_items_to_fer('tagger')

    taggers = _train_taggers(
        model=config['model'],
        intent_to_fer=intent_to_fer,
        intents_to_train=intents_to_train
    )

    _save_taggers_to_tar_archive(app_name=app_name, name='tagger', taggers=taggers, from_scratch=from_scratch)


def do_main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--train-dataset-path', type=str, required=True, help='Path to dataset')
    parser.add_argument('--intents-to-train', type=str, dest='intents_to_train',
                        help='Specify regexp to filter intents for which we should train taggers')
    parser.add_argument('--from-scratch', action='store_true', default=False,
                        help='Left only trained taggers, clear old from archive')
    parser.add_argument('--config', type=str, required=True, help='Config with tagger training params')
    parser.add_argument('--app-name', default='personal_assistant.app', help='Application name')

    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    train_taggers(config_path=args.config,
                  app_name=args.app_name,
                  dataset_path=args.train_dataset_path,
                  intents_to_train=args.intents_to_train,
                  from_scratch=args.from_scratch)


if __name__ == '__main__':
    do_main()
