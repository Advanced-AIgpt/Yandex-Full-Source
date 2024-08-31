#!/usr/bin/env python
# encoding: utf-8
from os.path import join
from itertools import izip, count
import time
from multiprocessing import Pool
from math import ceil

from nile.api.v1.datetime import date_range

from operations.utils_yt_to_clickhouse import copy_table, FINALIZE_QUERY, clean_partitions
from utils.nirvana.op_caller import call_as_operation
from utils.yt.reader import iter_table, count_rows
from utils.clickhouse.requesting import post_req
from utils.clickhouse.conv import (
    to_str_array, to_num_array, to_json,
    escape, nullable_bool,
    to_str_item_array, to_item_subarrays,
)

import traceback

session_fmt = (
    # user level:
    '{rec[uuid]}\t{rec[user_id]}\t{rec[cohort]}\t{this_week}\t'
    '{rec[app]}\t{rec[platform]}\t{version}\t'
    '{rec[device_id]}\t{device}\t'
    # session level:
    '{rec[fielddate]}\t{session_id[1]}\t{session_id[3]}\t'
    '{experiments}\t{rec[is_exp_changed]:b}\t'
).format


def extract_session_level(record):
    device = record["device"].encode("utf8").replace('\n', '')

    rec = {key: escape(record[key])
           for key in ['uuid',
                       'user_id',
                       'cohort',
                       'app',
                       'platform',
                       'device_id',
                       'fielddate']}

    rec['is_exp_changed'] = record['is_exp_changed']

    return session_fmt(
        rec=rec,
        device=device,
        this_week=int(record['is_new'] == '1 week'),
        version=record['version'],
        session_id=record['session_id'].split('_'),
        experiments=to_str_array(record['experiments'])
    )

def extract_request_level(req, prefix, ord_idx, rev_idx):
    cards = req['cards']
    cb = req['callback']
    form = cb.get('form_update', {})
    # conversation
    yield prefix
    yield escape(req['_query'])
    yield '\t'
    yield escape(req['_reply'])
    yield '\t'

    yield req['req_id']
    yield '\t'
    # timing
    yield str(req['ms_server_time'])
    yield '\t'
    yield str(req['ts'])
    yield '\t'
    yield str(req['delta'])
    yield '\t'
    # order idx
    yield str(ord_idx)
    yield '\t'
    yield str(rev_idx)
    yield '\t'
    # intent
    yield req['intent'].replace('\t', '\\t')
    yield '\t'
    yield req.get('mm_scenario') or '\\N'
    yield '\t'
    yield req['generic_scenario']
    yield '\t'
    yield req['gc_intent'].replace('\t', '\\t') if req.get('gc_intent') else '\\N'
    yield '\t'
    yield req.get('gc_source') or '\\N'
    yield '\t'
    yield req.get('restored', '\\N').replace('\t', '\\t')
    yield '\t'
    yield req['skill_id'].strip().replace('\t', '\\t') if req.get('skill_id') else '\\N'
    yield '\t'

    yield req['type']
    yield '\t'

    # exp data
    yield to_num_array(req.get('test_ids'))
    yield '\t'
    yield req.get('expboxes') or '\\N'
    yield '\t'

    yield to_str_array(map(escape, req['suggests']))
    yield '\t'
    # cards
    yield to_str_item_array(cards, 'text')
    yield '\t'
    yield to_str_item_array(cards, 'type')
    yield '\t'
    yield to_str_item_array(cards, 'card_id')
    yield '\t'
    yield to_str_item_array(cards, 'intent_name')
    yield '\t'
    yield to_item_subarrays(cards, 'actions')
    yield '\t'
    # callback
    yield cb.get('name') or '\\N'
    yield '\t'
    yield escape(cb.get('form_name') or form.get('name'))
    yield '\t'
    yield nullable_bool(cb.get('resubmit') or form.get('resubmit'))
    yield '\t'
    yield to_json(cb.get('slots') or form.get('slots'))
    yield '\t'
    yield cb.get('mode') or '\\N'
    yield '\t'
    yield escape(cb.get('caption'))
    yield '\t'
    yield cb.get('suggest_type') or '\\N'
    yield '\t'
    yield escape(cb.get('utterance'))
    yield '\t'
    yield escape(cb.get('uri'))
    yield '\t'
    yield escape(cb.get('action_name'))
    yield '\t'
    yield cb.get('req_id') or '\\N'
    yield '\t'
    yield cb.get('card_id') or '\\N'
    yield '\t'
    yield cb.get('intent_name', '\\N').replace('\t', '\\t') if cb.get('intent_name', '\\N') is not None else '\\N'
    yield '\t'
    yield to_json(cb.get('btn_data'))
    yield '\t'
    yield escape(cb.get('source'))
    yield '\t'
    yield escape(cb.get('utm_source'))
    yield '\t'
    yield escape(cb.get('utm_medium'))
    yield '\t'
    yield escape(cb.get('skill_id'))
    yield '\n'



