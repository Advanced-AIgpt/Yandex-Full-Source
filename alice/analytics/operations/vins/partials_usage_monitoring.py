#!/usr/bin/env python
# encoding: utf-8

from utils.nirvana.op_caller import call_as_operation
from utils.statface.nile_client import make_stat_client
from utils.yt.dep_manager import list_deps

from nile.api.v1 import (
    clusters,
    Record,
    extractors as ne,
    aggregators as na,
    statface as ns
)

from qb2.api.v1 import filters as qf

import yt.yson as yson
import json


def mapper1(records, vins, stat, err):
    for record in records:
        if record['qloud_project'] == 'voice-ext' \
                and record['qloud_application'] == 'uniproxy' \
                and record['qloud_environment'] in ('station-stable', 'prestable'):

            message = record['message']

            if message.find('VinsPartial') != -1:
                try:
                    eventtime = record['iso_eventtime']
                    fielddate = eventtime[0:10]
                    if fielddate < "2018-08-31" and message[12] == 'b':
                        msg = yson.loads(message[14:-1])    # OLD FORMAT
                    else:
                        msg = json.loads(message[12:])  # NEW FORMAT STARTS FROM 31 Aug 2018
                    messageId = msg['Directive']['refMessageId']
                    sessionId = msg['Session']['SessionId']
                    status = msg['Directive']['status']

                    vins(Record(messageId=messageId,
                                sessionId=sessionId,
                                status=status,
                                fielddate=fielddate))
                except:
                    err(Record(fielddate=fielddate, message=message))

            if message.find('RequestStat') != -1:
                try:
                    eventtime = record['iso_eventtime']
                    fielddate = eventtime[0:10]
                    if fielddate < "2018-08-31" and message[12] == 'b':
                        msg = yson.loads(message[14:-1])    # OLD FORMAT
                    else:
                        msg = json.loads(message[12:])  # NEW FORMAT STARTS FROM 31 Aug 2018
                    Time = msg['Event']['event']['payload']['durations']['onRecognitionEndTime-onVinsResponseTime']
                    messageId = msg['Event']['event']['payload']['refMessageId']
                    sessionId = msg['Session']['SessionId']

                    stat(Record(messageId=messageId,
                                sessionId=sessionId,
                                Time=int(Time),
                                fielddate=fielddate))
                except:
                    err(Record(fielddate=fielddate, message=message))


def mapper2(records, vins, stat, err):
    for record in records:
        message = record['message']

        if message.find('VinsPartial') != -1:
            try:
                eventtime = record['iso_eventtime']
                fielddate = eventtime[0:10]
                if fielddate < "2018-08-31" and message[12] == 'b':
                    msg = yson.loads(message[14:-1])    # OLD FORMAT
                else:
                    msg = json.loads(message[12:])  # NEW FORMAT STARTS FROM 31 Aug 2018
                messageId = msg['Directive']['refMessageId']
                sessionId = msg['Session']['SessionId']
                status = msg['Directive']['status']

                vins(Record(messageId=messageId,
                            sessionId=sessionId,
                            status=status,
                            fielddate=fielddate))
            except:
                err(Record(fielddate=fielddate, message=message))

        if message.find('RequestStat') != -1:
            try:
                eventtime = record['iso_eventtime']
                fielddate = eventtime[0:10]
                if fielddate < "2018-08-31" and message[12] == 'b':
                    msg = yson.loads(message[14:-1])    # OLD FORMAT
                else:
                    msg = json.loads(message[12:])  # NEW FORMAT STARTS FROM 31 Aug 2018
                Time = msg['Event']['event']['payload']['durations']['onRecognitionEndTime-onVinsResponseTime']
                messageId = msg['Event']['event']['payload']['refMessageId']
                sessionId = msg['Session']['SessionId']

                stat(Record(messageId=messageId,
                            sessionId=sessionId,
                            Time=int(Time),
                            fielddate=fielddate))
            except:
                err(Record(fielddate=fielddate, message=message))


def leave_one(groups):
    for key, records in groups:
        for rec in records:
            yield Record(key, rec)
            break


