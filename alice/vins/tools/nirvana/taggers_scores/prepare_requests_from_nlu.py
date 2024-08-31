# coding: utf-8

import logging
import requests
import click
import uuid
import json

import yt.logger as yt_logger
import yt.wrapper as yt

from collections import defaultdict


yt_logger.LOGGER.setLevel(logging.DEBUG)
logging.basicConfig(
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    level=logging.INFO
)
logger = logging.getLogger(__name__)


COLUMNS = [
    ('text', 'string'),
    ('request_id', 'string'),
    ('app', 'any'),
    ('device_state', 'any'),
    ('experiments', 'any'),
    ('slots', 'any'),
]
SCHEMA = [{'name': name, 'type': type_} for (name, type_) in COLUMNS]


def create_slot(start, end, is_continuation=False):
    return {
        'start': start,
        'end': end,
        'is_continuation': is_continuation,
    }


def _iterate_nlu(response):
    for line in response.iter_lines():
        line = line.strip()

        if line.startswith('#') or len(line) == 0:
            continue

        tokens = line.split()
        text = []

        slot_start = -1
        has_error = False
        slots = defaultdict(list)

        for index, token in enumerate(tokens):
            quote_pos = token.find('\'')
            if quote_pos == 0:
                if slot_start != -1:
                    has_error = True
                    break
                slot_start = index
                token = token[1:]

            quote_pos = token.find('\'')
            if quote_pos != -1:
                start_par_pos = token.find('(')
                end_par_pos = token.find(')')
                if slot_start == -1 or start_par_pos == -1 or end_par_pos == -1:
                    has_error = True
                    break
                slot_type = token[start_par_pos+1:end_par_pos]
                slots[slot_type].append(create_slot(start=slot_start, end=index+1))
                slot_start = -1

                token = token[:quote_pos]

            text.append(token)

        if has_error:
            logger.debug('Wrong markup for line "{}"'.format(line))
            continue

        yield ' '.join(text), slots


def _iterate_json(response):
    for line in response.iter_lines():
        line = line.strip()

        data = json.loads(line)

        yield data['text'], data['slots']


def _request_generator(input_url, intent, input_reader):
    response = requests.get(input_url)
    assert response.status_code == 200

    experiments = {
        'force_intent': intent,
        'debug_tagger_scores': True,
    }

    for text, slots in input_reader(response):
        yield {
            'text': text,
            'request_id': "ffffffff-ffff-ffff" + str(uuid.uuid4())[18:-1],
            'app': None,
            'device_state': None,
            'experiments': experiments,
            'slots': slots,
        }


@click.command()
@click.option('--input_url', required=True, help='Input nlu file', envvar='INPUT_URL')
@click.option('--intent', required=True, help='Intent name for nlu file', envvar='INTENT')
@click.option('--output', required=True, help='Output table path', envvar='OUTPUT_TABLE')
@click.option('--format', type=click.Choice(['nlu', 'json']), default='json')
@click.option('--yt-proxy', default='hahn', help='YT cluster proxy', envvar='YT_PROXY')
@click.option('--yt-token', required=True, envvar='YT_TOKEN', help='Yt token')
def main(input_url, intent, output, format, yt_proxy, yt_token):
    yt.config['proxy']['url'] = yt_proxy
    yt.config['token'] = yt_token

    if format == 'nlu':
        input_reader = _iterate_nlu
    elif format == 'json':
        input_reader = _iterate_json
    else:
        logger.error('Wrong format for input file "{}"'.format(format))

    with yt.Transaction():
        if yt.exists(output):
            yt.alter_table(output, schema=SCHEMA)
        else:
            yt.create(
                'table', output,
                attributes={
                    'optimize_for': 'scan',
                    'schema': SCHEMA,
                },
            )

        yt.write_table(output, _request_generator(input_url, intent, input_reader=input_reader))


if __name__ == '__main__':
    main()
