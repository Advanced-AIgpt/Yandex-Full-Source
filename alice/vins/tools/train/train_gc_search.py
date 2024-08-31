# -*- coding: utf-8 -*-

import argparse
import logging
import os

# Imports used only for typing
from typing import Mapping, List, Tuple, Dict, AnyStr, NoReturn  # noqa: F401
from token_classifier import TokenClassifier  # noqa: F401

from token_classifier import create_token_classifier
from vins_core.utils.data import TarArchive

from data_loaders import load_extractors
from vins_config_tools import load_app_config, load_feature_extractor_config
from vins_core.dm.formats import NluSourceItem

logger = logging.getLogger(__name__)


_EXTRACTOR_NAME = 'gc_search'
_MODELS_DATA_DIR = '../../core/vins_core/data'


def _collect_features(path, samples_extractor, feature_extractor):
    with open(path) as f:
        samples = samples_extractor(
            [NluSourceItem(text=(line.split('\t')[0])) for line in f.readlines()]
        )
        return feature_extractor(samples)


def _get_class_to_features(datasets_paths, samples_extractor, feature_extractor):
    logger.info('Collecting samples')
    class_to_features = {
        cls: _collect_features(
            path,
            samples_extractor,
            feature_extractor
        ) for cls, path in datasets_paths.iteritems()
    }
    return class_to_features


def _train_extractor(app_config, extractor_config, datasets_paths):
    # type: (AppConfig, Dict, VinsDataset) -> TokenClassifier

    extractor = create_token_classifier(model=extractor_config['model'])

    samples_extractor, feature_extractor, _ = load_extractors(
        app_config,
        features_extractor_list=[]
    )
    logger.info('Loading dataset from ' + ', '.join(datasets_paths.itervalues()))
    class_to_features = _get_class_to_features(datasets_paths, samples_extractor, feature_extractor)

    logger.info('Training gc_search')
    extractor.train(intent_to_features=class_to_features)
    return extractor


def _save_extractor_to_tar_archive(config, extractor, model_file=None):
    # type: (Dict, TokenClassifier) -> NoReturn

    logger.info('Saving the model')
    if not model_file:
        model_file = os.path.join(_MODELS_DATA_DIR, config['model_file'])

    with TarArchive(model_file, 'w') as archive:
        model_dir = config['model']
        extractor.save(archive, model_dir)


def train_gc_search(app_name, datasets_paths, model_file=None):
    # type: (AnyStr, AnyStr) -> NoReturn

    app_config = load_app_config(app_name)
    extractor_config = load_feature_extractor_config(app_config, _EXTRACTOR_NAME)

    extractor = _train_extractor(
        app_config=app_config,
        extractor_config=extractor_config,
        datasets_paths=datasets_paths
    )
    _save_extractor_to_tar_archive(
        config=extractor_config,
        extractor=extractor,
        model_file=model_file
    )


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--gc-samples-path', type=str, required=True, help="Path to samples with 'gc' class")
    parser.add_argument('--search-samples-path', type=str, required=True, help="Path to samples with 'search' class")
    parser.add_argument('--app-name', default='personal_assistant.app', help='Application name')
    parser.add_argument(
        '--model-file',
        default=None,
        help='Where to save model. Saved to vins_core/data/<file name from config> by default.'
    )

    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    train_gc_search(
        app_name=args.app_name,
        datasets_paths={'gc': args.gc_samples_path, 'search': args.search_samples_path},
        model_file=args.model_file
    )


if __name__ == '__main__':
    main()
