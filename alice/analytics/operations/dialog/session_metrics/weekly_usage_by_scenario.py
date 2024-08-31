#!/usr/bin/env python
# encoding: utf-8

from utils.nirvana.op_caller import call_as_operation
from utils.statface.nile_client import make_stat_client
from utils.yt.dep_manager import hahn_with_deps

from nile.api.v1 import (
    Record,
    aggregators as na,
    extractors as ne,
    statface as ns
)

from qb2.api.v1 import filters as qf

from datetime import timedelta, datetime
from itertools import combinations


FIELDS_WITH_TOTALS = ['app', 'scenario']


class TaskHelpers(object):

    def __init__(self, start_date, end_date):
        self.start_date = start_date
        self.end_date = end_date
        self.week = timedelta(6)

    def counter(self, records):
        for record in records:
            record = record.to_dict()
            dates = record["dates"]
            dates = [datetime.strptime(date, '%Y-%m-%d').date() for date in dates]
            del record['dates']

            # copypaste of https://a.yandex-team.ru/arc/trunk/arcadia/alice/analytics/operations/dialog/session_metrics/alice_weekly_usage.py?rev=5882687#L94
            cur_date = self.start_date
            ago_date = cur_date - self.week
            left = 0  # index of first date in dates that is >= than ago_date
            right = 0  # index of first date in dates that is > than cur_date
            # all the dates in dates[left:right] are >= ago_date and < cur_date
            while cur_date <= self.end_date:
                while left < len(dates) and dates[left] < ago_date:
                    left += 1
                while right < len(dates) and dates[right] <= cur_date:
                    right += 1
                number = right - left  # number of usages in week [ago_date, cur_date]
                if number != 0 and dates[left] == ago_date:
                    record["fielddate"] = ago_date.strftime("%Y-%m-%d")
                    record["number"] = number
                    yield Record(**record)
                cur_date += timedelta(1)
                ago_date += timedelta(1)


    def multiplier(self, records):
        for record in records:
            record = record.to_dict()
            yield Record(**record)
            for i in range(len(FIELDS_WITH_TOTALS)):
                for combination in combinations(FIELDS_WITH_TOTALS, i + 1):
                    total_record = record.copy()
                    for field in combination:
                        total_record[field] = '_total_'
                        yield Record(**total_record)


def compute_metrics(start, end, report_config, prepared_root):

    start_date = datetime.strptime(start, '%Y-%m-%d').date()
    end_date = datetime.strptime(end, '%Y-%m-%d').date()
    log_start = (start_date - timedelta(6)).strftime("%Y-%m-%d")

    helper = TaskHelpers(start_date, end_date)
    templates = dict(dates='{' + log_start + '..' + end + '}')

    cluster = hahn_with_deps(pool='voice',
                             templates=templates,
                             neighbours_for=__file__,
                             neighbour_names=None,
                             include_utils=True)

    job = cluster.job()

    (job.table(prepared_root + '/@dates')
        .project(
            'uuid',
            'app',
            'fielddate',
            scenario=ne.custom(lambda x: "empty" if x == "" else x, 'generic_scenario'))
        .map(helper.multiplier, intensity="cpu")
        .groupby('uuid', 'app', 'scenario')
        .aggregate(dates=na.distinct('fielddate', sorted=True))
        .map(helper.counter, intensity="cpu")
        .groupby('fielddate', 'scenario', 'app')
        .aggregate(users_1_a_week=na.count(predicate=qf.equals('number', 1)),
                   users_2_a_week=na.count(predicate=qf.equals('number', 2)),
                   users_3_a_week=na.count(predicate=qf.equals('number', 3)),
                   users_4_a_week=na.count(predicate=qf.equals('number', 4)),
                   users_5_a_week=na.count(predicate=qf.equals('number', 5)),
                   users_6_a_week=na.count(predicate=qf.equals('number', 6)),
                   users_7_a_week=na.count(predicate=qf.equals('number', 7)))
        .publish(report_config))

    job.run()


def main(start, end,
        report_path='VoiceTech/Dialog/session_metrics/usage_by_scenario',
        prepared_root='//home/alice/dialog/prepared_logs_expboxes'):
    client = make_stat_client()

    report_config = (ns.StatfaceReport()
        .path(report_path)
        .title('Ежедневность по сценариям')
        .scale('daily')
        .dimensions(
            ns.Date('fielddate'),
            ns.StringSelector('scenario'),
            ns.StringSelector('app'))
        .measures(
            ns.Number('users_1_a_week'),
            ns.Number('users_2_a_week'),
            ns.Number('users_3_a_week'),
            ns.Number('users_4_a_week'),
            ns.Number('users_5_a_week'),
            ns.Number('users_6_a_week'),
            ns.Number('users_7_a_week'))
        .client(client))

    compute_metrics(start, end, report_config, prepared_root)


if __name__ == '__main__':
    call_as_operation(main)