def compute_metrics(dates_param, report_config, error_path, save_vins_path, save_stat_path):
    """ date_param - период дат с присутствующими логами в home/logfeller/logs/alice-production-uniproxy/1d/
        report_config - конфигурация таблицы на статистике для публикации результата
        error_path - путь куда положить табличку со строками, которые не удалось распарсить,
                может быть None, тогда ошибки не запишутся
        save_vins_path, save_stat_path - путь к табличкам в YT HAHN для записи выделенных строк с
                    message типа VinsPartial или RequestStat, могут быть None, тогда таблички не выгружаются
    """

    cluster = clusters.Hahn(pool='voice').env(
        files=list_deps(neighbours_for=__file__,
                        neighbour_names=None,
                        include_utils=True),
            ).env(templates=dict(dates=dates_param))
    job = cluster.job('Vins partials usage monitoring')

    vins1, stat1, err1 = job.table('home/logfeller/logs/qloud-runtime-log/1d/@dates') \
        .map(mapper1, intensity='large_data')

    vins2, stat2, err2 = job.table('home/logfeller/logs/alice-production-uniproxy/1d/@dates') \
        .map(mapper2, intensity='large_data')

    if error_path is not None:
        job.concat(err1, err2).put(error_path)

    vins = job.concat(vins1, vins2)\
        .groupby('fielddate', 'messageId', 'sessionId') \
        .reduce(leave_one)

    if save_vins_path is not None:
        vins.put(save_vins_path)
    if save_stat_path is not None:
        job.concat(stat1, stat2).put(save_stat_path)

    table = vins.join(job.concat(stat1, stat2), by=['messageId', 'sessionId', 'fielddate'], type='inner') \
        .groupby('fielddate') \
        .aggregate(number_bad=na.count(predicate=qf.equals('status', 'bad_partial')),
                   number_good=na.count(predicate=qf.equals('status', 'good_partial')),
                   number_206=na.count(predicate=qf.equals('status', 'partial_206')),
                   number_no_response=na.count(predicate=qf.equals('status', 'no_response')),
                   percentile_bad=na.quantile('Time', (0.5, 0.95, 0.99, 0.999),
                                                predicate=qf.equals('status', 'bad_partial')),
                   percentile_good=na.quantile('Time', (0.5, 0.95, 0.99, 0.999),
                                                 predicate=qf.equals('status', 'good_partial')),
                   percentile_206=na.quantile('Time', (0.5, 0.95, 0.99, 0.999),
                                                predicate=qf.equals('status', 'partial_206')))

    table = table.project('fielddate', 'number_bad', 'number_good', 'number_206', 'number_no_response',
                          p50_bad=ne.custom(lambda array: array[0][1], 'percentile_bad'),
                          p95_bad=ne.custom(lambda array: array[1][1], 'percentile_bad'),
                          p99_bad=ne.custom(lambda array: array[2][1], 'percentile_bad'),
                          p999_bad=ne.custom(lambda array: array[3][1], 'percentile_bad'),
                          p50_good=ne.custom(lambda array: array[0][1], 'percentile_good'),
                          p95_good=ne.custom(lambda array: array[1][1], 'percentile_good'),
                          p99_good=ne.custom(lambda array: array[2][1], 'percentile_good'),
                          p999_good=ne.custom(lambda array: array[3][1], 'percentile_good'),
                          p50_206=ne.custom(lambda array: array[0][1], 'percentile_206'),
                          p95_206=ne.custom(lambda array: array[1][1], 'percentile_206'),
                          p99_206=ne.custom(lambda array: array[2][1], 'percentile_206'),
                          p999_206=ne.custom(lambda array: array[3][1], 'percentile_206')
                          )

    table.publish(report_config)

    job.run()


def main(date_start, date_end, report_path, error_path=None, save_vins_path=None, save_stat_path=None):
    client = make_stat_client()

    report_config = ns.StatfaceReport() \
        .path(report_path) \
        .title('Vins Partials Metrics') \
        .scale('daily') \
        .dimensions(ns.Date('fielddate')) \
        .measures(
        ns.Number('number_bad'),
        ns.Number('number_good'),
        ns.Number('number_206'),
        ns.Number('number_no_response'),
        ns.Number('p50_bad'),
        ns.Number('p95_bad'),
        ns.Number('p99_bad'),
        ns.Number('p999_bad'),
        ns.Number('p50_good'),
        ns.Number('p95_good'),
        ns.Number('p99_good'),
        ns.Number('p999_good'),
        ns.Number('p50_206'),
        ns.Number('p95_206'),
        ns.Number('p99_206'),
        ns.Number('p999_206')
    ).client(client)

    date_param = '{' + date_start + '..' + date_end + '}'

    compute_metrics(date_param, report_config, error_path, save_vins_path, save_stat_path)


if __name__ == '__main__':
    call_as_operation(main)