#!/usr/bin/env python
# coding: utf-8

from __future__ import unicode_literals

import logging.config
import codecs
import json
import click
import os

from vins_core.dm.formats import NluWeightedString
from vins_core.config.app_config import Entity
from vins_core.ner.fst_normalizer import DEFAULT_RU_NORMALIZER_NAME
from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.sample_processors.normalize import NormalizeSampleProcessor
from vins_tools.nlu.ner.fst_custom import NluFstCustomMorphyConstructor


click.disable_unicode_literals_warning = True
logger = logging.getLogger(__name__)


def make_custom_typed_nlu_fst(in_type_name, in_config_file, inflect_numbers, output_dir):
    samples_extractor = SamplesExtractor([NormalizeSampleProcessor(DEFAULT_RU_NORMALIZER_NAME)])

    entity = Entity(
        name=in_type_name,
        samples=in_config_file,
        inflect_numbers=inflect_numbers
    )

    input_data = {}
    for entity_canon, entity_synonims in entity.samples.iteritems():
        samples = samples_extractor(entity_synonims, filter_errors=True)
        input_data[entity_canon] = map(lambda sample: NluWeightedString(sample.text, sample.weight), samples)

    custom_entity_parser = NluFstCustomMorphyConstructor(
        fst_name=in_type_name,
        data=input_data,
        inflect_numbers=entity.inflect_numbers
    ).compile()

    custom_entity_parser.save(path=os.path.join(output_dir, in_type_name))


@click.command()
@click.argument('type_name')
@click.argument('config_json')
@click.option('--inflect-numbers', is_flag=True, default=False)
@click.option('-o', '--output', required=True)
def main(type_name, config_json, inflect_numbers, output):
    """This script builds fst for custom ner type."""
    logging.config.dictConfig({
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'standard': {
                'format': '[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s',
            },
            'colored': {
                '()': 'vins_core.logger.color_formatter.ColorFormatter',
                'format': '$COLOR[%(asctime)s] [%(request_id)s] [%(module)s:%(lineno)d] [%(levelname)s] %(message)s',
                'datefmt': '%Y-%m-%d_%H:%M:%S'
            }
        },
        'handlers': {
            'console': {
                'class': 'logging.StreamHandler',
                'level': 'DEBUG',
                'formatter': 'standard',
            }
        },
        'loggers': {
            '': {
                'handlers': ['console'],
                'level': 'DEBUG',
                'propagate': True,
            },
        },
    })

    with codecs.open(config_json, 'r', encoding='utf-8') as f:
        make_custom_typed_nlu_fst(
            type_name,
            json.load(f),
            inflect_numbers,
            output
        )


if __name__ == '__main__':
    main()
