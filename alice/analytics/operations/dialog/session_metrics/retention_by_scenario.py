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

from datetime import timedelta, datetime
from itertools import combinations


FIELDS_WITH_TOTALS = ['app', 'scenario']


class TaskHelpers(object):

    def __init__(self, start_date, end_date):
        self.start_date = start_date
        self.end_date = end_date

    def mapper(self, records):
        for record in records:
            record = record.to_dict()
            dates = record["dates"]
            dates = [datetime.strptime(date, '%Y-%m-%d').date() for date in dates]
            del record['dates']

            first_cohort = [self.start_date - timedelta(n_days) for n_days in range(7, 29, 7)]
            last_cohort = [self.end_date - timedelta(n_days) for n_days in range(7, 29, 7)]

            # copypaste of https://a.yandex-team.ru/arc/trunk/arcadia/alice/analytics/operations/dialog/session_metrics/retention_in_intents_for_period.py below
            # индексы в массиве отсортированных дат использования для данного пользователя
            # ограничивающие даты из 1ой, 2ой, 3ей, 4ой недели
            # от текущей даты(индекс cur_ind которой перебираем в цикле дальше)
            index = [[0, 0], [0, 0], [0, 0], [0, 0]]

            for cur_ind, cur_date in enumerate(dates):
                # был ли пользователь новым (не использовал Алису предыдущие 30 дней) в день с индексом cur_ind
                is_new = (cur_ind == 0 or cur_date - timedelta(30) > dates[cur_ind - 1])

                if not is_new:
                    continue

                # кол-во новых пользователей за неделю (размер когорты, за которой наблюдаем)
                if self.start_date <= cur_date <= self.end_date:
                    record["n_th_week"] = -1
                    record["fielddate"] = cur_date.strftime("%Y-%m-%d")
                    record["returned"] = 0
                    yield Record(**record)

                bounds = []
                for i in range(4):
                    bounds.append([cur_date + timedelta(i * 7 + 1),
                                   cur_date + timedelta(i * 7 + 7)])
                for i in range(4):
                    while index[i][0] < len(dates) and dates[index[i][0]] < bounds[i][0]:
                        index[i][0] += 1
                    while index[i][1] < len(dates) and dates[index[i][1]] <= bounds[i][1]:
                        index[i][1] += 1

                for week in range(4):
                    if cur_date > last_cohort[week] or cur_date + timedelta(6) < first_cohort[week]:
                        continue
                    # вернулся ли пользователь на неделе week
                    returned = int(index[week][1] - index[week][0] > 0)
                    cohort = cur_date
                    # перебор когорт, к которым он относится
                    for i in range(7):
                        if first_cohort[week] <= cohort <= last_cohort[week]:
                            record["n_th_week"] = week + 1
                            record["fielddate"] = cohort.strftime("%Y-%m-%d")
                            record["returned"] = returned
                            yield Record(**record)
                        cohort += timedelta(1)


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
    """
    start, end - период, для которого нужно посчитать новые значения
    смотрит в логи [start - (30 + 6 + 28), end]
    """
    start_date = datetime.strptime(start, '%Y-%m-%d').date()
    end_date = datetime.strptime(end, '%Y-%m-%d').date()
    log_start = (start_date - timedelta(30 + 6 + 28)).strftime("%Y-%m-%d")

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
        .map(helper.mapper, intensity="cpu")
        .groupby('fielddate', 'n_th_week', 'scenario', 'app')
        .aggregate(new=na.count(), returned=na.sum('returned'))
        .publish(report_config))

    job.run()


def main(start, end,
        report_path='VoiceTech/Dialog/session_metrics/retention_by_scenario',
        prepared_root='//home/alice/dialog/prepared_logs_expboxes'):
    client = make_stat_client()

    report_config = (ns.StatfaceReport()
        .path(report_path)
        .title('Retention по сценариям')
        .scale('daily')
        .dimensions(
            ns.Date('fielddate'),
            ns.StringSelector('scenario'),
            ns.NumberSelector('n_th_week'),
            ns.StringSelector('app'))
        .measures(
            ns.Number('new'),
            ns.Number('returned'))
        .client(client))

    compute_metrics(start, end, report_config, prepared_root)


if __name__ == '__main__':
    call_as_operation(main)
