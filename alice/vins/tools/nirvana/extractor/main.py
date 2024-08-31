# coding: utf-8
from __future__ import absolute_import, unicode_literals

import json
import logging
import os

from datetime import datetime as base_datetime

import yt.wrapper as yt

logger = logging.getLogger(__name__)
logging.basicConfig(format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                    level=logging.INFO)


COLUMNS = [
    ('request', 'any'),
    ('response', 'any'),
    ('features', 'any')
]


def get_schema():
    return [{'name': name, 'type': type_} for name, type_ in COLUMNS]


def parse_have_suggests(response):
    suggest = response.get('suggest', {})
    return isinstance(suggest, dict) and bool(suggest.get('items', []))


def parse_card_types(response):
    return list({card['type'] for card in response.get('cards', [])})


def parse_intent(response):
    return next((meta['intent'] for meta in response.get('meta', []) if meta['type'] == 'analytics_info'), None)


def parse_client_actions(response):
    return list({directive['name'] for directive in response.get('directives', [])})


def extract_features(response):
    response = response['response']
    return {
        'intent': parse_intent(response),
        'have_suggests': parse_have_suggests(response),
        'card_types': parse_card_types(response),
        'client_actions': parse_client_actions(response)
    }


def check_key_value_existence(key, dict_):
    return bool(dict_.get(key))


def init():
    if (
        check_key_value_existence('INPUT_CLUSTER', os.environ) and
        check_key_value_existence('INPUT_TABLE', os.environ)
    ):
        cluster = os.environ['INPUT_CLUSTER']
        input_table = os.environ['INPUT_TABLE']
    else:
        input_file = os.environ['INPUT_FILE']
        with open(input_file, 'r') as f:
            mr_table = json.load(f)
        cluster = mr_table['cluster']
        input_table = mr_table['table']

    yt.config['proxy']['url'] = cluster
    yt.config['token'] = os.environ['YT_TOKEN']

    hour_ms = 1000 * 60 * 60
    day_ms = hour_ms * 24
    expiration_hours = int(os.environ.get('EXPIRATION_HOURS', 0))

    expiration_timeout = day_ms * 2 if expiration_hours < 1 else hour_ms * expiration_hours

    need_temp_table = not check_key_value_existence('OUTPUT_TABLE', os.environ)

    if need_temp_table:
        output_table = yt.create_temp_table(attributes={'optimize_for': 'scan', 'schema': get_schema()},
                                            expiration_timeout=expiration_timeout)
    else:
        output_table = os.environ['OUTPUT_TABLE']

    output_file = os.environ['OUTPUT_FILE']

    logger.info('Welcome to VINS Extractor')
    logger.info('Using YT input path: {cluster}.[{table}]'.format(cluster=cluster, table=input_table))
    logger.info('Using YT output path: {cluster}.[{table}]'.format(cluster=cluster, table=output_table))
    if need_temp_table:
        logger.warning('Using temporary table, operation will fail if quota exceeded.\n'
                       'Temporary table will be expired in '
                       '{0} days and {1} hours'.format(expiration_timeout / day_ms,
                                                       (expiration_timeout % day_ms) / hour_ms))
    return cluster, input_table, output_table, output_file


def process(input_table):
    for i, row in enumerate(yt.read_table(input_table)):
        request, response = row['request'], row['response']
        features = extract_features(response)
        logger.info('Processed: {0}'.format(i + 1))
        yield {'request': request, 'response': response, 'features': features}


def main():
    cluster, input_table, output_table, output_file = init()
    logger.info('Start at {0}'.format(base_datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S')))

    with yt.Transaction():
        yt.write_table(output_table, process(input_table))

    with open(output_file, 'w') as f:
        json.dump({'cluster': cluster, 'table': output_table}, f, ensure_ascii=False)

    logger.info('End time: {0}'.format(base_datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S')))


if __name__ == '__main__':
    main()
