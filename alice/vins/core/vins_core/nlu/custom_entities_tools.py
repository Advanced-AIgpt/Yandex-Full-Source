# -*- coding: utf-8 -*-

import logging

from vins_core.ner.custom import CustomEntityParser
from vins_core.ner.wizard import NluWizardAliceTypeParserTime

CUSTOM_ENTITIES_DIRNAME = 'custom_entities'
ALL_CUSTOM_ENTITIES = 'all'

logger = logging.getLogger(__name__)


def load_custom_entity_parser(fst_name, entity_names, archive):
    entity = CustomEntityParser.load_from_archive(
        fst_name=fst_name,
        archive=archive,
        entity_names=entity_names
    )

    logger.info("Custom entity %s has been loaded from archive", fst_name)

    return entity


def load_custom_entities(entity_names, archive):
    return {ALL_CUSTOM_ENTITIES: load_custom_entity_parser(ALL_CUSTOM_ENTITIES, entity_names, archive)}


def load_mixed_parsers(app_cfg, archive):
    entity_names = {entity.name for entity in app_cfg.entities}

    logger.info('Loading custom entities')
    with archive.nested(CUSTOM_ENTITIES_DIRNAME) as arch:
        parsers = load_custom_entities(entity_names, arch)

    parsers['wizard_time'] = NluWizardAliceTypeParserTime()
    return parsers
