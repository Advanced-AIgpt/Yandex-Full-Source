# -*-coding: utf8 -*-
import datetime
import time
from functools import partial
from os import path
from qb2.api.v1 import typing as qt, filters as qf, extractors as qe
from nile.api.v1 import filters as nf, extractors as ne, aggregators as na, Record, with_hints

from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
import utils.yt.basket_common as common


@with_hints(output_schema=dict(
    req_id=qt.Optional[qt.String],
    uuid=qt.Optional[qt.String],
    server_time_ms=qt.Optional[qt.UInt64],
    session_id=qt.Optional[qt.String],
    reversed_session_sequence=int,
    location=qt.Optional[qt.Json],
    client_tz=qt.Optional[qt.String],
    client_time=qt.Optional[qt.Int64],
    main_req_id=qt.Optional[qt.String],
    session_sequence=qt.Optional[qt.Int64],
    main_session_sequence=qt.Optional[qt.Int64],
    generic_scenario=qt.Optional[qt.String],
    intent=qt.Optional[qt.String],
    app=qt.Optional[qt.String],
    device=qt.Optional[qt.String],
    query=qt.Optional[qt.String],
    reply=qt.Optional[qt.String],
    request_source=qt.Optional[qt.String],
    input_type=qt.Optional[qt.String],
    ))
def get_context_reducer(groups, context_len):
    """ принимает сессии , выбранные из expboxes, оставляет context_len перед основным запросом
        и добавляет нумерацию запросов"""
    for key, records in groups:
        records_dct = {x.to_dict()["session_sequence"]: x.to_dict() for x in records}
        session_seq = records_dct.values()[0]["main_session_sequence"]
        records_yielded = 0
        records_to_yield = []
        while session_seq in records_dct and records_yielded <= context_len:
            records_to_yield.append(records_dct[session_seq])
            records_yielded += 1
            session_seq -= 1

        ind = len(records_to_yield) - 1
        for reversed_ind, rec in enumerate(records_to_yield):
            rec["reversed_session_sequence"] = reversed_ind
            rec["session_sequence"] = ind
            ind -= 1
            yield Record.from_dict(rec)


