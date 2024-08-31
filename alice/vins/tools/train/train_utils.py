# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from vins_core.config.app_config import AppConfig
from vins_core.nlu.features.cache import factory
from vins_core.nlu.flow_nlu import FlowNLU

logger = logging.getLogger(__name__)


def check_feature_cache_consistency(feature_cache, app_conf, vinsfile_path=None):
    """ If the feature cache exists, load it and check feature list.
    If it does not, just print the list of features without actually saving the new cache on disk.
    """
    if feature_cache is not None:
        logger.info('Checking feature cache {} for consistency with vins config'.format(feature_cache))

        if app_conf is None:
            app_conf = AppConfig.from_vinsfile(vinsfile_path)

        features_extractor = FlowNLU.create_features_extractor(app_conf.nlu['feature_extractors'])
        # the logic with wizard_ce mimics that in vins_core.nlu.custom_entities_tools.load_mixed_parsers
        wizard_ce = app_conf.nlu.get('wizard_custom_entities', [])
        entities = [entity.name for entity in app_conf.entities if entity.name not in wizard_ce]
        if len(wizard_ce) > 0:
            entities.append('wizard_custom_entities')

        sample_features_cache = factory.FeatureCacheFactory.create(feature_cache)
        sample_features_cache.check_consistency(features_extractor, entities)

        logger.info('Everything is fine')
