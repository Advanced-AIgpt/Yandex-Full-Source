# coding: utf-8
from vins_core.config import AppConfig
from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.utils.data import find_vinsfile
from vins_core.utils.misc import call_once_on_dict

from vins_tools.nlu.ner.fst_custom import build_custom_entity_parser


LITE_SAMPLES_EXTRACTOR_RU = {'pipeline': [{'normalizer': 'normalizer_ru', 'name': 'normalizer'}]}


def get_extractor_and_entities_for_app(app_name):
    vinsfile = find_vinsfile(app_name)
    app_conf = AppConfig()
    app_conf.parse_vinsfile(vinsfile)

    if app_name == 'personal_assistant':
        extractor_config = LITE_SAMPLES_EXTRACTOR_RU
    else:
        extractor_config = app_conf.samples_extractor
    samples_extractor = SamplesExtractor.from_config(extractor_config)
    entities = app_conf.entities
    return samples_extractor, entities


def compile_custom_entities(entities, samples_extractor):
    parsers = {}
    for entity in entities:
        samples = (
            call_once_on_dict(samples_extractor, entity.samples, is_inference=False)
        )

        # create parser from samples
        parsers[entity.name] = build_custom_entity_parser(
            entity_name=entity.name, entity_samples=samples, entity_inflect_info=entity.inflect_info
        )

    return parsers


def save_parsers_to_archive(parsers, archive):
    for parser in parsers.itervalues():
        parser.save_to_archive(archive)


def compile_custom_entities_for_app(app_name):
    extractor, entities = get_extractor_and_entities_for_app(app_name=app_name)
    parsers = compile_custom_entities(entities, extractor)
    return parsers
