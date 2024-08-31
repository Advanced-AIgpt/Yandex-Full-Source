# -*- coding: utf-8 -*-

"""Converts relevance dataset to the format used during dataset building"""

import argparse
import uuid
import yt.yson as yson
import yt.wrapper as yt


def _get_reqid():
    return '-'.join(
        ['ffffffff-ffff-ffff'] + str(uuid.uuid4()).split('-')[3:]
    )

_SCHEMA = [
    {"name": "app", "type": "any"},
    {"name": "device_state", "type": "any"},
    {"name": "prev_intent", "type": "string"},
    {"name": "text", "type": "string"},
    {"name": "true_intents", "type": "string"},
    {"name": "request_id", "type": "string"},
]


_APP = yson.json_to_yson({
    'app_id': 'ru.yandex.quasar.app',
    'app_version': '1.0',
    'device_manufacturer': 'Yandex',
    'device_model': 'Station',
    'os_version': '6.0.1',
    'platform': 'android'
})


_DEVICE_STATE = yson.json_to_yson({
    'is_tv_plugged_in': True
})


_PREV_INTENT = 'personal_assistant.general_conversation.general_conversation'


def mapper(input_row):
    text = input_row['utt']
    true_intents = []
    for key, value in input_row.iteritems():
        if key != 'utt' and value == 1:
            true_intents.append(key)

    if true_intents:
        true_intents = ','.join(true_intents)

        yield {
            'app': _APP,
            'device_state': _DEVICE_STATE,
            'prev_intent': _PREV_INTENT,
            'text': text,
            'true_intents': true_intents,
            'request_id': _get_reqid()
        }


def main():
    argument_parser = argparse.ArgumentParser(add_help=True)
    argument_parser.add_argument('--input', action='store', required=True, help='input')
    argument_parser.add_argument('--output', action='store', required=True, help='output')
    args = argument_parser.parse_args()

    if yt.exists(args.output):
        yt.remove(args.output)

    yt.create_table(args.output, attributes={'schema': _SCHEMA})

    yt.run_map(
        mapper,
        source_table=args.input,
        destination_table=args.output,
    )


if __name__ == '__main__':
    main()
