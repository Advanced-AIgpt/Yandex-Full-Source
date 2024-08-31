#!/usr/bin/env python
# encoding: utf-8
from os import path

from nile.api.v1 import (
    clusters,
    extractors as ne,
    aggregators as na,
    datetime as nd
)

from utils.nirvana.op_caller import call_as_operation
from utils.yt.modifier import buffer_table


SCHEMA = [
    {"name": "uuid", "type": "string", "required": True, "sort_order": "ascending"},
    {"name": "cohort", "type": "string", "required": True},
    {"name": "first_day", "type": "string", "required": True}
]


def get_day_users(job, dialogs_node, cohort_week, date):
    table_name = path.join(dialogs_node, date)
    uuids = job.table(table_name).project('uuid').unique('uuid')
    return uuids.project('uuid',
                         cohort=ne.const(cohort_week),
                         first_day=ne.const(date))


def update_users_table(job, day_users, src_table, trg_table):
    if src_table is None:
        union = day_users
    else:
        union = job.concat(job.table(src_table), day_users)
    union.groupby(
        'uuid'
    ).aggregate(
        cohort=na.min('cohort'),
        first_day=na.min('first_day')
    ).sort(
        'uuid'
    ).put(
        trg_table
    )
    job.run()


def main(date, dialogs_root='//home/voice/vins/logs/dialogs', src_users_table=None, trg_users_table=None, pool=None):
    """
    Заполнение сводных данных по юзерам
    :param str date: Дата "YYYY-MM-DD", за которую нужно брать обновления
    :param str dialogs_root: YT-папка с логами диалогов
    :param str|None src_users_table: Таблица, в которой уже есть аккумулированные обновления.
        Если не указана, то в `trg` будут помещены данные только за день date.
    :param str|None trg_users_table: Таблица, в которую аккумулируются обновления.
        Может быть равна исходной
    :param str|None pool: название YT пула
    :return: None
    """
    if trg_users_table is None:
        if src_users_table is None:
            raise UserWarning('Must be set src or trg users table')
        trg_users_table = src_users_table

    cohort_week = nd.round_period(date, 'weekly')

    job = clusters.Hahn(pool=pool).job()

    day_users = get_day_users(job, dialogs_root, cohort_week, date)

    with buffer_table(trg_users_table, schema=SCHEMA, prefix='//tmp/alice/update-users-info') as tmp_table:
        update_users_table(job, day_users, src_users_table, tmp_table)


if __name__ == '__main__':
    call_as_operation(main)