def iter_lines(table_path):
    for record in iter_table(table_path):
        prefix = extract_session_level(record)
        for req, ord_idx, rev_idx in izip(record['session'],
                                          count(start=1),
                                          count(start=len(record['session']),
                                                step=-1)):
            yield extract_request_level(req, prefix, ord_idx, rev_idx)


# Распределение по процессам

def gen_tasks(src_root, start_date, end_date, slice_cnt=96):
    # Разбивка таблиц на части для воркеров
    # Ну и заодно для более частных отчётов в stdout
    tasks = []
    for date in date_range(start_date, end_date):
        tbl_size = count_rows(join(src_root, date))
        if tbl_size is None:
            continue

        slice_size = int(ceil(float(tbl_size) / slice_cnt))
        for num in xrange(1, slice_cnt+1):
            tasks.append((date,
                          '[#{}:#{}]'.format((num-1) * slice_size,
                                           num * slice_size),
                          '{:02}/{}'.format(num, slice_cnt)))

    return tasks


class Worker(object):
    # Чтобы использовать в multiprocessing, нужен pickleable объект, т.е. объявленный на верхнем уровне
    def __init__(self, kwargs):
        self.kwargs = kwargs

    def __call__(self, (date, slice, part_name)):
        try:
            copy_table(iter_lines, date=date, slice=slice, replace=False, **self.kwargs)
        except Exception as e:
            print(traceback.format_exc())
            raise
        return date, part_name


def parallel_copy_table(src_root, trg_table, start_date, end_date, processes=2, chunk_size=50000, expire_days=30):
    """
    Копирует данные из YT-таблиц с сессиями в таблицу clickhouse.
    Если за эти же числа уже были данные - они будут перезаписаны.
    :param str src_root: YT-директория с табличками sessions
    :param str trg_table: Название таблицы в CH
    :param str start_date: Минимальная дата за которую нужно копировать данные
    :param str end_date: Максимальная дата за которую нужно копировать данные
    :param int processes: Количество процессов. Даже в отсутствие дополнительных ядер,
        имеет смысл делать хотя бы 2, чтобы второй мог работать, пока первый ждёт ответа от YT или CH
    :param int chunk_size: Количество строк, которые пишем в CH за один запрос.
    :param int expire_days: Из CH удаляются данные старее чем `end_date` - `expire_days`
    :return: None
    """
    worker = Worker({'src_root': src_root,
                     'trg_table': trg_table,
                     'chunk_size': chunk_size})

    print '{}: clean partitions...'.format(time.strftime('%H:%M:%S'))
    parts = clean_partitions(trg_table, start_date, end_date, expire_days=expire_days)
    print 'cleaned partitions: {}'.format(parts)

    print '{}: gen tasks...'.format(time.strftime('%H:%M:%S'))
    tasks = gen_tasks(src_root, start_date, end_date)
    print '{} tasks: {}'.format(len(tasks), tasks)

    p = Pool(processes)

    print '{}: start loading'.format(time.strftime('%H:%M:%S'))
    st = time.time()
    for date, part_name in p.imap_unordered(worker, tasks):
        elapsed = (time.time() - st) / 60
        print '{now}: "{date}" part {part} loaded, {elapsed:.2f} min elapsed'.format(
            now=time.strftime('%H:%M:%S'),
            elapsed=elapsed,
            date=date, part=part_name,
        )
    print '{}: loading tasks finished'.format(time.strftime('%H:%M:%S'))

    for date in date_range(start_date, end_date):
        # Непонятно, имеет ли смысл такие запросы параллелить
        print '{now}: finalize {date}'.format(now=time.strftime('%H:%M:%S'), date=date)
        post_req(FINALIZE_QUERY.format(table=trg_table, date=date))
    print '{}: finalize finished'.format(time.strftime('%H:%M:%S'))


if __name__ == '__main__':
    # Выключаем надоедливые "InsecurePlatformWarning"
    import requests.packages.urllib3
    requests.packages.urllib3.disable_warnings()

    call_as_operation(parallel_copy_table)
    # Для тестов:
    # DATE = '2018-04-10'
    # ST_DATE = '2018-04-10'
    # TRG = 'dialogs'
    # expire_days=40
    # parallel_copy_table('//home/voice/dialog/sessions', TRG, ST_DATE, DATE, processes=4, expire_days=expire_days)
    # Для однопоточной обработки можно пользовать copy_table
