# -*-coding=utf-8-*-
import random
import hashlib

from nile.api.v1 import (
    clusters,
    Record,
    filters as nf,
    extractors as ne,
)
from qb2.api.v1 import filters as qf
from utils.nirvana.op_caller import call_as_operation


def random_part(i):
    return ''.join(map(lambda x: random.choice("0123456789abcdef"), xrange(i)))


def random_hash():
    return ''.join([''.join(['f']*8), '-', ''.join(['f']*4), '-', ''.join(['f']*4), '-',
                    random_part(4), '-', random_part(12)])


def get_duplicate_key(info_obj):
    key_phrase = ''
    for k in sorted(info_obj.keys()):
        if k != 'uuid':
            key_phrase += str(k) + str(info_obj[k])
    m = hashlib.md5()
    m.update(key_phrase)
    return m.hexdigest()


def get_quality_task(records):
    max_context_turns = 3
    for rec in records:
        context = [(None, '')] * max_context_turns
        context_info = [(None, '')] * max_context_turns
        for row in rec['session']:
            if row['intent'].startswith('personal_assistant\tfeedback'):
                continue
            do_not_use_user_logs = row.get('do_not_use_user_logs', False)
            context.pop(0)
            context_info.pop(0)
            if do_not_use_user_logs:
                context.append((None, None))
                context_info.append((None, None))
            else:
                context.append((row['type'], row['_query']))
                context_info.append((row['type'], row['_query']))

            if 'external_skill' not in row['intent'] and 'market' not in row['intent'] and 'image' not in row['intent']:
                if not do_not_use_user_logs and \
                        row['_reply'] != 'EMPTY' and row['_query'] is not None and row['_reply'] is not None and \
                        not any([(x[1] is None or x[1] == 'EMPTY') for x in context]):
                    res = {'context_' + str(i): context[-i - 1][1] for i in xrange(len(context))}

                    info_obj = {'context_' + str(i): context_info[-i - 1][1] for i in xrange(len(context_info))}

                    info_obj['intent'] = row['intent']
                    info_obj['reply'] = row['_reply']
                    info_obj['uuid'] = rec['uuid']
                    info_obj['request_id'] = row['req_id']

                    res['intent'] = row['intent']
                    res['reply'] = row['req_id']
                    res['reply_hint'] = row['_reply']

                    res['info'] = info_obj
                    res['key'] = random_hash()
                    res['duplicate_key'] = get_duplicate_key(info_obj)

                    yield Record(**res)
            context.pop(0)
            context_info.pop(0)
            if do_not_use_user_logs:
                context.append((None, None))
                context_info.append((None, None))
            else:
                context.append((None, row['req_id']))
                context_info.append((None, row['_reply']))


def filter_bucket(groups):
    for key, records in groups:
        bucket = set()
        for rec in records:
            if rec['duplicate_key'] in bucket:
                continue
            else:
                bucket.add(rec['duplicate_key'])
                yield rec


def prepare_to_join(records):
    for rec in records:
        if rec['context_2'] != '':
            yield Record(**{"task_key": rec['key'],
                            "intent": rec['intent'],
                            "reply_hint": rec['reply_hint'],
                            "req_id": rec['context_1'],
                            "info": rec['info'],
                            "seq": 0})
        yield Record(**{"task_key": rec['key'],
                        "intent": rec['intent'],
                        "reply_hint": rec['reply_hint'],
                        "req_id": rec['reply'],
                        "info": rec['info'],
                        "seq": 1})


def make_tasks(groups):
    for key, records in groups:
        task = []
        for rec in records:
            task.append({"type": "user", "message": {"text": rec['utterance_text']}})
            task.append({"type": "assistant", "message": rec['response']})
        yield Record(**{"key": key.task_key,
                        "intent": key.intent,
                        "info": rec['info'],
                        "task": {"dialog": task, "key": key.task_key},
                        "directives_dict": {x['name']: [x['payload'], x.get('sub_name', '')]
                                            for x in rec["response"]["directives"]}
                        })


def main(start_date, end_date, input_table=None, app='search_app_prod', pool=None, tmp_path=None, size=4000):
    if input_table is None:
        date_str = '{' + start_date + '..' + end_date + '}'
    else:
        date_str = input_table.split('/')[-1]

    templates = dict(job_root='//tmp/robot-voice-qa',
                     date=date_str)
    if tmp_path is not None:
        templates['tmp_files'] = tmp_path

    if pool is None:
        cluster = clusters.Hahn().env(templates=templates)
    else:
        cluster = clusters.Hahn(pool=pool).env(templates=templates)

    output_table = "//tmp/robot-voice-qa/quality_tasks_" + random_hash()

    job = cluster.job()
    tasks = job.table('//home/voice/dialog/sessions/@date') \
        .filter(nf.equals('app', app)) \
        .map(get_quality_task) \
        .sort('key') \
        .top(int(size) * 2, by='key', memory_limit=4000) \
        .project(ne.all(), fake_key=ne.const(1)) \
        .groupby('fake_key') \
        .reduce(filter_bucket) \
        .top(int(size), by='key', memory_limit=4000) \
        .map(prepare_to_join)

    responses = job.table('home/voice/vins/logs/dialogs/@date') \
        .filter(nf.equals('type', 'UTTERANCE')) \
        .filter(qf.or_(qf.not_(qf.defined('do_not_use_user_logs')), qf.not_(qf.nonzero('do_not_use_user_logs')))) \
        .filter(nf.custom(lambda x: 'request_id' in x, 'request')) \
        .project('response',
                 'utterance_text',
                 req_id=ne.custom(lambda x: x['request_id'], 'request'))

    tasks.join(responses, by='req_id', assume_unique_left=True) \
        .groupby('task_key', 'intent', 'reply_hint') \
        .sort('seq') \
        .reduce(make_tasks) \
        .put(output_table)

    job.run()

    return {"cluster": "hahn", "table": output_table}


if __name__ == '__main__':
    call_as_operation(main)
