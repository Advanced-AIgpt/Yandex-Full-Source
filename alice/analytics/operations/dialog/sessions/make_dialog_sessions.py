# -*-coding: utf8 -*-

import os
from collections import OrderedDict
import time
from datetime import datetime, timedelta

from nile.api.v1 import (
    Record,
    grouping as ng,
    extractors as ne,
    datetime as nd,
    filters as nf,
    aggregators as na,
    with_hints,
)
from qb2.api.v1 import typing as qt

from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
import utils.yt.basket_common as common

from usage_fields import get_extractors, get_filters
from replica_constructor import merge_callbacks, PER_SESSION_PARAMS

INTERMEDIATE_SESSIONS_SCHEMA = OrderedDict([
    ('session', qt.Optional[qt.Json]),
    ('uuid', qt.Optional[qt.String]),
    ('icookie', qt.Optional[qt.String]),
    ('puid', qt.Optional[qt.String]),
    ('user_id', qt.Optional[qt.String]),
    ('device_id', qt.Optional[qt.String]),
    ('device_revision', qt.Optional[qt.String]),
    ('app', qt.Optional[qt.String]),
    ('country_id', qt.Optional[qt.Int64]),
    ('lang', qt.Optional[qt.String]),
    ('experiments', qt.Optional[qt.Json]),
    ('is_exp_changed', qt.Optional[qt.Bool]),
    ('platform', qt.Optional[qt.String]),
    ('version', qt.Optional[qt.String]),
    ('session_id', qt.Optional[qt.String]),
    ('fielddate', qt.Optional[qt.String]),
    ('testids', qt.Optional[qt.Json]),
    ('enviroment_state', qt.Optional[qt.Json]),
    ('device', qt.Optional[qt.String]),
    ('subscription', qt.Optional[qt.String]),
])

SCHEMA = OrderedDict([
    ('cohort', qt.Optional[qt.String]),
    ('is_new', qt.Optional[qt.String]),
    ('first_day', qt.Optional[qt.String]),
])
SCHEMA.update(INTERMEDIATE_SESSIONS_SCHEMA)


MAX_QUERIES_IN_SESSION = 10000
# Конструирование сессии


@with_hints(output_schema=INTERMEDIATE_SESSIONS_SCHEMA)
def get_sessions(groups):
    for key, records in groups:
        # req_id is in groupby keys for tech sessions
        # but req_id is needed fo log_item['session'] not for log_item
        log_item = {f: key[f] for f in ('uuid', 'app', 'platform', 'version')}

        log_item['session'] = session = []
        session_idx = 0
        log_item['session_id'] = '%s_%s_%s_%s' % (
            key.uuid,
            records.current.ms_server_time,
            records.current.req_id,
            session_idx)

        already_seen = ''
        previous_time = records.current.ts

        log_item['fielddate'] = records.current.fielddate
        log_item['testids'] = records.current.testids
        log_item['country_id'] = records.current.country_id
        log_item['lang'] = records.current.lang
        log_item['device'] = records.current.device
        log_item['device_id'] = records.current.device_id
        log_item['device_revision'] = records.current.device_revision
        log_item['puid'] = records.current.puid
        log_item['icookie'] = records.current.icookie
        log_item['user_id'] = records.current.device_id or key.uuid
        log_item['subscription'] = records.current.subscription
        log_item['enviroment_state'] = records.current.enviroment_state
        log_item['enrollment_headers'] = records.current.enrollment_headers
        log_item['guest_data'] = records.current.guest_data
        if records.current.app.startswith('yabro') and records.current.user_id_from_cookies:
            log_item['user_id'] = records.current.user_id_from_cookies

        experiments = set(records.current.experiments)
        log_item['experiments'] = sorted(experiments)
        log_item['is_exp_changed'] = False

        for record in records:
            rec = record.to_dict()
            composite_id = '%s%s' % (rec['ts'], rec['req_id'])
            if composite_id == already_seen:
                continue
            already_seen = composite_id

            if (rec.get('callback_name') == 'on_reset_session' or len(session) >= MAX_QUERIES_IN_SESSION):
                # or (len(session) % 50 == 0 and len(cyson.dumps(log_item)) > 8 * 1024 * 1024)
                if session:
                    yield Record(**log_item)

                    log_item['session'] = session = []
                    session_idx += 1
                    id_prefix = '_'.join(log_item['session_id'].split('_')[:-1])
                    log_item['session_id'] = '%s_%s' % (id_prefix, session_idx)
                continue
            # fielddate, testids, etc. are needed for log_item but not for log_item['session']
            replica = {key: rec[key] for key in rec.keys() if key not in PER_SESSION_PARAMS}

            replica['delta'] = rec['ts'] - previous_time
            previous_time = rec['ts']

            session.append(replica)

            if set(rec['experiments']) != experiments:
                log_item['is_exp_changed'] = True

        if session:
            yield Record(**log_item)


