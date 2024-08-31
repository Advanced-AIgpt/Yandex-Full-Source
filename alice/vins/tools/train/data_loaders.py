# -*- coding: utf-8 -*-

"""A set of tools used for dataset collection."""

import codecs
import pandas as pd
from collections import defaultdict

# Imports used only for typing
from typing import Mapping, List, Tuple, Dict, Iterable, AnyStr, NoReturn  # noqa: UnusedImport

from vins_core.nlu.features_extractor import FeaturesExtractor  # noqa: UnusedImport
from vins_core.nlu.template_entities import TemplateEntitiesFormat  # noqa: UnusedImport
from vins_core.config.app_config import AppConfig, Entity, load_app_config  # noqa: UnusedImport
from vins_core.ner.fst_base import NluFstBase  # noqa: UnusedImport
from vins_core.ner.fst_parser import NluFstParser  # noqa: UnusedImport

from vins_core.config.intent_config_loaders import parse_intent_configs, iterate_nlu_sources_for_intent
from vins_core.dm.formats import FuzzyNLUFormat, NluSourceItem
from vins_core.ner.fst_presets import FstParserFactory
from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.base_nlu import FeatureExtractorFromItem
from vins_core.nlu.features.cache import factory
from vins_core.nlu.custom_entities_tools import load_mixed_parsers
from vins_core.nlu.features_extractor import FeaturesExtractorFactory
from vins_core.utils.misc import call_once_on_dict

from vins_tools.nlu.ner.fst_custom import build_custom_entity_parser

from vins_config_tools import open_model_archive, iterate_intents_from_config


def load_extractors(app_conf, samples_extractor_list=None, features_extractor_list=None, load_custom_entities=True):
    # type: (AppConfig, Iterable[AnyStr], Iterable, bool) -> Tuple[SamplesExtractor, FeaturesExtractor, NluFstParser]

    samples_extractor_configs = app_conf.samples_extractor

    if samples_extractor_list is not None:
        samples_extractor_configs['pipeline'] = [
            config for config in samples_extractor_configs['pipeline'] if config['name'] in samples_extractor_list
        ]

    samples_extractor = SamplesExtractor.from_config(samples_extractor_configs)
    features_extractor, custom_entity_parsers = _load_features_extractor(app_conf, features_extractor_list,
                                                                         samples_extractor, load_custom_entities)
    return samples_extractor, features_extractor, custom_entity_parsers


def _load_features_extractor(app_conf, features_extractor_list, samples_extractor, load_custom_entities):
    # type: (AppConfig, Iterable[Mapping], SamplesExtractor, bool) -> Tuple[FeaturesExtractor, NluFstParser]

    features_extractors_configs = app_conf.nlu.get('feature_extractors', {})
    if features_extractor_list:
        features_extractors_configs = [
            config for config in features_extractors_configs if config['id'] in features_extractor_list
        ]

    if app_conf.nlu.get('compiled_model') and load_custom_entities:
        with open_model_archive(app_conf) as archive:
            custom_entity_parsers = load_mixed_parsers(app_conf, archive)
    else:
        custom_entity_parsers = _build_custom_entity_parsers(app_conf.entities, samples_extractor)

    fe_factory = FeaturesExtractorFactory()

    fst_config = app_conf.nlu.get('fst', {})
    fst_parser_factory = FstParserFactory.from_config(fst_config)
    fst_parser_factory.load()

    custom_entity_parser = fst_parser_factory.create_parser(
        fst_config.get('parsers', []),
        additional_parsers=custom_entity_parsers.values()
    )

    fe_factory.register_parser(custom_entity_parser)

    for conf in features_extractors_configs:
        fe_factory.add(**conf)

    return fe_factory.create_extractor(), custom_entity_parsers


def _build_custom_entity_parsers(entity_confs, samples_extractor):
    # type: (List[Entity], SamplesExtractor) -> Dict[AnyStr, NluFstBase]

    parsers = {}
    for entity in entity_confs:
        samples = call_once_on_dict(samples_extractor, entity.samples, is_inference=False)
        parsers[entity.name] = build_custom_entity_parser(
            entity.name, entity_samples=samples, entity_inflect_info=entity.inflect_info
        )
    return parsers


