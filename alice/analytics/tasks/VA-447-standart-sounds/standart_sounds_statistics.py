#!/usr/bin/env python
# encoding: utf-8

from utils.nirvana.op_caller import call_as_operation
from utils.statface.nile_client import make_stat_client

from nile.api.v1 import (
    clusters,
    Record,
    aggregators as na,
    filters as nf,
    statface as ns,
    with_hints, extended_schema
)

from qb2.api.v1 import extractors as ne


@with_hints(output_schema={'sound': str, 'skill_id': str, 'fielddate': str, 'channel': str,
                           'category': str, 'developer_type': str, 'developer': str})
def multiply_records(records):
    for record in records:
        sounds = [record.get('sound')[16:-7], '_total_']
        skill_ids = [record.get('skill_id', '_undefined_'), '_total_']
        channels = [record.get('channel', '_undefined_'), '_total_']
        categories = [record.get('categoryLabel', '_undefined_'), '_total_']
        developerTypes = [record.get('developerType', '_undefined_'), '_total_']
        developers = [record.get('developerName', '_undefined_'), '_total_']

        for ch in channels:
            for ca in categories:
                for dt in developerTypes:
                    for de in developers:
                        for s in sounds:
                            for id in skill_ids:
                                yield Record(sound=s,
                                             skill_id=id,
                                             fielddate=record.get("fielddate"),
                                             channel=ch,
                                             category=ca,
                                             developer_type=dt,
                                             developer=de)


def main(start, end, report_path):
    """
       Считаем метрики по сессиям пользователей
       для платформы Диалоги для дат от start - 7days до end.

       :param start: string "YYYY-MM-DD"
       :param end: string "YYYY-MM-DD"
       :param report_path: путь на статистике до !папки, в которую будем загружать отчет
       :param tmp_path: путь для временных файлов (они небольшие, это не сами таблицы)

       """
    client = make_stat_client()

    report_config = ns.StatfaceReport() \
        .path(report_path) \
        .title('Использование стандартных звуков в навыках') \
        .scale('daily') \
        .dimensions(
        ns.Date('fielddate'),
        ns.StringSelector('skill'),
        ns.StringSelector('sound'),
        ns.StringSelector('channel'),
        ns.StringSelector('category'),
        ns.StringSelector('developer_type'),
        ns.StringSelector('developer')
    ).measures(
        ns.Number('times_used')
    ).client(client)

    if start != end:
        dates_str = '{' + start + '..' + end + '}'
    else:
        dates_str = start

    cluster = clusters.Hahn()
    job = cluster.job()

    sounds = job.table('//home/voice/nikitachizhov/VA-447/' + dates_str)\
        .project(ne.all(), ne.table_path('table_path')) \
        .project(ne.all(), ne.custom("fielddate", lambda table_path: table_path[-10:]).add_hints(type=str))

    skills = job.table('home/paskills/skills/stable')\
                .filter(nf.equals('onAir', True))

    dimensions = skills.project('channel', 'categoryLabel', 'developerType', 'developerName', skill_id = 'id')
    names = skills.project('name', skill_id='id')

    sounds = sounds.join(dimensions, by='skill_id')

    sounds = sounds.map(multiply_records)\
                   .groupby('skill_id', 'sound', 'fielddate', 'channel', 'category', 'developer_type', 'developer')\
                   .aggregate(times_used=na.count())\
                   .join(names, type='left', by='skill_id')\
                   .project(ne.all(exclude=['skill_id', 'name']),
                            ne.custom('skill', lambda name: name if name is not None else '_total_')
                              .allow_null_dependency()
                              .add_hints(type=str))

    sounds.publish(report_config)

    job.run()


if __name__ == '__main__':
    call_as_operation(main)