def get_user_table(groups):
    for key, records in groups:
        for record in records:
            yield record
            break


def is_new(day, fielddate):
    days = (nd.Datetime.from_iso(fielddate) - nd.Datetime.from_iso(day)).days
    if days < 7:
        return '1 week'
    else:
        return '>1 week'


def get_cohort(init_date):
    # return start of week from init_date
    date = datetime.strptime(init_date, '%Y-%m-%d')
    return (date - timedelta(days=date.weekday())).strftime('%Y-%m-%d')


def filter_greetings(distinct_intents):
    """
    Filter out if uuid only had get_greetings messages during this day
    :return:
    """
    if len(distinct_intents) == 1 and distinct_intents[0] == 'personal_assistant\tscenarios\tskill_recommendation\tget_greetings':
        return False
    return True


def filter_development_devices(device):
    # https://st.yandex-team.ru/VA-1947
    if device == "Yandex development":
        return False
    return True


def make_usage_logs(job, expboxes, date):
    us = (job
          .table(expboxes)
          .label('expboxes_table')
          .qb2(log='generic-yson-log',
               fields=get_extractors(date),
               filters=get_filters()))

    filtered_by_greetings = (us
                             .groupby('uuid')
                             .aggregate(distinct_intents=na.distinct('intent'))
                             .filter(nf.custom(filter_greetings, 'distinct_intents')))

    filtered_by_fatness = (us
                           .groupby('uuid')
                           .aggregate(cnt=na.count())
                           .filter(nf.custom(lambda x: x < 1500, 'cnt')))

    us = (us
          .join(filtered_by_greetings, by='uuid', assume_unique_right=True)
          .join(filtered_by_fatness, by='uuid', assume_unique_right=True)
          .groupby('uuid', 'app', 'platform', 'version')
          .sort('client_time', 'ms_server_time', 'req_id')
          .reduce(merge_callbacks))

    main_us, tech_us = us.split(nf.equals('type', 'tech'))

    main_sessions = (main_us
                     .groupby('uuid', 'app', 'platform', 'version')
                     .sort('ts', 'ms_server_time', 'req_id')
                     .groupby(ng.sessions('ts', timeout=30 * 60))
                     .reduce(get_sessions, memory_limit=16 * 1024)
                     .filter(nf.custom(filter_development_devices, 'device')))

    tech_sessions = (tech_us
                     .groupby('uuid', 'app', 'platform', 'version', 'req_id')
                     .reduce(get_sessions, memory_limit=16 * 1024)
                     .filter(nf.custom(filter_development_devices, 'device')))

    return job.concat(main_sessions, tech_sessions)