def load_source_data_by_config(app_name, config_paths=None):
    # type: (AnyStr, Iterable[AnyStr]) -> Dict[AnyStr, List[NluSourceItem]]

    app_conf = load_app_config(app_name)

    intent_configs = parse_intent_configs(iterate_intents_from_config(app_conf, config_paths))

    input_data = defaultdict(list)
    for intent_config in intent_configs:
        for nlu_source in iterate_nlu_sources_for_intent(intent_config, app_conf.nlu_templates):
            input_data[intent_config.name].extend(nlu_source.load())

    return input_data


def load_source_data_from_text_file(path, intent, trainable_classifiers=(), can_use_to_train_tagger=False):
    # type: (AnyStr, AnyStr, Iterable[AnyStr], bool) -> Dict[AnyStr, List[NluSourceItem]]

    items = []
    with codecs.open(path, encoding='utf8') as f:
        for line in f:
            text = line.strip()
            items.append(NluSourceItem(text=text, original_text=text, slots=[],
                                       trainable_classifiers=trainable_classifiers,
                                       can_use_to_train_tagger=can_use_to_train_tagger))
    return {intent: items}


def load_source_data_from_nlu(path,                          # type: AnyStr
                              intent,                        # type: AnyStr
                              nlu_templates,                 # type: Mapping[AnyStr, TemplateEntitiesFormat]
                              trainable_classifiers=(),      # type: Iterable[AnyStr]
                              can_use_to_train_tagger=False  # type: bool
                              ):
    # type: (...) -> Dict[AnyStr, List[NluSourceItem]]

    with codecs.open(path, encoding='utf8') as f_in:
        items = FuzzyNLUFormat.parse_iter(utterances=f_in,
                                          templates=nlu_templates,
                                          trainable_classifiers=trainable_classifiers,
                                          can_use_to_train_tagger=can_use_to_train_tagger).items
    return {intent: items}


def load_source_data_from_tsv(path, text_col='text', intent_col='intent',
                              trainable_classifiers=(), can_use_to_train_tagger=False):
    # type: (AnyStr, AnyStr, AnyStr, Iterable[AnyStr], bool) -> Dict[AnyStr, List[NluSourceItem]]

    data = pd.read_csv(path, sep='\t')

    result = defaultdict(list)
    for _, row in data.iterrows():
        result[row[intent_col]].append(NluSourceItem(
            text=row[text_col], original_text=row[text_col], slots=[],
            trainable_classifiers=trainable_classifiers,
            can_use_to_train_tagger=can_use_to_train_tagger)
        )
    return result


def extract_features(intent_to_source_data,    # type: Mapping[AnyStr, Iterable[NluSourceItem]]
                     samples_extractor,        # type: SamplesExtractor
                     features_extractor,       # type: FeaturesExtractor
                     custom_entity_parsers,    # type: List
                     classifiers_list,         # type: Iterable[AnyStr]
                     feature_cache_path=None,  # type: AnyStr
                     train_taggers=True        # type: bool
                     ):
    # type: (...) -> Dict[AnyStr, List]

    sample_features_cache = None
    if feature_cache_path:
        sample_features_cache = factory.FeatureCacheFactory.create(feature_cache_path)
        sample_features_cache.check_consistency(features_extractor, custom_entity_parsers.keys())

    if sample_features_cache and not sample_features_cache.UPDATABLE:
        def _get_items_from_cache(items):
            return [
                sample_features_cache[item] for item in items
                if item in sample_features_cache
            ]
        output = {}
        for key, items in intent_to_source_data.iteritems():
            cached_items = _get_items_from_cache(items)
            if cached_items:
                output[key] = cached_items
    else:
        output = call_once_on_dict(
            function=FeatureExtractorFromItem(
                samples_extractor=samples_extractor,
                features_extractor=features_extractor,
                feature_cache=sample_features_cache,
                trainable_classifiers=classifiers_list,
                train_taggers=train_taggers
            ),
            mappable=intent_to_source_data,
            filter_errors=False
        )
    return output
