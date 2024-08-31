# -*-coding: utf8 -*-
#!/usr/bin/env python

from nile.api.v1 import (
    clusters,
    Record,
    with_hints,
    extractors as ne,
    aggregators as na,
    filters as nf
)
from collections import OrderedDict
from qb2.api.v1 import typing
from utils.nirvana.op_caller import call_as_operation
import json


SCHEMA = OrderedDict([
    ("key", typing.Optional[typing.String]),
    ("dialog", typing.Optional[typing.Json])
])


def get_response(vins_response):
    try:
        response = json.loads(vins_response)['directive']['payload']['response'] #.decode("utf-8")
    except TypeError:
        response = ""
    return response


def create_assistant_message(response):
    return {
        "message": response,
        "type": "assistant"
    }


def create_user_message(text):
    return {
        "message": {"text": text},
        "type": "user"
    }


@with_hints(output_schema=SCHEMA)
def make_json_dialogs(groups):
    for key, records in groups:
        new_record = {'dialog': []}
        for rec in records:
            if not rec['text'] or not rec['response']:
                continue
            if not new_record.get('key'):
                new_record['key'] = rec['fake_request_id']
            user_message = create_user_message(rec['text'])
            assistant_message = create_assistant_message(rec['response'])
            new_record['dialog'] += [user_message, assistant_message]
        yield Record(**new_record)


def main(output_path, basket, use_logs=False, pool="voice",
        downloader_result=None, start_date=None, end_date=None, vins='//home/voice/vins/logs/dialogs'):
    """
    Формирует json для отрисовки дивных карточек по данным из логов/прокачек.
    Возвращает таблицу с полями key (фейковый request_id первой реплики из корзины) и dialog (list реплик).
    use_logs == True:
        Ответы Алисы рисуются по залогированным в vins/logs/dialogs потрохам карточек.
        В логах за даты (start_date, end_date) грепаются real_reqid корзины.
        Вместо корзины можно подать любую таблицу с колонками (request_id, real_reqid, real_session_id, session_sequence, text).
        Запросы пользователя рисуются как баблы.
    use_logs == False:
        Ответы Алисы рисуются по результатам прокачки корзины (downloader_result).
    """

    templates = {}
    if start_date and end_date:
        templates = dict(dates='{' + start_date + '..' + end_date + '}')

    cluster = clusters.Hahn(pool=pool).env(templates=templates)
    job = cluster.job()
    basket_data = (job.table(basket)
        .project(
            'session_sequence',
            'text',
            request_id='real_reqid',
            session_id='real_session_id',
            fake_request_id='request_id'
        )
    )

    if use_logs:
       joined = (job.table(vins + '/@dates')
            .project('request_id', 'response')
            .join(basket_data, type='inner', by='request_id',  assume_unique_right=True)
        )

    else:
        joined = (job.table(downloader_result)
            .project(
                fake_request_id='RequestId',
                response=ne.custom(get_response, 'VinsResponse'))
            .join(basket_data, type='inner', by='fake_request_id',  assume_unique_right=True)
        )

    result = joined.groupby('session_id').sort('session_sequence').reduce(make_json_dialogs).put(output_path, schema=SCHEMA)
    job.run()
    return {'tableName': output_path}


if __name__ == '__main__':
    call_as_operation(main)