def make_sessions(job, sessions_path, expboxes, users_path, devices_path, date):
    io_option = {"table_writer": {"max_row_weight": 134217728}}
    job = job.env(default_memory_limit=(10*1024),
                  yt_spec_defaults={"job_io": io_option, "partition_job_io": io_option,
                                    "merge_job_io": io_option, "map_job_io": io_option,
                                    "reduce_job_io": io_option, "sort_job_io": io_option},
                  yt_auto_merge={"final_tables": "disabled", "tmp_tables": "disabled"},
                  merge_strategy={"final_tables": "never", "tmp_tables": "never"})

    sessions_table = make_usage_logs(job, expboxes, date)
    users_table = job.table(users_path).label('users_table')
    devices_table = (
        job.table(devices_path).label('devices_table')
        .project('device_id', 'init_date')
        .groupby('device_id')
        .aggregate(init_date=na.min('init_date'))
    )

    not_speakers, speakers = (
        sessions_table
        .split(nf.custom(lambda app: app in common.QUASAR_APPS, 'app'))
    )
    speakers = speakers.join(devices_table, type='left', by='device_id', assume_unique_right=True)

    sessions_table = (
        not_speakers.concat(speakers)
        .join(users_table, by='uuid', assume_unique_right=True)
        .project(
            ne.all(),
            cohort=ne.custom(lambda a, b: get_cohort(a) if a else b, 'init_date', 'cohort')
                .with_type(qt.Optional[qt.String]),
            first_day=ne.custom(lambda a, b: a if a else b, 'init_date', 'first_day').with_type(qt.Optional[qt.String])
        )
        .project(*(set(SCHEMA.keys()) - {'is_new'}), is_new=ne.custom(is_new, 'first_day', 'fielddate').with_type(qt.Optional[qt.String]))
        .label('sessions_table')
        .put(sessions_path, schema=SCHEMA)
    )

    return job


def main(date,
         sessions_root,
         expboxes,
         users_path='//home/alice/dialog/misc/uuids_info',
         devices_path='//home/marketing-data/andrewha/Station/backup/quasar_init',
         pool=None,
         tmp_path=None,
         custom_cluster_params=None,
         include_utils=True,
         neighbour_names=['usage_fields.py', 'replica_constructor.py', 'intent_scenario_mapping.py']):
    """
    Создание таблички с сессиями диалогов
    :param str|list|tuple date: Дата "YYYY-MM-DD",
        имя входной таблицы логов и имя выходной таблицы с сессиями вычисляется из неё
        Может быть задана пара из начальной и конечной даты. Тогда будет создано табличек по количеству дней
    :param str sessions_root: YT-папка, куда выгружать табличку
    :param str dialogs_root: YT-папка, в которой искать табличку с логами за день `date`
    :param str subscriptions_root: YT-папка с данными про подписки puid-а
    :param str users_path: YT-таблица, из которой брать инфу по пользователям
    :param str devices_path: YT-таблица, в которой хранится информация о девайсах
    :param str|None pool: название YT пула
    :param str|None tmp_path: путь для хранения tmp файлов нирваны.
    пример для параллельных запусков в нирване: tmp_path='//tmp/username/nirvana-tmp-${datetime.timestamp}'
    //tmp/username должна существовать до запуска транзакции. иначе при параллельных инстансах будет проблема с локом.
    :rtype: dict[str, str]
    :return: Путь к выгруженной таблице с сессиями
    """

    templates = dict(tmp=tmp_path) if tmp_path else None
    sessions_path = os.path.join(sessions_root, date)
    cluster = hahn_with_deps(pool=pool,
                             use_yql=True,
                             templates=templates,
                             neighbours_for=__file__,
                             neighbour_names=neighbour_names,
                             custom_cluster_params=custom_cluster_params,
                             include_utils=include_utils)

    job = cluster.job()

    job = make_sessions(job, expboxes, sessions_path, users_path, devices_path, date)

    with job.driver.transaction():
        job.run()
    return {'sessions_path': sessions_path}


if __name__ == '__main__':
    st = time.time()
    print('start at', time.ctime(st))
    call_as_operation(main)
    # main('2020-02-20',
    #      '//home/voice/ilnur/VA-1264/prod/sessions',
    #      '//home/voice/ilnur/VA-1264/prod/dialogs',
    #      '//home/voice/ilnur/VA-1264/uids_info_2020_02_20_2'
    #  )  # local run
    print('total elapsed {:2f} min'.format((time.time() - st) / 60))
