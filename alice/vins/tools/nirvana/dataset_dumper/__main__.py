# coding: utf-8

from uuid import uuid4
import logging
import os
import re

import click

import yt.wrapper as yt

from vins_core.common import sample
from vins_core.config.app_config import load_app_config, Project
from vins_core.config.intent_config_loaders import parse_intent_configs, iterate_nlu_sources_for_intent
from vins_core.app_utils import load_app
from vins_core.utils.data import load_data_from_file

import personal_assistant  # noqa: UnusedImport


logging.basicConfig(
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    level=logging.INFO
)
logging.getLogger('yt').setLevel(logging.INFO)
logger = logging.getLogger(__name__)

COLUMNS = [
    ('text', 'string'),
    ('intent', 'string'),
    ('request_id', 'string'),
    ('app', 'any'),
    ('device_state', 'any'),
    ('use_for_tagger', 'boolean'),
    ('use_for_knn', 'boolean'),
    ('use_for_metric', 'boolean'),
    ('additional_options', 'any'),
    ('source_path', 'string'),
]
SCHEMA = [{'name': name, 'type': type_} for (name, type_) in COLUMNS]
CUSTOM_INTENTS = [
    'personal_assistant/config/scenarios/scenarios.mlconfig.json',
    'personal_assistant/config/handcrafted/handcrafted.mlconfig.json',
    'personal_assistant/config/stroka/stroka.mlconfig.json',
    'personal_assistant/config/navi/navi.mlconfig.json',
]


def _get_sample(item):
    return sample.Sample.from_nlu_source_item(item)


def nlu_generator_no_custom_intents(intent_pattern, app='personal_assistant'):
    app_obj = load_app(app, force_rebuild=False)
    for (intent_name, nlu_source_items) in app_obj.nlu.nlu_sources_data.iteritems():
        if not re.match(intent_pattern, intent_name):
            continue

        logger.info('Process intent in "no_custom_intents": {0}'.format(intent_name))
        for nlu_source_item in nlu_source_items:
            sample = _get_sample(nlu_source_item)
            use_for_knn = ('scenarios' in nlu_source_item.trainable_classifiers)
            use_for_tagger = nlu_source_item.can_use_to_train_tagger
            tags = sample.tags if use_for_tagger else None
            if use_for_knn or use_for_tagger:
                yield {
                    'text': sample.text,
                    'intent': intent_name,
                    'request_id': str(uuid4()),
                    'app': None,
                    'device_state': None,
                    'use_for_knn': use_for_knn,
                    'use_for_tagger': use_for_tagger,
                    'additional_options': {'nlu_tags': tags},
                    'source_path': nlu_source_item.source_path
                }


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


def nlu_generator_with_custom_intents(intent_pattern, app_name='personal_assistant', config_paths=CUSTOM_INTENTS):
    app_conf = load_app_config(app_name)
    intent_configs = parse_intent_configs(iterate_intents_from_config(app_conf, config_paths))
    for intent_config in intent_configs:
        if not re.match(intent_pattern, intent_config.name):
            continue

        logger.info('Process intent in "with_custom_intents": {0}'.format(intent_config.name))
        for nlu_source in iterate_nlu_sources_for_intent(intent_config, app_conf.nlu_templates):
            items = nlu_source.load()
            for nlu_source_item in items:
                sample = _get_sample(nlu_source_item)
                yield {
                    'text': sample.text,
                    'intent': intent_config.name,
                    'request_id': str(uuid4()),
                    'app': None,
                    'device_state': None,
                    'use_for_metric': True,
                    'source_path': nlu_source_item.source_path
                }


def dataset_generator(intent_pattern, app_name='personal_assistant'):
    dataset = {}
    for ds_item in nlu_generator_with_custom_intents(intent_pattern, app_name):
        key = (ds_item['source_path'], ds_item['text'])
        dataset[key] = ds_item

    for ds_item in nlu_generator_no_custom_intents(intent_pattern, app_name):
        key = (ds_item['source_path'], ds_item['text'])
        if key in dataset:
            result_item = dataset.pop(key)
            result_item.update(ds_item)
            yield result_item
        else:
            yield ds_item

    for ds_item in dataset.itervalues():
        yield ds_item


def compose_intent_pattern(intents_to_train_classifiers, intents_to_train_tagger):
    if not intents_to_train_classifiers and not intents_to_train_tagger:
        raise ValueError('No intents in dataset')
    pattern = '(^%s$)|(^%s$)' % (intents_to_train_classifiers, intents_to_train_tagger)
    return re.compile(pattern)


def create_or_alter_table(client, table_path):
    if client.exists(table_path):
        client.alter_table(table_path, schema=SCHEMA)
    else:
        client.create(
            'table', table_path,
            attributes={
                'optimize_for': 'scan',
                'schema': SCHEMA,
            },
        )


@click.command()
@click.option('--output', required=True, help='Nirvana output table')
@click.option('--intents-to-train-classifiers', default='.*', help='Intents name regex to train classifiers')
@click.option('--intents-to-train-tagger', default='.*', help='Intents name regex to train taggers')
def main(output, intents_to_train_classifiers, intents_to_train_tagger):
    client = yt.YtClient(
        proxy=os.environ['YT_PROXY'],
        token=os.environ['YT_TOKEN'],
    )
    pattern = compose_intent_pattern(intents_to_train_classifiers, intents_to_train_tagger)
    with client.Transaction(timeout=15 * 3600 * 10):
        create_or_alter_table(client, output)
        client.write_table(output, dataset_generator(pattern))


if __name__ == '__main__':
    main()
