# -*-coding: utf8 -*-
from nile.api.v1 import (
    clusters,
    Record,
    aggregators as na,
    extractors as ne,
    statface as ns,
    with_hints, extended_schema
)

from utils.statface.nile_client import make_stat_client
from utils.nirvana.op_caller import call_as_operation
from itertools import product
import urlparse, json
from urllib import quote


TOTAL_FIELDS = ('app', 'platform', 'session_type')
TOTAL = '_total_'


@with_hints(output_schema=extended_schema())
def map_multiply_records(records):
    for record in records:
        parsed_record = record.to_dict()
        total_fields = tuple(
            (parsed_record[_], TOTAL) for _ in TOTAL_FIELDS
        )
        for fields in product(
                *(total_fields)
        ):
            for idx in range(len(TOTAL_FIELDS)):
                parsed_record[TOTAL_FIELDS[idx]] = fields[idx]
            yield Record(**parsed_record)


@with_hints(output_schema=dict(source=str, fielddate=str, uuid=str, **{field: str for field in TOTAL_FIELDS}))
def get_source(records):
    for record in records:
        intent = record['intent']
        gc_source = record['other'].get('gc_source')
        if intent.startswith("personal_assistant\tgeneral_conversation\tgeneral_conversation") \
                or intent.startswith("personal_assistant\tscenarios\texternal_skill_gc") \
                or record.get('mm_scenario') == 'GeneralConversation':
            if gc_source:
                yield Record(**{
                    'source': gc_source,
                    'fielddate': record['fielddate'],
                    'uuid': record['uuid'],
                    'app': record['app'],
                    'platform': record['platform'],
                    'session_type': intent.split('\t')[2].replace('_dummy', '')
                })


def compute_metrics(date_param, report_config, expboxes_root, pool):
    if pool is None:
        cluster = clusters.Hahn().env()
    else:
        cluster = clusters.Hahn(pool=pool).env()

    if date_param[0] == date_param[1]:
        date_str = date_param[0]
    else:
        date_str = '{' + date_param[0] + '..' + date_param[1] + '}'

    job = cluster.job()
    job.table(expboxes_root + "/" + date_str) \
        .map(get_source) \
        .map(map_multiply_records, intensity='ultra_cpu') \
        .groupby('source', 'fielddate', 'app', 'platform', 'session_type') \
        .aggregate(count=na.count()) \
        .project(ne.all()) \
        .publish(report_config)
    job.run()


def main(start_date, end_date, scale='daily', report='VoiceTech/Dialog/gc_sources_count',
         expboxes_root='//home/alice/dialog/prepared_logs_expboxes', pool=None):
    client = make_stat_client()
    date_param = [start_date, end_date]

    report_pa_usage = ns.StatfaceReport() \
        .path(report) \
        .title("Источники болталки") \
        .scale(scale) \
        .dimensions(
        ns.Date('fielddate'),
        ns.StringSelector('source'),
        ns.StringSelector('app'),
        ns.StringSelector('platform'),
        ns.StringSelector('session_type')
    ).measures(
        ns.Number('count')
    ).client(client)

    compute_metrics(date_param, report_pa_usage, expboxes_root, pool)


if __name__ == '__main__':
    call_as_operation(main)
