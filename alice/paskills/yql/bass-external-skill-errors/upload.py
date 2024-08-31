# coding: utf-8
from __future__ import print_function

import argparse
import json
import logging
import time

import requests
from multiprocessing.dummy import Pool


"""
Скрипт загрузки отчётов в stat из YT.
"""


logger = logging.getLogger(__name__)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--token', required=True)
    return parser.parse_args()


def wait_for_upload(token, uuid):
    status = 'inprogress'
    while status not in ('success', 'error'):
        url = 'https://upload.stat.yandex-team.ru/_v3/meta_storage/add_data_status/' + uuid
        headers = {
            'Authorization': 'OAuth {}'.format(token),
            'Content-Type': 'application/json',
            'Accept': 'application/json',
        }
        try:
            response = requests.get(
                url,
                headers=headers,
                verify=False,
            )
            status = response.json().get('status') or 'inprogress'
        except (ValueError, requests.RequestException):
            logger.error('upload.stat.yandex.ru invalid response', exc_info=True)
        if status == 'inprogress':
            time.sleep(0.25)


def upload_table(table):
    headers = {
        'Authorization': 'OAuth {}'.format(table['token']),
        'Content-Type': 'application/json',
        'Accept': 'application/json',
    }
    upload_uuid = None
    while upload_uuid is None:
        try:
            response = requests.post(
                "https://upload.stat.yandex-team.ru/_api/report/yt_data_upload",
                headers=headers,
                data=json.dumps({
                    'name': table['stat_path'],
                    'scale': table['scale'],
                    'cluster': 'hahn',
                    'table_path': table['table_path'],
                    'replace_mask': ['fielddate'],
                }),
                verify=False,
            )
            upload_uuid = response.json()['uuid']
        except (ValueError, requests.RequestException):
            logger.error('upload.stat.yandex.ru invalid response', exc_info=True)
            time.sleep(10.)
    wait_for_upload(table['token'], upload_uuid)
    return table['table_path']


def main():
    args = parse_args()
    tables = [
        {
            'stat_path': 'VoiceTech/external_skills/external_skill_errors/dashboard',
            'table_path': '//home/paskills/skill_errors/reports/24h/errors_per_problem',
            'scale': 'h',
            'token': args.token,
        },
        {
            'stat_path': 'VoiceTech/external_skills/external_skill_errors/errors_per_day',
            'table_path': '//home/paskills/skill_errors/reports/stat',
            'scale': 'd',
            'token': args.token,
        },
        {
            'stat_path': 'VoiceTech/external_skills/external_skill_errors/external_skill_errors_24h',
            'table_path': '//home/paskills/skill_errors/reports/24h/errors_per_skill_and_problem',
            'scale': 'h',
            'token': args.token,
        },
    ]
    pool = Pool(processes=len(tables))
    results = pool.map(upload_table, tables)
    for table in results:
        print(table, 'ok')


if __name__ == '__main__':
    logging.basicConfig(format='%(asctime)s %(name)-12s %(levelname)-8s %(message)s')
    main()
