#!/usr/bin/env python
# encoding: utf-8
from os.path import join
import time
from multiprocessing import Pool
from math import ceil

from nile.api.v1.datetime import date_range

from operations.utils_yt_to_clickhouse import copy_table, clean_partitions, FINALIZE_QUERY
from utils.nirvana.op_caller import call_as_operation
from utils.yt.reader import iter_table, count_rows, list_dir
from utils.clickhouse.requesting import post_req
from utils.clickhouse.conv import escape


def get_app_and_date(table_path):
    app, date = table_path.split('/')[-2:]
    date = date[:10]
    return app, date


def iter_lines(table_path):
    for record in iter_table(table_path):
        if record['result'] == 'bad':
            app, date = get_app_and_date(table_path)
            yield escape(date)
            yield '\t'
            yield escape(app)
            yield '\t'
            yield escape(record.get('action', '\\N'))
            yield '\t'
            yield record.get('intent', '\\N').replace('\t', '\\t')
            yield '\t'
            yield str(record.get('probability', '\\N'))
            yield '\t'
            yield escape(record.get('url', '\\N'))
            yield '\t'
            info = record.get('info')
            yield info.get('uuid', '\\N')
            yield '\t'
            yield escape(info.get('context_0', '\\N'))
            yield '\t'
            yield escape(info.get('context_1', '\\N'))
            yield '\t'
            yield escape(info.get('context_2', '\\N'))
            yield '\t'
            yield escape(info.get('reply', '\\N'))
            yield '\t'
            yield escape(info.get('request_id', '\\N'))
            yield '\n'


# Распределение по процессам

def gen_tasks(src_root, start_date, end_date, slice_cnt=24):
    # Разбивка таблиц на части для воркеров
    # Ну и заодно для более частных отчётов в stdout
    tasks = []
    for date in date_range(start_date, end_date):
        tbl_size = count_rows(join(src_root, date))
        if tbl_size is None:
            continue

        slice_size = int(ceil(float(tbl_size) / slice_cnt))
        for num in xrange(1, slice_cnt+1):
            tasks.append((src_root,
                          date,
                          '[#{}:#{}]'.format((num-1) * slice_size,
                                           num * slice_size),
                          '{:02}/{}'.format(num, slice_cnt)))

    return tasks


class Worker(object):
    # Чтобы использовать в multiprocessing, нужен pickleable объект, т.е. объявленный на верхнем уровне
    def __init__(self, kwargs):
        self.kwargs = kwargs

    def __call__(self, (src_root, date, slice, part_name)):
        copy_table(iter_lines, src_root=src_root, date=date, slice=slice, replace=False, **self.kwargs)
        return src_root, date, part_name


def parallel_copy_table(src_root_dir, trg_table, start_date, end_date, processes=2, chunk_size=50000, expire_days=30):
    """
    Копирует данные из YT-таблиц с толокерской раметкой в таблицу clickhouse.
    Если за эти же числа уже были данные - они будут перезаписаны.
    :param str src_root_dir: YT-директория с директориями приложений с табличками разметок
    :param str trg_table: Название таблицы в CH
    :param str start_date: Минимальная дата за которую нужно копировать данные
    :param str end_date: Максимальная дата за которую нужно копировать данные
    :param int processes: Количество процессов. Даже в отсутствие дополнительных ядер,
        имеет смысл делать хотя бы 2, чтобы второй мог работать, пока первый ждёт ответа от YT или CH
    :param int chunk_size: Количество строк, которые пишем в CH за один запрос.
    :param int expire_days: Из CH удаляются данные старее чем `end_date` - `expire_days`
    :return: None
    """
    worker = Worker({'trg_table': trg_table,
                     'chunk_size': chunk_size})

    print '{}: clean partitions...'.format(time.strftime('%H:%M:%S'))
    parts = clean_partitions(trg_table, start_date, end_date, expire_days=expire_days)
    print 'cleaned partitions: {}'.format(parts)

    print '{}: gen tasks...'.format(time.strftime('%H:%M:%S'))
    tasks = []
    for dir_name in list_dir(src_root_dir):
        src_root = join(src_root_dir, dir_name)
        tasks.extend(gen_tasks(src_root, start_date, end_date))
    print '{} tasks: {}'.format(len(tasks), tasks)

    p = Pool(processes)

    print '{}: start loading'.format(time.strftime('%H:%M:%S'))
    st = time.time()
    for src_root, date, part_name in p.imap_unordered(worker, tasks):
        elapsed = (time.time() - st) / 60
        print '{now}: "{date}" part {part} of app {app} loaded, {elapsed:.2f} min elapsed'.format(
            now=time.strftime('%H:%M:%S'),
            elapsed=elapsed,
            date=date, part=part_name,
            app=src_root.strip('/').split('/')[-1],
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
    # TRG = 'toloka_quality'
    # expire_days=40
    # parallel_copy_table('//home/voice/dialog/toloka/alice_quality_v2', TRG, ST_DATE, DATE, processes=4, expire_days=expire_days)
    # Для однопоточной обработки можно пользовать copy_table