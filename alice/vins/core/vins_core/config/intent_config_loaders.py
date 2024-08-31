# -*- coding: utf-8 -*-

import logging
import attr

from vins_core.dm.intent import Intent
from vins_core.dm.nlu_sources import create_nlu_source


logger = logging.getLogger(__name__)


@attr.s(frozen=True)
class IntentConfig(object):
    intent = attr.ib()
    name = attr.ib()
    prior = attr.ib()
    tagger_data_keys = attr.ib()
    fallback = attr.ib()
    fallback_threshold = attr.ib()
    total_fallback = attr.ib()
    nlu = attr.ib()
    trainable_classifiers = attr.ib()
    positive_sampling = attr.ib()
    negative_sampling_from = attr.ib()
    negative_sampling_for = attr.ib()
    scenarios = attr.ib()
    utterance_tagger = attr.ib()
    allowed_prev_intents = attr.ib()


def parse_intent_configs(intent_configs):
    output = []
    for intent_config in intent_configs:
        intent, tagger_data_keys = _create_intent_from_config(intent_config)
        output.append(IntentConfig(
            intent,
            intent.name,
            intent.prior,
            tagger_data_keys,
            intent_config.fallback,
            intent_config.fallback_threshold,
            intent_config.total_fallback,
            intent_config.nlu,
            intent_config.trainable_classifiers,
            intent_config.positive_sampling,
            intent_config.negative_sampling_from,
            intent_config.negative_sampling_for,
            intent_config.scenarios,
            intent_config.utterance_tagger,
            intent_config.allowed_prev_intents
        ))
    return output


def _create_intent_from_config(intent_config):
    intent = Intent.from_config(intent_config)

    tagger_data_keys = [intent.name]

    # intent inheritance logic
    if intent.parent_name and intent_config.parent_examples_in_tagger:
        tagger_data_keys.append(intent.parent_name)

    return intent, tagger_data_keys


def iterate_nlu_sources_for_intent(intent_config, nlu_templates):
    for nlu_source_config in intent_config.nlu.config:
        logger.info('Start configuring NLU data for intent %s', intent_config.name)
        nlu_source = create_nlu_source(
            source_type=nlu_source_config['source'],
            config=nlu_source_config,
            nlu_templates=nlu_templates,
            trainable_classifiers=nlu_source_config.get(
                'trainable_classifiers', intent_config.trainable_classifiers)
        )
        yield nlu_source
