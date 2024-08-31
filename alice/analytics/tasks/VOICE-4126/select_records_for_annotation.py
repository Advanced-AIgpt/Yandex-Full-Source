#!/usr/bin/env python
import argparse
import datetime
import json
import logging
import random
import re
import sys
import time
import yt.wrapper as yt
import yt.yson as yson

from yql.api.v1.client import YqlClient


if __name__ == '__main__':
    root_logger = logging.getLogger('')
    file_logger = logging.StreamHandler()
    file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
    root_logger.addHandler(file_logger)
    root_logger.setLevel(logging.DEBUG)

    parser = argparse.ArgumentParser()
    parser.add_argument('--yql-cond', metavar='YQL_EXPR_FILE', type=str,
        help='text file with yql condition for filter records (sample file context: lang regexp "ru"'
            ' AND source regexp  "(27fbd96d-ec5b-4688-a54d-421d81aa8cd2|mobile-navi)"'
            ' AND topic regexp "mapsyari")')
    parser.add_argument('--recs-limit', metavar='RECS_LIMIT', type=int, default=1000,
        help='result records limit (default: 1000)')
    parser.add_argument('--result', metavar='RESULT_JSON', type=str,
        help='filename for result (json list of records(dicts))')
    parser.add_argument('--result-mds', metavar='RESULT_MDS', type=str,
        help='filename for result with mds_keys (one per line)')
    parser.add_argument('--result-dates', metavar='DATES_FILENAME', type=str,
        help='filename for writing used date(s) YYYY-MM-DD[__YYYY-MM-DD]')
    parser.add_argument('--from-date', metavar='FROM_DATE', type=str, default='-0',
        help=("date for first log table (YYYY-MM-DD) or '-' + time_delta " +
              "(days) from TO_DATE (default: 0 == use only TO_DATE day)"))
    parser.add_argument('--to-date', metavar='TO_DATE', type=str, default='yesterday',
        help='date for last log table (YYYY-MM-DD or yesterday) (default: yesterday)')
    parser.add_argument('--bl-delta', type=int, default=14,
        help='not use records with alredy used uuid for given period (days; default: 14)')
    parser.add_argument('--bl-folder', type=str,
        help='folder with old tasks for annnotations - get from this tables uuid-s for build & use uuid-s blacklist'
        '\nsample: //tmp/and42/toloka-jobs')
    parser.add_argument('--yt-token', metavar='YT_TOKEN', type=str,
                        help='YT security token')
    parser.add_argument('--yql-token', metavar='YQL_TOKEN', type=str,
                        help='YQL security token')

    context = parser.parse_args()

    if not context.result:
        raise Exception('need RESULT_JSON')
    if not context.yql_cond:
        raise Exception('need YQL_COND')
    if not context.yql_token:
        raise Exception('need YQL_TOKEN')

    yt.config["proxy"]["url"] = "hahn.yt.yandex.net"
    if context.yt_token:
        yt.config["token"] = context.yt_token

    with open(context.yql_cond) as f:
        yql_cond = f.read().strip()

    # build used log tables lists
    if context.to_date == 'yesterday':
        to_date = datetime.datetime.now() - datetime.timedelta(1)
    else:
        to_date = datetime.datetime.strptime(context.to_date, '%Y-%m-%d')
    if context.from_date.startswith('-'):
        days_delta = int(context.from_date[1:])
        from_date = to_date - datetime.timedelta(days_delta)
    else:
        from_date = datetime.datetime.strptime(context.from_date, '%Y-%m-%d')
    it_date = from_date
    list_dates = []
    while it_date <= to_date:
        list_dates.append(it_date.strftime('%Y-%m-%d'))
        it_date += datetime.timedelta(1)
    if not list_dates:
        raise Exception('invalid from .. to date range (from > to)')

    logging.debug('use logs from days {}'.format(list_dates))
    logs_tables = ','.join("[home/voice-speechbase/uniproxy/logs_v2/{}]".format(date) for date in list_dates)
    # get most fresh tables with annotation jobs for build uuid-s black-list
    if context.bl_folder:
        folder = context.bl_folder.rstrip('/')
        logging.debug('search fresh tables in folder {} (for build uuid-s black-list))'.format(folder))
        re_date = re.compile('^\d\d\d\d-\d\d-\d\d$')
        min_fresh_dt = datetime.datetime.now() - datetime.timedelta(context.bl_delta)
        min_fresh_date = min_fresh_dt.strftime('%Y-%m-%d')
        old_tables = yt.list(folder)
        # per day tables
        most_fresh_tables = [tn for tn in old_tables if re_date.match(tn) and tn >= min_fresh_date]
        # and append range tables
        re_date = re.compile('^\d\d\d\d-\d\d-\d\d__\d\d\d\d-\d\d-\d\d$')
        most_fresh_tables += [tn for tn in old_tables if re_date.match(tn) and tn[:-10] >= min_fresh_date]
        min_ts = int(time.mktime(min_fresh_dt.timetuple()))
    else:
        most_fresh_tables = None
    if most_fresh_tables:
        logging.debug('for build black-list use tables  {})'.format(most_fresh_tables))
        build_black_list_uuids = '$black_uu=(SELECT DISTINCT uuid FROM CONCAT(\n  {}\n) WHERE ts > {});'.format(
            ', '.join(['[{}/{}]'.format(folder.strip('/'), tn) for tn in most_fresh_tables]), min_ts)
        filter_black_list_uuids = 'LEFT ONLY JOIN $black_uu AS bl ON sb.uuid == bl.uuid'
    else:
        logging.warning('skip using black-list filter for uuid-s (not found fresh tables)')
        build_black_list_uuids = ''
        filter_black_list_uuids = ''

    out_table = "tmp/voice-annotation-{}-".format(random.randint(1, 100000000)) + datetime.datetime.now().strftime('%Y-%m-%d-%H-%M-%S')

    query = """PRAGMA inferscheme;

$a=(SELECT mds_key, xml_key, requestId as reqid, audiobit, audiorate, is_sound
 , DateTime::ToSeconds(DateTime::FromString(date_created)) AS ts, uuid
 FROM RANGE([home/voice-speechbase/uniproxy/logs_v2], [{from_date}], [{to_date}]) AS tbl
 WHERE
  {conditions}
  AND mds_key != ""
);

$b=(SELECT mds_key, xml_key, ts, uuid, RANDOM(reqid) AS rnd
 FROM $a
 WHERE audiobit == 16 AND audiorate==16000 AND is_sound
 );
""".format(
        conditions=yql_cond,
        from_date=from_date.strftime('%Y-%m-%d'),
        to_date=to_date.strftime('%Y-%m-%d'),
    )

    if context.bl_delta:
        query += """
$uu=(SELECT MAX(sb.rnd) AS max_rnd, sb.uuid AS uuid
 FROM $b AS sb
 GROUP BY uuid);

{build_black_list_uuids}

INSERT INTO [{out_table}]
 SELECT sb.mds_key AS mds_key, sb.xml_key AS xml_key, sb.ts AS ts, sb.uuid AS uuid, sb.rnd AS rnd
  FROM $b AS sb
  INNER JOIN $uu AS uu ON uu.max_rnd == sb.rnd AND uu.uuid == sb.uuid
  {filter_black_list_uuids}
  ORDER BY rnd
  LIMIT {limit};
""".format(
        out_table=out_table,
        limit=context.recs_limit,
        build_black_list_uuids=build_black_list_uuids,
        filter_black_list_uuids=filter_black_list_uuids,
    )
    else:
        query += """
INSERT INTO [{out_table}]
 SELECT sb.mds_key AS mds_key, sb.xml_key AS xml_key, sb.ts AS ts, sb.uuid AS uuid, sb.rnd AS rnd
  FROM $b AS sb
  ORDER BY rnd
  LIMIT {limit};
        """.format(
            out_table=out_table,
            limit=context.recs_limit,
        )

    client = YqlClient(db='hahn', token=context.yql_token)
    request = client.query(query)
    logging.info('run YQL query {}'.format(query))
    request.run()  # just start the execution
    logging.info('wait query result')
    results = request.get_results()  # wait execution finish
    if not results.is_success:
        logging.error('request failed: status={}'.format(results.status))
        if results.errors:
            for error in results.errors:
                logging.error('returned error: {}'.format(error))
        logging.info('remove temporaty table {}'.format(tmp_table_name))
        exit(1)
    logging.info('query is success')

    # read table to result files
    out_table = '//' + out_table
    recs = yt.read_table(out_table)
    if context.result_mds:
        mds_f = open(context.result_mds, 'w')
    res_f = open(context.result, 'w')
    cnt = 0
    for rec in recs:
        cnt += 1
        if cnt == 1:
            res_f.write('[\n')
        else:
            res_f.write(',\n')
        res_f.write(json.dumps(yson.yson_to_json(rec)))
        if mds_f:
            mds_f.write(rec['mds_key'] + '\n')

    res_f.write('\n]')
    res_f.close()
    if mds_f:
        mds_f.close()
    logging.debug('read from table {} {} recs'.format(out_table, cnt))

    logging.debug('remove temporary output table {}'.format(out_table))
    yt.remove(out_table)

    if context.result_dates:
        with open(context.result_dates, 'w') as f:
            if len(list_dates) == 1:
                f.write(list_dates[0])
            else:
                f.write(list_dates[0] + '__' + list_dates[-1])

    logging.info('success finish')
