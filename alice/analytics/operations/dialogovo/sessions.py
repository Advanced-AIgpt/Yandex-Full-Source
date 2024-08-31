#!/usr/bin/env python
# encoding: utf-8
from utils.nirvana.op_caller import call_as_operation
from utils.statface.nile_client import make_stat_client
from utils.yt.dep_manager import hahn_with_deps

from nile.api.v1 import (
    clusters,
    Record,
    aggregators as na,
    filters as nf,
    statface as ns,
    extractors as ne
)
from qb2.api.v1 import filters as qf

from dialogovo_utils import *


def is_long(length, category):
    if category == 'games_trivia_accessories' and length > 5:
        return True
    elif category == 'kids' and length > 2:
        return True
    else:
        return length > 1


def reduce_length(groups):
    for key_record, records in groups:
        length = 0
        for record in records:
            length += 1
        yield Record(
            fielddate=record['fielddate'],
            uuid=record['uuid'],
            channel=record['channel'],
            app=record['app'],
            platform=record['platform'],
            developer_type=record['developer_type'],
            developer_name=record['developer_name'],
            category=record['category'],
            name=record['name'],
            skill_session_id=key_record['skill_session_id'],
            length=length
        )


def compute_metrics(start,
                    end,
                    input_path,
                    report_config,
                    tmp_path,
                    pool):

    dates = "{{{}..{}}}".format(start, end)
    if tmp_path is not None:
        templates = dict(dates=dates,
                         tmp_files=tmp_path)
    else:
        templates = dict(dates=dates)

    cluster = hahn_with_deps(pool=pool,
                             templates=templates,
                             neighbours_for=__file__,
                             neighbour_names=['dialogovo_utils.py'])

    job = cluster.job('Dialogovo: computing sessions metrics')

    job \
        .table(input_path + "/@dates") \
        .map(get_skill_props) \
        .filter(nf.not_(nf.equals('skill_session_id', None))) \
        .groupby('skill_session_id') \
        .reduce(reduce_length) \
        .map(clone_with('name')) \
        .map(clone_with('platform')) \
        .map(clone_with('app')) \
        .map(clone_with('developer_type')) \
        .map(clone_with('developer_name')) \
        .map(clone_with('category')) \
        .map(clone_with('channel')) \
        .groupby('fielddate', 'channel', 'category', 'developer_type', 'app', 'platform', 'developer_name', 'name') \
        .aggregate(
            dau=na.count_distinct('uuid'),
            sessions=na.count(),
            long_sessions=na.count(predicate=qf.custom(is_long, 'length', 'category')),
            sum_length=na.sum('length'),
            mean_length=na.mean('length'),
            median_length=na.median('length')
        ).publish(report_config)

    job.run()

def main(from_date, to_date, input_path, report_path, tmp_path=None, pool='voice'):
    """
    :param start: string "YYYY-MM-DD"
    :param end: string "YYYY-MM-DD"
    :param queries_path: путь до результата выполнения скрипта queries.py
    :param report_path: путь на статистике
    :param tmp_path:

    """
    client = make_stat_client()

    report_config = ns.StatfaceReport() \
        .path(report_path + '_sessions') \
        .title('Диалогово. Сессионные метрики') \
        .scale('daily') \
        .dimensions(
            ns.Date('fielddate'),
            ns.StringSelector('channel'),
            ns.StringSelector('app'),
            ns.StringSelector('platform'),
            ns.StringSelector('category'),
            ns.StringSelector('developer_type'),
            ns.StringSelector('developer_name'),
            ns.StringSelector('name')
        ).measures(
            ns.Number('dau'),
            ns.Number('sessions'),
            ns.Number('long_sessions'),
            ns.Number('sum_length'),
            ns.Number('mean_length'),
            ns.Number('median_length'),
        ).client(client)

    compute_metrics(from_date,
                    to_date,
                    input_path,
                    report_config,
                    tmp_path,
                    pool)

if __name__ == '__main__':
    call_as_operation(main)
