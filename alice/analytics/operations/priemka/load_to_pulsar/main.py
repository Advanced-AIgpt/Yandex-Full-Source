#!/usr/bin/env python
# encoding: utf-8

import sys
import json
import logging
import click

import yt.wrapper as yt

from pulsar import (
    PulsarClient,
    DatasetInfo,
    ModelInfo,
    InstanceInfo,
)

from alice.analytics.operations.dialog.pulsar.pulsar_columns import get_pulsar_columns


def _setup_logger():
    logging.basicConfig(
        format="[%(levelname)s] %(asctime)s - %(message)s",
        stream=sys.stdout,
        level=logging.INFO
    )


def get_table_columns(table, rows_count=10):
    """
    Считывает первые `row_count` из таблицы и возвращает набор колонок в таблице
    :param str table:
    :param int rows_count:
    :return List[str]:
    """
    columns = set()
    for idx, row in enumerate(yt.read_table(table)):
        if idx >= rows_count:
            break
        columns.update(row.keys())
    return list(columns)


@click.command()
@click.option('--inp', required=True)
@click.option('--out', required=True)
@click.option('--pulsar-token', required=True)
@click.option('--yt-token', required=True)
def main(inp, out, pulsar_token, yt_token):
    _setup_logger()
    logging.info('load_to_pulsar start')
    with open(inp) as fi:
        data = json.load(fi)

    logging.info('config loaded')

    yt.config.config['proxy']['url'] = 'hahn'
    yt.config.config['token'] = yt_token

    client = PulsarClient(token=pulsar_token)

    for item in data:
        logging.info('  pulsar instance start')
        columns_list = get_table_columns(item['table'])
        item['columns_list'] = columns_list
        pulsar_columns = get_pulsar_columns(columns_list)
        logging.info('  pulsar instance got columns: {}'.format(columns_list))

        instance_params = dict(
            model=ModelInfo(name=item['model_name']),
            dataset=DatasetInfo(name=item['dataset_name']),
            result={},
            per_object_data_metainfo=pulsar_columns,
            model_output_yt_table=item['table'],
        )

        for key in ['name', 'description', 'workflow_url']:
            if item.get(key):
                instance_params[key] = item.get(key)

        if item.get('instance_id'):
            instance_params['instance_id'] = item['instance_id']
            instance = InstanceInfo(**instance_params)
            client.update(instance)
            logging.info('  pulsar instance updated: {}'.format(item['instance_id']))
        else:
            instance = InstanceInfo(**instance_params)
            item['instance_id'] = client.add(instance)
            logging.info('  pulsar instance added: {}'.format(item['instance_id']))

        item['pulsar_link'] = 'https://pulsar.yandex-team.ru/get/' + item['instance_id'] + '?model_output=true'

    with open(out, 'w') as fo:
        json.dump(data, fo, indent=4)

    logging.info('load_to_pulsar finish')
