# coding: utf-8
from __future__ import absolute_import, unicode_literals

import json
import logging
import os

from Queue import Queue
from datetime import datetime as base_datetime
from threading import Thread

import attr
import requests
import yt.wrapper as yt

logger = logging.getLogger(__name__)
logging.basicConfig(format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                    level=logging.INFO)
CA_PATH = 'allCAs.pem'
COLUMNS = [
    ('request', 'any'),
    ('response', 'any')
]

FAIL_COLUMNS = [
    ('request', 'any')
]


def get_schema(columns):
    return [{'name': name, 'type': type_} for name, type_ in columns]


def check_key_value_existence(key, dict_):
    return bool(dict_.get(key))


def make_pool(n, target):
    pool = []
    for _ in range(n):
        t = Thread(target=target)
        t.daemon = True
        t.start()
        pool.append(t)
    return pool


def post(request, url):
    if os.path.exists(CA_PATH):
        verify = CA_PATH
    else:
        verify = False

    timeout = (10., 20.)
    if isinstance(request, str):
        headers = {'Content-Type': 'application/x-www-form-urlencoded'}
        response = requests.post(url, headers=headers, data=request, verify=verify, timeout=timeout)
    else:
        headers = {'Content-Type': 'application/json'}
        response = requests.post(url, headers=headers, json=request, verify=verify, timeout=timeout)
    response.raise_for_status()
    return response.content


@attr.s
class YtData(object):
    cluster = attr.ib()
    token = attr.ib()


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

    yt_data = YtData(cluster, os.environ['YT_TOKEN'])
    yt.config['proxy']['url'] = yt_data.cluster
    yt.config['token'] = yt_data.token

    vins_url = os.environ['VINS_URL']

    hour_ms = 1000 * 60 * 60
    day_ms = hour_ms * 24
    expiration_hours = int(os.environ.get('EXPIRATION_HOURS', 0))

    expiration_timeout = day_ms * 2 if expiration_hours < 1 else hour_ms * expiration_hours

    need_temp_table = not check_key_value_existence('OUTPUT_TABLE', os.environ)

    if need_temp_table:
        output_table = yt.create_temp_table(attributes={'optimize_for': 'scan', 'schema': get_schema(COLUMNS)},
                                            expiration_timeout=expiration_timeout)
    else:
        output_table = os.environ['OUTPUT_TABLE']

    fail_table = yt.create_temp_table(attributes={'optimize_for': 'scan', 'schema': get_schema(FAIL_COLUMNS)},
                                      expiration_timeout=day_ms)

    output_file = os.environ['OUTPUT_FILE']
    output_fail_file = os.environ['OUTPUT_FAIL_FILE']

    logger.info('Welcome to VINS Requester')
    logger.info('Using YT input path: {cluster}.[{table}]'.format(cluster=cluster, table=input_table))
    logger.info('Using YT output path: {cluster}.[{table}]'.format(cluster=cluster, table=output_table))
    if need_temp_table:
        logger.warning('Using temporary table, operation will fail if quota exceeded.\n'
                       'Temporary table will be expired in '
                       '{0} days and {1} hours'.format(expiration_timeout / day_ms,
                                                       (expiration_timeout % day_ms) / hour_ms))
    logger.info('Failed requests will be putted in following table and will be expired in 24 hours: '
                '{cluster}.[{table}]'.format(cluster=cluster, table=fail_table))
    logger.info('Using VINS Url: {0}'.format(vins_url))

    return yt_data, input_table, output_table, fail_table, output_file, output_fail_file, vins_url


def main():
    yt_data, input_table, output_table, fail_table, output_file, output_fail_file, vins_url = init()

    logger.info('Start at {0}'.format(base_datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S')))

    n_threads = int(os.environ.get('N_THREADS', 10))
    max_retries = int(os.environ.get('MAX_RETRIES', 3))

    input_queue = Queue(n_threads * 2)
    output_queue = Queue()
    write_queue = Queue()

    total_items = 0
    total_done = 0
    total_failed = 0

    def process_worker():
        while True:
            attempt, request = input_queue.get()
            if request is None:
                break
            try:
                response = post(request, vins_url)
            except Exception as e:
                logger.exception(e)
                output_queue.put((attempt + 1, request))
            else:
                if response is not None:
                    write_queue.put((True, request, response))
                output_queue.put((-1, request))
            input_queue.task_done()

    def write_worker():
        failed = []

        def get_rows():
            while True:
                success, request, response = write_queue.get()
                if success is None:
                    break
                if success:
                    yield {'request': request, 'response': response}
                else:
                    failed.append({'request': request})
                write_queue.task_done()

        with yt.Transaction():
            client = yt.YtClient(yt_data.cluster, yt_data.token)
            client.write_table(output_table, get_rows())
            if failed:
                client.write_table(fail_table, failed)

    def process_output_queue(done_number, failed_number):
        while not output_queue.empty():
            attempt, request = output_queue.get()
            if attempt == -1:
                done_number += 1
                logger.info('Processed rows: {0}'.format(done_number))
            else:
                if attempt > max_retries:
                    failed_number += 1
                    logger.warning('Failed rows: {0}'.format(failed_number))
                    write_queue.put((False, request, None))
                else:
                    input_queue.put((attempt, request), block=True)
            output_queue.task_done()
        return done_number, failed_number

    threads = make_pool(n_threads, process_worker) + make_pool(1, write_worker)

    for row in yt.read_table(input_table):
        total_items += 1
        logger.info('Generated rows: {0}'.format(total_items))
        total_done, total_failed = process_output_queue(total_done, total_failed)
        input_queue.put((0, row['request']), block=True)

    while total_done + total_failed < total_items:
        total_done, total_failed = process_output_queue(total_done, total_failed)

    input_queue.join()
    output_queue.join()
    write_queue.join()

    for _ in range(n_threads):
        input_queue.put((0, None))
    write_queue.put((None, None, None))

    for thread in threads:
        thread.join()

    with open(output_file, 'w') as f:
        json.dump({'cluster': yt_data.cluster, 'table': output_table}, f, ensure_ascii=False)

    with open(output_fail_file, 'w') as f:
        json.dump({'cluster': yt_data.cluster, 'table': fail_table}, f, ensure_ascii=False)

    logger.info('End time: {0}'.format(base_datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S')))
    logger.info('Success: {0}\nFailed: {1}\nTotal: {2}'.format(total_done, total_failed, total_items))


if __name__ == '__main__':
    main()
