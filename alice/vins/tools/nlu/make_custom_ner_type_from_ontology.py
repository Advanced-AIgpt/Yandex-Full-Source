#!/usr/bin/env python
# coding: utf-8

from __future__ import unicode_literals

import logging.config
import click
import os

from vins_core.dm.formats import NluWeightedString
from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.sample_processors.normalize import NormalizeSampleProcessor
from vins_core.config.app_config import Entity
from vins_tools.nlu.ner.fst_custom import NluFstCustomMorphyConstructor
from personal_assistant.ontology.onto import Ontology

click.disable_unicode_literals_warning = True
logger = logging.getLogger(__name__)


def make_custom_typed_nlu_fst(entity_name, ontology, inflect_numbers, output):
    samples_extractor = SamplesExtractor([NormalizeSampleProcessor()])

    onto_entity = ontology[entity_name]

    entity = Entity(
        name=entity_name,
        samples={
            i.name: samples_extractor([p.text for p in i.phrases])
            for i in onto_entity.instances
        },
        inflect_numbers=inflect_numbers
    )

    input_data = {}
    for entity_canon, entity_synonims in entity.samples.iteritems():
        input_data[entity_canon] = map(lambda sample: NluWeightedString(sample.text, sample.weight), entity_synonims)

    custom_entity_parser = NluFstCustomMorphyConstructor(
        fst_name=entity_name,
        data_file=input_data,
        inflect_numbers=entity.inflect_numbers
    ).compile()

    custom_entity_parser.save(path=os.path.join(output, entity_name))


@click.command()
@click.argument('entity_name')
@click.argument('ontology_json')
@click.option('--inflect-numbers', is_flag=True, default=False)
@click.option('-o', '--output', required=True)
def main(entity_name, ontology_json, inflect_numbers, output):
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

    onto = Ontology.from_file(ontology_json)

    make_custom_typed_nlu_fst(
        entity_name,
        onto,
        inflect_numbers,
        output
    )


if __name__ == '__main__':
    main()
