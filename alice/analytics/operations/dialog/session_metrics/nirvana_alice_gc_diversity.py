# -*-coding: utf8 -*-
from nile.api.v1 import (
    clusters,
    Record,
    statface as ns,
    aggregators as na,
    multischema, with_hints, modified_schema
)
from utils.nirvana.op_caller import call_as_operation
from utils.statface.nile_client import make_stat_client

from itertools import product
TOTAL_FIELDS = ('session_type', 'app', 'platform',)
TOTAL = '_total_'


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


def get_gc_replies(records):
    for rec in records:
        if rec['reply'] != 'EMPTY' and not rec.get('do_not_use_user_logs', False):
            if (rec['intent'].endswith('external_skill_gc\tcontinue') or
                rec['intent'].endswith('external_skill_gc')) and \
                    rec['other'].get('gc_intent') is not None and rec['other'].get('gc_intent').endswith('general_conversation'):
                session_type = 'external_skill_gc'
            elif rec['intent'].endswith('general_conversation'):
                session_type = 'general_conversation'
            else:
                continue
            yield Record(reply=rec['reply'],
                         fielddate=rec['fielddate'],
                         app=rec['app'],
                         platform=rec['platform'],
                         session_type=session_type)


def compute_metrics(date_param, report_config, expboxes_root):
    if date_param[0] == date_param[1]:
        date_str = date_param[0]
    else:
        date_str = '{' + date_param[0] + '..' + date_param[1] + '}'

    cluster = clusters.yt.Hahn()
    job = cluster.job()

    sessions = job.table(expboxes_root + '/' + date_str) \
                  .map(get_gc_replies) \
                  .groupby('reply', 'fielddate', 'app', 'platform', 'session_type') \
                  .aggregate(count=na.count()) \
                  .groupby('fielddate', 'app', 'platform', 'session_type')

    top_q = sessions.top(10000, by='count').groupby('fielddate', 'app', 'platform', 'session_type')
    stat_10000 = top_q.aggregate(top_10000=na.sum('count'))
    stat_1000 = top_q.top(1000, by='count') \
        .groupby('fielddate', 'app', 'platform', 'session_type') \
        .aggregate(top_1000=na.sum('count'))
    stat_100 = top_q.top(100, by='count') \
        .groupby('fielddate', 'app', 'platform', 'session_type') \
        .aggregate(top_100=na.sum('count'))
    stat_10 = top_q.top(10, by='count') \
        .groupby('fielddate', 'app', 'platform', 'session_type') \
        .aggregate(top_10=na.sum('count'))
    total = sessions.aggregate(total=na.sum('count'))
    stat_10000.join(total, by=['fielddate', 'app', 'platform', 'session_type']) \
        .join(stat_1000, by=['fielddate', 'app', 'platform', 'session_type']) \
        .join(stat_100, by=['fielddate', 'app', 'platform', 'session_type']) \
        .join(stat_10, by=['fielddate', 'app', 'platform', 'session_type']) \
        .map(map_multiply_records) \
        .groupby('fielddate', 'app', 'platform', 'session_type') \
        .aggregate(top_10000=na.sum('top_10000'),
                   top_1000=na.sum('top_1000'),
                   top_100=na.sum('top_100'),
                   top_10=na.sum('top_10'),
                   total=na.sum('total')) \
        .publish(report_config)

    job.run()


def main(start_date, end_date, report_path, expboxes_root='//home/alice/dialog/prepared_logs_expboxes'):
    client = make_stat_client()
    date_param = [start_date, end_date]
    report_config = ns.StatfaceReport().path(report_path) \
        .scale('daily') \
        .dimensions(
            ns.Date('fielddate'),
            ns.StringSelector('app'),
            ns.StringSelector('platform'),
            ns.StringSelector('session_type')) \
        .measures(
            ns.Number('top_10'),
            ns.Number('top_100'),
            ns.Number('top_1000'),
            ns.Number('top_10000'),
            ns.Number('total')).client(client)

    compute_metrics(date_param, report_config, expboxes_root)


if __name__ == '__main__':
    call_as_operation(main)
