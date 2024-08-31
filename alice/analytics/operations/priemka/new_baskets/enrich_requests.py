# -*-coding: utf8 -*-
from nile.api.v1 import filters as nf, extractors as ne
from qb2.api.v1 import typing as qt, filters as qf
from os import path
from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
import utils.yt.basket_common as common

import datetime
import re
import time


def filter_vicinity_requests(ss, main_ss, ts, main_ts, timedelta, vicinity_requests_before, vicinity_requests_after):
    if abs(ts - main_ts) <= timedelta:
        if ss < main_ss and main_ss - ss <= vicinity_requests_before:
            return True
        if ss > main_ss and ss - main_ss <= vicinity_requests_after:
            return True
    return False


def normalize_intent(intent):
    return intent.replace('.', '\t').replace('__', '\t')


def main(start_date=None, end_date=None, input_requests_table=None, output_table=None, add_requests_from_logs=False,
         apps=None, intents=None, regexp=None, regexp_sample_size=0, intent_sample_size=0,
         add_vicinity_requests=False, vicinity_requests_timedelta=20, vicinity_requests_before=0,
         vicinity_requests_after=0, tmp_path=None, memory_limit=2048, pool=None, store_checkpoints=False):
    if start_date is None:
        start_date_dt = datetime.datetime.now() - datetime.timedelta(days=300)
        start_date = start_date_dt.strftime("%Y-%m-%d")
    if end_date is None:
        end_date_dt = datetime.datetime.now() - datetime.timedelta(days=5)
        end_date = end_date_dt.strftime("%Y-%m-%d")

    templates = {"job_root": "//tmp/robot-voice-qa",
                 'checkpoints_root': path.dirname(output_table),
                 }
    if tmp_path is not None:
        templates['tmp_files'] = tmp_path
    cluster = hahn_with_deps(templates=templates, include_utils=True, pool=pool, use_yql=True)
    job = cluster.job().env(default_memory_limit=memory_limit)

    output_table = output_table or "//tmp/robot-voice-qa/new_scenarios_basket_" + common.random_part(16)
    date_str = '{' + start_date + '..' + end_date + '}'

    if not input_requests_table and not add_requests_from_logs:
        raise ValueError('There should be either input_requests_table or add_requests_from_logs==True')

    flattened_sessions = job.table(path.join("//home/alice/dialog/prepared_logs_expboxes", date_str)) \
        .project('req_id', 'uuid', 'session_id', 'session_sequence', 'fielddate', 'server_time_ms',
                 'app', 'client_time', 'input_type', 'intent', 'query')

    input_requests = flattened_sessions \
        .join(job.table(input_requests_table), by_left=('uuid', 'req_id'), by_right=('_uuid', 'req_id'),
              assume_small_right=True, force_unique=True) \
        .project('text', 'uuid', 'req_id', 'request_source',
                 'session_id', 'session_sequence', 'input_type', 'client_time', 'fielddate', 'server_time_ms') \
        .unique('uuid') \
        .checkpoint('enrich_input_requests_0_' + datetime.datetime.now().strftime("%m%d_%H%M"))

    if add_requests_from_logs:
        # добавляем запросы из логов
        logs = flattened_sessions \
            .filter(
                    qf.one_of('input_type', {'text', 'voice'}),
                    qf.defined('query'),
                    qf.one_of('app', apps + [None, ]),
                    )

        regexp_sample = logs.filter(nf.custom(lambda query: regexp is not None and re.search(regexp.encode('utf8'), query) is not None, 'query')) \
            .unique('uuid') \
            .random(count=regexp_sample_size) \
            .project('req_id', 'uuid', 'session_id', 'session_sequence', 'input_type', 'client_time', 'fielddate', 'server_time_ms',
                     text='query',
                     request_source=ne.const('regexp').with_type(qt.Optional[qt.String]),
                     )

        intent_sample = logs.filter(nf.custom(lambda intent: intents is not None and intent in map(normalize_intent, intents), 'intent')) \
            .unique('uuid') \
            .random(count=intent_sample_size) \
            .project('req_id', 'uuid', 'session_id', 'session_sequence', 'input_type', 'client_time', 'fielddate', 'server_time_ms',
                     text='query',
                     request_source=ne.const('intent').with_type(qt.Optional[qt.String])
                     )

        input_requests = job.concat(regexp_sample, intent_sample, input_requests) \
            .unique('req_id')\
            .checkpoint('enrich_input_requests_1_' + datetime.datetime.now().strftime("%m%d_%H%M"))

    if add_vicinity_requests:
        # добавляем запросы из окрестности
        used_session_ids = input_requests \
            .project(
                'session_id',
                main_req_id='req_id',
                main_session_sequence='session_sequence',
                main_ts='client_time',
                main_request_source='request_source'
            )

        vicinity_requests = flattened_sessions \
            .filter(qf.one_of('input_type', {'text', 'voice'})) \
            .join(used_session_ids, by='session_id', assume_small_right=True, force_unique_right=True) \
            .project(
                ne.all(),
                vicinity_requests_timedelta=ne.const(vicinity_requests_timedelta),
                vicinity_requests_before=ne.const(vicinity_requests_before),
                vicinity_requests_after=ne.const(vicinity_requests_after)
            ) \
            .filter(nf.custom(
                filter_vicinity_requests,
                'session_sequence',
                'main_session_sequence',
                'client_time',
                'main_ts',
                'vicinity_requests_timedelta',
                'vicinity_requests_before',
                'vicinity_requests_after'
            ))\
            .unique('req_id') \
            .project('req_id', 'uuid',
                     'session_id', 'session_sequence', 'input_type', 'fielddate', 'server_time_ms',
                     text='query',
                     request_source=ne.custom(lambda s: s + '_vicinity', 'main_request_source') \
                        .with_type(qt.Optional[qt.String]),
                     ).checkpoint('enrich_vicinity_requests_' + datetime.datetime.now().strftime("%m%d_%H%M"))

        input_requests = job.concat(input_requests, vicinity_requests) \
            .project('text', 'uuid', 'req_id', 'request_source',
                     'session_id', 'session_sequence', 'input_type', 'fielddate', 'server_time_ms') \
            .unique('req_id')

    input_requests \
        .unique('uuid') \
        .put(output_table)

    with job.driver.transaction():
        job.run(store_checkpoints=store_checkpoints)
    return {"cluster": "hahn", "table": output_table}


if __name__ == '__main__':
    st = time.time()
    print 'start at', time.ctime(st)
    call_as_operation(main)
    print 'total elapsed {:2f} min'.format((time.time() - st) / 60)
