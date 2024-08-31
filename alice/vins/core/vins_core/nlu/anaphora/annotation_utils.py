# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from vins_core.nlu.samples_extractor import SamplesExtractorError

logger = logging.getLogger(__name__)


def rename_response_to_turn_annotations(annotations):
    rename_annotations(annotations, {
        'entitysearch': 'response_entitysearch',
        'wizard': 'response_wizard',
    })


def rename_turn_to_response_annotations(annotations):
    rename_annotations(annotations, {
        'response_entitysearch': 'entitysearch',
        'response_wizard': 'wizard',
    })


def rename_annotations(annotations, rename_rules):
    for current_key, new_key in rename_rules.iteritems():
        annotations.delete(new_key)
        if current_key in annotations:
            annotations[new_key] = annotations[current_key]
            annotations.delete(current_key)


def prepare_anaphora_response_annotations(annotations):
    if annotations and 'entitysearch' in annotations:
        annotations['entitysearch'].entities = filter_entities(annotations['entitysearch'].entities)


def has_anaphora_annotations(annotations):
    return annotations and 'wizard' in annotations and 'entitysearch' in annotations


def extract_anaphora_annotations(sample, samples_extractor, is_response):
    extractor_part = ['wizard', 'entitysearch']
    try:
        logger.debug('Extracting entities from response: {}'.format(sample.text))
        sample = samples_extractor([sample], sample_processor_names=extractor_part)[0]
        if is_response:
            prepare_anaphora_response_annotations(sample.annotations)
    except SamplesExtractorError:
        logger.warning('Application`s SamplesExtractor does not have necessary sample processors '
                       'to run entity search, entities from the response will not be extracted.')
    return sample


def filter_entities(entity_list):
    if entity_list:
        first_entity = min(entity_list, key=lambda entity: entity.start)
        return [first_entity]
    else:
        return []
