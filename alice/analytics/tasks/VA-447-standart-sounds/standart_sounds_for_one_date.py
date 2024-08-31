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
    with_hints
)

from qb2.api.v1 import extractors as ne

import re


@with_hints(output_schema={'sound': str, 'fielddate': str, 'skill_id': str})
def find_records_with_standart_sounds(records):
    for record in records:
        if ('response' in record and
                record['response'] is not None and
                'voice_text' in record['response'] and
                record['response']['voice_text'] is not None and
                "<speaker audio=\"alice-" in record['response']['voice_text']):

            skill_id = 'not_a_skill'

            if 'form' in record and record['form'] and 'slots' in record['form'] and record['form']['slots']:
                for slot in record['form']['slots']:
                    if 'slot' in slot and slot['slot'] == 'skill_id' and 'value' in slot and slot['value']:
                        skill_id = slot['value']

            for category in re.findall('<speaker audio=\"alice-.{5,35}opus\">', record['response']['voice_text']):
                yield Record(fielddate=record['fielddate'],
                             skill_id=skill_id,
                             sound=category,
                             request_id=record['request'].get('request_id', ''))


@with_hints(output_schema={'sound': str, 'skill_id': str, 'fielddate': str, 'channel': str,
                           'category': str, 'developer_type': str, 'developer': str})
def multiply_records(records):
    for record in records:
        sounds = [record.get('sound')[16:-7], '_total_']
        skill_ids = [record.get('skill_id', '_undefined_'), '_total_']
        channels = [record.get('channel', '_undefined_'), '_total_']
        categories = [record.get('categoryLabel', '_undefined_'), '_total_']
        developer_types = [record.get('developerType', '_undefined_'), '_total_']
        developers = [record.get('developerName', '_undefined_'), '_total_']

        for channel in channels:
            for category in categories:
                for developer_type in developer_types:
                    for developer in developers:
                        for sound in sounds:
                            for skill_id in skill_ids:
                                yield Record(sound=sound,
                                             skill_id=skill_id,
                                             fielddate=record.get("fielddate"),
                                             channel=channel,
                                             category=category,
                                             developer_type=developer_type,
                                             developer=developer)


def main(date, report_path, pool=None):
    """
       Считаем метрики по сессиям пользователей
       для платформы Диалоги для дат от start - 7days до end.

       :param date: string "YYYY-MM-DD"
       :param report_path: путь на статистике до !папки, в которую будем загружать отчет"
       :param pool: YT pool
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

    if pool is None:
        cluster = clusters.Hahn()
    else:
        cluster = clusters.Hahn(pool=pool)
    job = cluster.job()

    sounds = job.table('//home/voice/vins/logs/dialogs/' + date) \
        .project(ne.all(), ne.const('fielddate', date)) \
        .map(find_records_with_standart_sounds) \
        .project(ne.all(), ne.table_path('table_path')) \
        .project(ne.all(), ne.custom("fielddate", lambda table_path: table_path[-10:]).add_hints(type=str))

    skills = job.table('//home/paskills/skills/stable') \
                .filter(nf.equals('onAir', True))

    dimensions = skills.project('channel', 'categoryLabel', 'developerType', 'developerName', skill_id='id')
    names = skills.project('name', skill_id='id')

    sounds = sounds.join(dimensions, by='skill_id')

    sounds = sounds.map(multiply_records) \
                   .groupby('skill_id', 'sound', 'fielddate', 'channel', 'category', 'developer_type', 'developer') \
                   .aggregate(times_used=na.count()) \
                   .join(names, type='left', by='skill_id') \
                   .project(ne.all(exclude=['skill_id', 'name']),
                            ne.custom('skill', lambda name: name if name is not None else '_total_')
                              .allow_null_dependency()
                              .add_hints(type=str))

    sounds.publish(report_config)

    job.run()


if __name__ == '__main__':
    call_as_operation(main)
