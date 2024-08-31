# -*- coding: utf-8 -*-

"""A set of tools used for loading some stuff by app config."""

import codecs
import json

# Import used only for typing
from typing import AnyStr, Iterable, Generator, NoReturn, Dict, Optional, List  # noqa: UnusedImport

from vins_core.utils import archives
from vins_core.utils.archives import Archive  # noqa: UnusedImport

from vins_core.config.app_config import AppConfig, Project, load_app_config  # noqa: UnusedImport
from vins_core.utils.data import find_vinsfile, get_checksum, get_resource_full_path, load_data_from_file
from vins_core.nlu.classifiers_cascade import ClassifiersCascade
from vins_core.config.intent_config_loaders import parse_intent_configs
from vins_core.nlu.base_nlu import IntentInfo


def iterate_intents_from_config(app_conf, config_paths=None):
    # type: (AppConfig, Iterable[AnyStr]) -> Generator[AnyStr, None, None]

    if config_paths is None:
        for intent in app_conf.intents:
            yield intent
    else:
        for config_path in config_paths:
            project_dict = load_data_from_file(config_path)
            project = Project.from_dict(
                project_dict,
                base_name=app_conf.projects[0].name,
                nlu_templates=app_conf.nlu_templates
            )
            for intent in project.intents:
                yield intent


def _get_model_archive_path(app_conf):
    # type: (AppConfig) -> AnyStr
    return get_resource_full_path(app_conf.nlu['compiled_model']['path'])


def open_model_archive(app_conf, archiver=None, mode='r:*'):
    # type: (AppConfig, Archive, AnyStr) -> Archive

    model = app_conf.nlu['compiled_model']
    path = _get_model_archive_path(app_conf)

    archiver = archiver or getattr(archives, model['archive'])
    return archiver(path=path, mode=mode, sha256=model.get('sha256'))


def update_checksum(app_name):
    # type: (AnyStr) -> AnyStr

    vins_file = find_vinsfile(app_name)
    model_file = _get_model_archive_path(load_app_config(app_name))
    sha = get_checksum(model_file)

    with codecs.open(vins_file, 'r', encoding='utf8') as f:
        vins_file_data = json.load(f)

    vins_file_data['nlu']['compiled_model']['sha256'] = sha
    with codecs.open(vins_file, 'w', encoding='utf8') as f:
        json.dump(vins_file_data, f, sort_keys=False, ensure_ascii=False, indent=2, separators=(',', ': '))
        f.write('\n')

    return sha


def load_feature_extractor_config(app_conf, extractor_id):
    # type: (AppConfig, AnyStr) -> Dict

    for extractor in app_conf.nlu['feature_extractors']:
        if extractor['id'] == extractor_id:
            return extractor

    assert False, 'Config for extractor {} not found'.format(extractor_id)


def load_classifier_config(app_conf, classifier_name):
    # type: (AppConfig, AnyStr) -> Dict

    for conf in ClassifiersCascade._iterate_classifiers_configs(app_conf.nlu.get('intent_classifiers', {})):
        if conf['name'] == classifier_name:
            return conf

    assert False, 'Config for classifier {} not found'.format(classifier_name)


def load_tagger_config(app_conf):
    # type: (AppConfig) -> Dict

    tagger_config = app_conf.nlu.get('utterance_tagger', None)

    assert tagger_config is not None, 'Config for utterance tagger not found'
    return tagger_config


def get_intent_config_paths(app_name, is_metric_learning):
    # type: (AnyStr, bool) -> Optional[List[AnyStr]]

    if app_name == 'personal_assistant.app' and is_metric_learning:
        # TODO: should not the thing be customized somehow?
        return ['personal_assistant/config/scenarios/scenarios.mlconfig.json',
                'personal_assistant/config/handcrafted/handcrafted.mlconfig.json',
                'personal_assistant/config/stroka/stroka.mlconfig.json',
                'personal_assistant/config/navi/navi.mlconfig.json']
    elif app_name == 'crm_bot.app' and is_metric_learning:
        return ['crm_bot/config/scenarios/scenarios.mlconfig.json']
    else:
        return None


def collect_intent_infos(app_conf, config_paths=None):
    # type: (AppConfig, List[AnyStr]) -> Tuple[Dict[AnyStr, IntentInfo], List[AnyStr]]

    intent_configs = parse_intent_configs(iterate_intents_from_config(app_conf, config_paths))

    intent_infos = {}
    for intent_config in intent_configs:
        intent_infos[intent_config.name] = IntentInfo(
            prior=intent_config.prior,
            fallback_threshold=intent_config.fallback_threshold,
            tagger_data_keys=[],
            trainable_classifiers=intent_config.trainable_classifiers or [],
            anaphora_allowed=None,
            positive_sampling=intent_config.positive_sampling,
            negative_sampling_from=intent_config.negative_sampling_from,
            negative_sampling_for=intent_config.negative_sampling_for,
            scenarios=intent_config.scenarios,
            utterance_tagger=intent_config.utterance_tagger
        )

    intents = [intent_conf.intent for intent_conf in intent_configs]
    return intent_infos, intents


def load_train_config(config_path, section=None):
    with codecs.open(config_path, 'r', encoding='utf8') as f:
        config = json.load(f)

        if section is not None:
            return config[section]
        else:
            return config
