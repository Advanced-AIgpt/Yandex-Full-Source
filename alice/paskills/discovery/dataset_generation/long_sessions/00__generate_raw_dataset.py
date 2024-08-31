# Based on arc:/statbox/abt/metrics/statbox_abt_metrics/metrics/alice/external_skills_long_sessions_metrics.py

# -*-coding: utf8 -*-
from nile.api.v1 import (
    clusters,
    Record,
    aggregators as na,
    with_hints,
)

from datetime import datetime, timedelta
from itertools import product
from argparse import ArgumentParser
from os import path


@with_hints(
    output_schema=dict(
        skill_id=str,
        uuid=str,
        session_id=str,
        external_session_id=str,
        queries_number=int,
        has_errors=int,
    )
)
def map_get_values(records):
    for record in records:
        previous_session = None
        for session in record['session']:
            if not previous_session:
                previous_session = session
            tmp = record.to_dict()
            if session.get('skill_id'):
                tmp['skill_id'] = session.get('skill_id')
                if session.get('external_session_seq') and session.get('external_session_id'):
                    tmp['external_session_id'] = session.get('external_session_id') + record['session_id']
                    tmp['queries_number'] = session.get('external_session_seq')
                    tmp['has_errors'] = 0
                    previous_session = session
                    yield Record(**tmp)
                if session.get('error_type'):
                    tmp['external_session_id'] = str(previous_session.get('external_session_id'))\
                        + record['session_id']
                    tmp['queries_number'] = -1
                    tmp['has_errors'] = 1
                    previous_session = session
                    yield Record(**tmp)


@with_hints(
    output_schema=dict(
        uuid=str,
        skill_id=str,
        session_id=str,
        query_number=int,
        with_6_queries=int,
        with_3_queries=int,
        with_1_query=int,
        has_errors=int,
    )
)
def map_get_queries(records):
    for record in records:
        parsed_record = record.to_dict()
        if parsed_record['has_errors'] > 0 and parsed_record['query_number'] < 6:
            parsed_record['has_errors'] = 1
        else:
            parsed_record['has_errors'] = 0

        if parsed_record['query_number'] >= 6:
            parsed_record['with_6_queries'] = 1
            parsed_record['with_3_queries'] = 0
            parsed_record['with_1_query'] = 0
        elif parsed_record['query_number'] >= 3 and parsed_record['has_errors'] == 0:
            parsed_record['with_3_queries'] = 1
            parsed_record['with_6_queries'] = 0
            parsed_record['with_1_query'] = 0
        elif parsed_record['query_number'] >= 1 and parsed_record['has_errors'] == 0:
            parsed_record['with_3_queries'] = 0
            parsed_record['with_6_queries'] = 0
            parsed_record['with_1_query'] = 1
        else:
            parsed_record['with_6_queries'] = 0
            parsed_record['with_3_queries'] = 0
            parsed_record['with_1_query'] = 0
        yield Record(**parsed_record)


@with_hints(
    output_schema=dict(
        uuid=str,
        skill_id=str,
        session_id=str,
        is_long_session=int,
        query_number=int,
        has_errors=int,
    )
)
def map_count_sessions(records):
    for record in records:
        parsed_record = record.to_dict()
        if parsed_record['category'] == 'games_trivia_accessories':
            parsed_record['is_long_session'] = parsed_record['with_6_queries']
        elif parsed_record['category'] == 'kids':
            parsed_record['is_long_session'] = parsed_record['with_3_queries'] + parsed_record['with_6_queries']
        else:
            parsed_record['is_long_session'] = parsed_record['with_1_query'] + parsed_record['with_3_queries'] + parsed_record['with_6_queries']
        yield Record(**parsed_record)


def __compute_metrics(job, input_table, output_table):
    session = job.table(input_table)
    session.map(
        map_get_values,
    ).groupby(
        'external_session_id', 'skill_id', 'uuid', 'session_id'
    ).aggregate(
        query_number=na.max('queries_number'),
        has_errors=na.sum('has_errors'),
    ).map(
        map_get_queries
    ).join(
        job.table('//home/paskills/skills/stable'),
        by_left='skill_id',
        by_right='id',
        assume_unique_right=True,
        allow_undefined_keys=False
    ).map(
        map_count_sessions
    ).put(
        output_table
    )
    return session


def compute_metrics(date, start_date, end_date, sessions_root, scale, pool, results_path):

    assert (date or (start_date and end_date)), "you should specify dates: Either date or start_date AND end_date"

    cluster = clusters.Hahn(pool=pool)

    job = cluster.job('[Discovery] long sessions')

    sessions = []

    if start_date and end_date:

        end = datetime(*map(int, end_date.split('-')))
        start = datetime(*map(int, start_date.split('-')))

        assert start <= end, 'end_date is smaller then start_date'

        for i in range((end - start).days + 1):
            date_str = (start + timedelta(i)).isoformat()[:10]

            sessions.append(
                __compute_metrics(job,
                                  path.join(sessions_root, date_str),
                                  path.join(results_path, date_str)))
    else:
        sessions = __compute_metrics(job,
                                     path.join(sessions_root, date),
                                     path.join(results_path, date))
    job.run()


def main():
    arg_parser = ArgumentParser()
    arg_parser.add_argument('--date')
    arg_parser.add_argument('--start-date')
    arg_parser.add_argument('--end-date')
    arg_parser.add_argument('--scale', default='daily')
    arg_parser.add_argument('--pool', default='voice')
    arg_parser.add_argument('--sessions-root', default='//home/voice/dialog/sessions')
    arg_parser.add_argument('--tmp-path', default='//home/paskills/discovery/datasets/long-sessions')
    args = arg_parser.parse_args()
    compute_metrics(args.date, args.start_date, args.end_date,
                    args.sessions_root, args.scale, args.pool, args.tmp_path)


if __name__ == '__main__':
    main()