def main(reqids, start_date=None, end_date=None, output_table=None, add_context=True, pool=None,
         request_id_column_name='request_id', unique_uuids=False, tmp_path=None, memory_limit=1024, context_len=1,
         store_checkpoints=False, voice_source='asr', use_input_uuids=False, use_session_ids=False):

    random_part = common.random_part(16)
    output_table = output_table or "//tmp/robot-voice-qa/new_scenarios_basket_" + random_part
    templates = {"job_root": "//tmp/robot-voice-qa",
                 'checkpoints_root': path.dirname(output_table)
                 }
    if tmp_path is not None:
        templates['tmp_files'] = tmp_path

    """ при отсутствии входящих дат выбираем интервал в 296 дней, с окончанием 5 дней назад """
    if start_date is None:
        start_date = (datetime.datetime.now() - datetime.timedelta(days=300)).strftime("%Y-%m-%d")
    if end_date is None:
        end_date = (datetime.datetime.now() - datetime.timedelta(days=5)).strftime("%Y-%m-%d")

    date_str = '{' + start_date + '..' + end_date + '}'

    assert voice_source in {'asr', 'uniproxy'}

    """ инициализация доступа к кластеру"""
    cluster = hahn_with_deps(templates=templates, include_utils=True, pool=pool, use_yql=True)
    io_option = {"table_writer": {"max_row_weight": 134217728}}
    job = cluster.job().env(yt_spec_defaults={"job_io": io_option, "partition_job_io": io_option,
                                              "merge_job_io": io_option, "map_job_io": io_option,
                                              "reduce_job_io": io_option, "sort_job_io": io_option},
                            default_memory_limit=memory_limit)

    """ берём входящую таблицу запросов, выделяем ключ для джойна (с добавлением uuid если есть) """
    reqids_join_key = ('uuid', 'req_id') if use_input_uuids else ('req_id',)
    reqids_project_extras = ['uuid', ] if use_input_uuids else []
    reqids_table = job.table(reqids) \
        .project(qe.log_field('request_source', None),
                 req_id=request_id_column_name,
                 *reqids_project_extras)

    """ берём prepared за нужные даты и только с нужными полями (фильтруем позже) """
    flattened_sessions = job.table(path.join('//home/alice/dialog/prepared_logs_expboxes', date_str)) \
        .project(
            'req_id', 'uuid', 'server_time_ms', 'session_id', 'session_sequence',
            'location', 'client_tz', 'client_time', 'do_not_use_user_logs',
            'generic_scenario', 'intent', 'app', 'device', 'query', 'reply', 'input_type',
        )

    """ ищем session_id по входящим рекидам или берём готовую таблицу с session_id (если они есть на входе)"""
    if use_session_ids and use_input_uuids:
        used_session_ids = job.table(reqids) \
            .project(
                'session_id',
                'request_source',
                main_uuid='uuid',
                main_req_id=request_id_column_name,
                main_session_sequence='session_sequence',
                main_input_type='input_type',
            )
    else:
        used_session_ids = flattened_sessions \
            .join(reqids_table, by=reqids_join_key, assume_small_right=True,
                  force_unique=True, allow_undefined_keys=False, assume_defined=True,) \
            .filter(qf.defined('req_id')) \
            .project(
                'session_id',
                'request_source',
                main_uuid='uuid',
                main_req_id='req_id',
                main_session_sequence='session_sequence',
                main_input_type='input_type'
            )
    used_session_ids.checkpoint(random_part + '_used_session_ids')

    if unique_uuids:
        used_session_ids = used_session_ids.unique('main_uuid')

    """ собираем все запросы с выбранными session_id, применяем фильтры"""
    used_sessions = used_session_ids \
        .join(flattened_sessions, by='session_id', assume_small_left=True, type='inner',
              allow_undefined_keys=False, assume_defined=True,) \
        .unique('main_req_id', 'session_id', 'session_sequence') \
        .project(
            ne.all(exclude='input_type'),
            input_type=ne.custom(common.fix_click_input_type, 'input_type', 'query'
                                 ).with_type(qt.Optional[qt.String])
        ) \
        .filter(
            qf.one_of('input_type', {'text', 'voice'}),
            *common.get_common_filters()
        ) \
        .checkpoint(random_part + '_used_sessions_')

    reduced_sessions = used_sessions \
        .groupby('main_req_id') \
        .reduce(partial(get_context_reducer, context_len=context_len)) \
        .filter(qf.defined('req_id')) \
        .checkpoint(random_part + '_sessions_with_context')

    """ выборка из таблиц wonder/logs с полями голоса
        выбор полей - джойн с рекидами со входа - преобразование полей """

    def get_mds_key(wonderlogs_asr):
        """ получает mds_key в зависимости от выбранного источника голоса (global voice_source)"""
        if voice_source == 'asr':
            return getattr(getattr(wonderlogs_asr, 'data', None), 'mds_key', None)
        elif voice_source == 'uniproxy':
            mds_uniproxy = getattr(getattr(wonderlogs_asr, 'voice_by_uniproxy', None), 'mds', None)
            return '/'.join(mds_uniproxy.split('/')[-2:]) if mds_uniproxy else None

    voice_url_template = 'https://speechbase.voicetech.yandex-team.ru/getfile/{}?norm=1' \
        + ('&storage-type=s3&s3-bucket=voicelogs' if voice_source == 'asr' else '')

    reqids_table_for_wl_join = reduced_sessions \
        .filter(qf.defined('main_req_id')) \
        .project(
            'server_time_ms',
            'req_id',
            uuid=ne.custom(lambda x: x[3:], 'uuid').with_type(qt.Optional[qt.String]),
        )

    voice_requests = job.table(path.join('//home/alice/wonder/logs', date_str), ignore_missing=True) \
        .project('_server_time_ms', '_uuid', '_megamind_request_id', 'asr') \
        .join(reqids_table_for_wl_join,
              by_left=('_server_time_ms', '_uuid', '_megamind_request_id'),
              by_right=('server_time_ms', 'uuid', 'req_id'),
              assume_small_right=True, force_unique=True,
              allow_undefined_keys=False, assume_defined=True) \
        .project(
            'req_id',
            mds_key=ne.custom(get_mds_key, 'asr').with_type(qt.Optional[qt.String]),
            voice_url=ne.custom(lambda key: voice_url_template.format(key) if key else None,
                                'mds_key').with_type(qt.Optional[qt.String]),
        )

    """ выборка из vins/logs/dialogs, затем обработка полей"""
    vins_logs_requests = job.table(path.join("//home/voice/vins/logs/dialogs", date_str), ignore_missing=True) \
        .project('experiments', 'form_name', 'server_time_ms', 'uuid', 'request', 'analytics_info') \
        .join(reduced_sessions.project('server_time_ms', 'uuid'),
              by=('server_time_ms', 'uuid'), assume_small_right=True, force_unique=True,
              allow_undefined_keys=False, assume_defined=True) \
        .project(
            ne.all(exclude=('request', 'analytics_info')),
            req_id=ne.custom(lambda request: request.get('request_id'),
                             'request').with_type(qt.Optional[qt.String]),
            device_state=ne.custom(lambda request: request.get('device_state'),
                                   'request').with_type(qt.Optional[qt.Json]),
            iot_config=ne.custom(lambda ai: ai.get('iot_user_info'),
                                 'analytics_info').with_type(qt.Optional[qt.Json]),
            request_additional_options=ne.custom(lambda request: request.get('additional_options'),
                                                 'request').hide(),
            additional_options=ne.custom(common.clean_additional_options,
                                         'request_additional_options').with_type(qt.Optional[qt.Json])
        ).checkpoint(random_part + '_selected_vins_')

    """ BIG JOIN: когда готовы reduced_sessions - добавляем к ним голос и данные из vins/logs/dialogs """
    all_requests_with_audio_and_device_states = reduced_sessions \
        .join(voice_requests, by='req_id', type='left') \
        .checkpoint(random_part + '_requests_with_voice_') \
        .join(vins_logs_requests, by=('server_time_ms', 'uuid', 'req_id'),
              assume_small_left=True, force_unique_right=True, allow_undefined_keys=False) \
        .checkpoint(random_part + '_requests_with_voice_vins_') \
        .project(
            ne.all(),
            new_req_id=ne.custom(common.generate_id, 'req_id').with_type(qt.Optional[qt.String])
        )

    """ добавляем фейковые session_ids """
    session_ids = all_requests_with_audio_and_device_states \
        .groupby('main_req_id') \
        .reduce(
            with_hints(output_schema=dict(main_req_id=qt.Optional[qt.String],
                                          new_session_id=qt.Optional[qt.String],))
            (common.get_session_ids)
        ) \
        .filter(nf.custom(lambda session_id: len(session_id.split('__')) <= 2, 'new_session_id'))

    """ обрабатываем выходные поля """
    basket = all_requests_with_audio_and_device_states \
        .join(session_ids, by='main_req_id') \
        .project(
            ne.all(),
            toloka_intent=ne.const('other')
        ) \
        .qb2(
            log='generic-yson-log',
            fields=common.get_basket_fields(do_not_change_ss=True)
        )

    basket.put(output_table)

    with job.driver.transaction():
        job.run(store_checkpoints=store_checkpoints)

    return {"cluster": "hahn", "table": output_table}


if __name__ == '__main__':
    st = time.time()
    print 'start at', time.ctime(st)
    call_as_operation(main)
    print 'total elapsed {:2f} min'.format((time.time() - st) / 60)
