# -*-coding: utf8 -*-
from nile.api.v1 import (
    clusters,
    Record,
    filters as nf,
    aggregators as na,
    extractors as ne,
    statface as ns,
    datetime as nd,
    multischema, with_hints, modified_schema, extended_schema
)
from itertools import product
from utils.statface.nile_client import make_stat_client
from utils.nirvana.op_caller import call_as_operation
from datetime import timedelta, datetime
from itertools import combinations
import yt.wrapper as yt
from utils.yt.cli import make_simple_client

DELIMITER = '\t'
TOTAL = '_total_'
FIELDS_WITH_TOTALS = ('scenario', 'intent', 'app', 'platform', 'is_new', 'cohort', 'country_id')


@with_hints(output_schema=extended_schema())
def multiplier(records):
        for record in records:
            record = record.to_dict()
            yield Record(**record)
            for i in range(len(FIELDS_WITH_TOTALS)):
                for combination in combinations(FIELDS_WITH_TOTALS, i + 1):
                    total_record = record.copy()
                    for field in combination:
                        total_record[field] = TOTAL
                        yield Record(**total_record)


def compute_metrics(start_date, end_date, report_table, scale, pool, prepared_root):

    templates = dict(dates='{' + start_date + '..' + end_date + '}')
    io_option = {"table_writer": {"max_row_weight": 134217728}}


    cluster = clusters.Hahn(pool=pool).env(
        templates=templates,
        yt_spec_defaults={"job_io": io_option, "partition_job_io": io_option, "merge_job_io": io_option,
                          "map_job_io": io_option, "reduce_job_io": io_option, "sort_job_io": io_option})

    job = cluster.job()
    sessions = (job.table(prepared_root + '/@dates')
        .filter(nf.not_(nf.equals('input_type', 'tech')))
        .project(
            ne.all(exclude='country_id'),
            country_id=ne.custom(lambda x: str(x), 'country_id').add_hints(type=str),
            scenario=ne.custom(lambda x: "empty" if x == "" else x, 'generic_scenario').add_hints(type=str))
        .map(multiplier, intensity="cpu")
        .groupby('uuid', 'fielddate', *FIELDS_WITH_TOTALS)
        .aggregate(hits=na.count(), intensity='ultra_cpu')
        .groupby('fielddate', *FIELDS_WITH_TOTALS)
        .aggregate(hits=na.sum('hits'),
                   users=na.count_distinct('uuid'),
                   intensity='cpu')
        .project(
            ne.all(),
            intent_tree=ne.custom(lambda x, y: '\t{}\t{}\t'.format(x, y.replace('\t', '.')), 'scenario', 'intent').add_hints(type=str))
            .put(report_table, ttl=timedelta(days=7)))

    job.run()


def main(start, end, scale='daily',
         report_intents="VoiceTech/Dialog/session_metrics/intents",
         prepared_root='//home/alice/dialog/prepared_logs_expboxes',
        report_root='//home/sda/reports/intents',
        pool='voice',):

    client = make_stat_client()

    report_config = (ns.StatfaceReport()
        .path(report_intents)
        .title("Использование по интентам")
        .scale(scale)
        .dimensions(
            ns.Date('fielddate'),
            ns.TreeSelector('intent_tree'),
            ns.StringSelector('is_new'),
            ns.StringSelector('cohort'),
            ns.StringSelector('app'),
            ns.StringSelector('platform'),
            ns.StringSelector('country_id'))
        .measures(
            ns.Number('users'),
            ns.Number('users_share'),
            ns.Number('hits'),
            ns.Number('hits_share'))
        .client(client))

    report_table = '{}/{}_{}'.format(report_root, start, end)
    cli = make_simple_client()
    if not yt.exists(report_table, client=cli):
        print '{} calculating...'.format(report_table)
        compute_metrics(start, end, report_table, scale, pool, prepared_root)
    else:
        print '{} already exists'.format(report_table)

    io_option = {"table_writer": {"max_row_weight": 134217728}}
    cluster = clusters.Hahn(pool=pool).env(
        yt_spec_defaults={"job_io": io_option, "partition_job_io": io_option, "merge_job_io": io_option,
                          "map_job_io": io_option, "reduce_job_io": io_option, "sort_job_io": io_option})


    job = cluster.job()
    (job.table(report_table)
        .filter(nf.not_(nf.equals('intent', '')))
        .publish(report_config, mode='remote'))

    print '{} loading to {}...'.format(report_table, report_intents)
    job.run()


if __name__ == '__main__':
    call_as_operation(main)
