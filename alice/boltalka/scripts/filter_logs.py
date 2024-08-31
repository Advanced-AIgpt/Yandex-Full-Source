#!/usr/bin/python
# coding=utf-8
import os
import sys
import codecs
import argparse
import yt.wrapper as yt
from copy import copy
from collections import defaultdict
yt.config.set_proxy("hahn.yt.yandex.net")
yt.config["tabular_data_format"] = yt.YsonFormat(control_attributes_mode="row_fields", table_index_column="@table_index")

LOGS_PREFIX = '//home/voice/vins/logs/dialogs'


def yield_utterance(row):
    if row['request']['app_info']['app_id'] not in ('ru.yandex.searchplugin.beta', 'ru.yandex.searchplugin', 'ru.yandex.mobile'):
        return
    context = ''
    feedback = None
    if row['type'] == 'UTTERANCE':
        context = row['utterance_text'].strip()
    elif row['type'] == 'CALLBACK':
        feedback_raw = row['callback_args'].get('form_update', {}).get('name', '')
        if feedback_raw.endswith('positive'):
            feedback = 1
        elif feedback_raw.endswith('negative'):
            feedback = 0
        else:
            return
    else:
        return

    if context and len(context) > 1000:
        return

    if len(row['response'].get('cards', [])) != 0 and \
                            row['response']['cards'][0]['text'] not in ('...', None):
        reply = row['response']['cards'][0]['text']
    else:
        reply = row['response']['voice_text']

    if reply is None:
        reply = ''

    session_id = None
    session_seq = None
    if row['form_name'].startswith('personal_assistant.scenarios.external_skill') and \
                                            not row['form_name'].endswith('__deactivate'):
        session_id = row['form']['slots'][2]['value']['id']
        session_seq = row['form']['slots'][2]['value']['seq']
    elif row['app_id'] == 'gc_skill':
        session_id = row['request']['additional_options']['session_id']
        session_seq = int(row['request']['request_id'])

    yield {
        'uuid': row['uuid'],
        'server_time': row['server_time'],
        'form_name': row['form_name'],
        'source': row['utterance_source'],
        'reply': reply,
        'context': context,
        'feedback': feedback,
        'app_id': row['app_id'],
        'session_id': session_id,
        'session_seq': session_seq,
        'client_time': row['client_time'],
    }


def merge_external_skill_dialogues(key, rows):
    if key['session_id'] is None:
        for row in rows:
            yield row
    else:
        rows = [dict(x) for x in set(tuple(row.items()) for row in rows)]
        reply_form_dct = {row['reply']: row['form_name'] for row in rows if row['app_id'] == 'gc_skill'}

        for row in rows:
            if row['feedback'] is not None:
                yield row
            elif row['app_id'] == 'pa':
                if row['reply'] in reply_form_dct:
                    row['form_name'] = reply_form_dct[row['reply']]
                yield row



def main(args):
    srcs = []
    lower_bound = LOGS_PREFIX + '/' + args.from_date
    upper_bound = LOGS_PREFIX + '/' + args.to_date
    for table in yt.list(LOGS_PREFIX, absolute=True):
        if lower_bound <= table <= upper_bound:
            srcs.append(table)

    yt.run_map(yield_utterance, srcs, args.dst)
    yt.run_sort(args.dst, args.dst, sort_by=['session_id'])
    yt.run_reduce(merge_external_skill_dialogues, args.dst, args.dst, reduce_by=['session_id'], sort_by=['session_id'])
    yt.run_sort(args.dst, args.dst, sort_by=['uuid', 'client_time'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--from-date', default='2017-08-01')
    parser.add_argument('--to-date', default='2017-08-06')
    parser.add_argument('--dst', required=True)
    args = parser.parse_args()

    main(args)
