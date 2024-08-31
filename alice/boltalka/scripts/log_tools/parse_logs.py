# coding=utf-8
import os
import sys
import codecs
import argparse
import yt.wrapper as yt
yt.config.set_proxy("hahn.yt.yandex.net")
yt.config["tabular_data_format"] = yt.YsonFormat(control_attributes_mode="row_fields", table_index_column="@table_index")


GC_SKILL_ID = 'bd7c3799-5947-41d0-b3d3-4a35de977111'
APP_IDS = ('ru.yandex.searchplugin.beta', 'ru.yandex.searchplugin', 'ru.yandex.mobile')


def yield_utterances(row):
    if row['request']['app_info']['app_id'] not in APP_IDS:
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

    #if len(context) > 1000:
    #    return

    if len(row['response'].get('cards', [])) != 0 and \
                            row['response']['cards'][0]['text'] not in ('...', None):
        reply = row['response']['cards'][0]['text']
    else:
        reply = row['response']['voice_text']

    if reply is None:
        reply = ''

    session_id = None
    session_seq = None
    skill_id = None
    if row['form_name'].startswith('personal_assistant.scenarios.external_skill') and \
                                            not row['form_name'].endswith('__deactivate'):
        session_id = row['form']['slots'][2]['value']['id']
        session_seq = row['form']['slots'][2]['value']['seq']
        skill_id = row['form']['slots'][0]['value']
    elif row['app_id'] == 'gc_skill':
        session_id = row['request']['additional_options']['session_id']
        session_seq = int(row['request']['request_id'])
        skill_id = row['request']['additional_options']['skill_id']

    response_meta = None
    overriden_form = None
    if len(row['response']['meta']) != 0:
        response_meta = row['response']['meta'][0]['type']
        if response_meta == 'form_restored':
            overriden_form = row['response']['meta'][0]['overriden_form']

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
        'skill_id': skill_id,
        'response_meta': response_meta,
        'overriden_form': overriden_form,
    }


def remove_duplicates(key, rows):
    rows = [dict(x) for x in set(tuple(row.items()) for row in rows)]
    for row in rows:
        yield row


def merge_external_skill_dialogues(key, rows):
    if key['skill_id'] != GC_SKILL_ID:
        for row in rows:
            yield row
    else:
        rows = list(rows)
        form_name_dct = {}
        for row in rows:
            if row['app_id'] == 'gc_skill':
                key = '\t'.join([str(row['session_seq']), row['reply']])
                form_name_dct[key] = row['form_name']
        for row in rows:
            if row['feedback'] is not None:
                yield row
            elif row['app_id'] == 'pa':
                key = '\t'.join([str(row['session_seq']), row['reply']])
                if key in form_name_dct:
                    row['form_name'] = form_name_dct[key]
                yield row


def main(args):
    srcs = []
    lower_bound = args.src + '/' + args.from_date
    upper_bound = args.src + '/' + args.to_date
    for table in yt.list(args.src, absolute=True):
        if lower_bound <= table <= upper_bound:
            srcs.append(table)

    yt.run_map(yield_utterances, srcs, args.dst)
    yt.run_sort(args.dst, args.dst, sort_by=['uuid'])
    yt.run_reduce(remove_duplicates, args.dst, args.dst, reduce_by=['uuid'])
    yt.run_sort(args.dst, args.dst, sort_by=['session_id', 'skill_id'])
    yt.run_reduce(merge_external_skill_dialogues, args.dst, args.dst, reduce_by=['session_id', 'skill_id'],
                                                                      sort_by=['session_id', 'skill_id'])
    yt.run_sort(args.dst, args.dst, sort_by=['uuid', 'client_time'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--from-date', required=True)
    parser.add_argument('--to-date', required=True)
    parser.add_argument('--src', default='//home/voice/vins/logs/dialogs')
    parser.add_argument('--dst', required=True)
    args = parser.parse_args()
    main(args)
