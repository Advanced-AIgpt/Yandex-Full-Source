# -*-coding: utf8 -*-
from functools import partial
from nile.api.v1 import with_hints, extractors as ne, filters as nf
from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
from qb2.api.v1 import typing as qt, filters as qf

from slices_mapping import is_fresh_query
import generic_scenarios_in_basket as mappings
import utils.yt.basket_common as common


@with_hints(output_schema=dict(
    req_id=qt.Optional[qt.String],
    uuid=qt.Optional[qt.String],
    server_time_ms=qt.Optional[qt.UInt64],
    session_id=qt.Optional[qt.String],
    real_session_id=qt.Optional[qt.String],
    location=qt.Optional[qt.Json],
    client_tz=qt.Optional[qt.String],
    client_time=qt.Optional[qt.Int64],
    main_ts=qt.Optional[qt.Int64],
    main_req_id=qt.Optional[qt.String],
    session_sequence=qt.Optional[qt.Int64],
    main_session_sequence=qt.Optional[qt.Int64],
    generic_scenario=qt.Optional[qt.String],
    intent=qt.Optional[qt.String],
    toloka_intent=qt.Optional[qt.String],
    app=qt.Optional[qt.String],
    device=qt.Optional[qt.String],
    query=qt.Optional[qt.String],
    reply=qt.Optional[qt.String],
    voice_text=qt.Optional[qt.String],
    request_source=qt.Optional[qt.String],
    input_type=qt.Optional[qt.String],
    fielddate=qt.Optional[qt.String],
    testids=qt.Optional[qt.Json],
    ))
def get_context(groups, timedelta):
    for key, records in groups:
        session = sorted(list(records), key=lambda x: x['session_sequence'])
        main_ss = key.main_session_sequence

        # мы почистили сессию от дубликатов и приватных пользовательских запросов,
        # поэтому надо восстановить порядковые номера запросов в сессии
        valid_session = False
        for idx, record in enumerate(session):
            if record['session_sequence'] == key.main_session_sequence:
                main_ss = idx
                valid_session = True
                break
        if not valid_session:
            break

        context_idx = main_ss - 1
        # для умного дома ищется ближайший запрос не стоп, т.к. есть доп.вопрос про, спросили ли тоже самое
        if session[context_idx].get('generic_scenario') == 'iot_do':
            while context_idx >= 0 and session[context_idx].get('generic_scenario') == 'stop':
                context_idx -= 1
        if context_idx != -1 and \
                abs(session[context_idx].get('client_time') - session[main_ss].get('client_time')) <= timedelta:
            yield session[context_idx]
        yield session[main_ss]


def get_basket_scenarios_ban_set(apps):
    # определяет "свежий срез" и "старые сценарии" по whitelist'у из https://nda.ya.ru/t/rlRDckLK4voupY
    # TODO: https://st.yandex-team.ru/VA-1900 - заменить хардкод сценариев на сценарии из "больших" корзин
    apps_set = set(apps) if apps else set()
    full_ban_set = mappings.BAN_SET
    if apps_set & common.QUASAR_APPS:
        full_ban_set = full_ban_set.union(mappings.QUASAR_UE2E_SCENARIOS)
    if apps_set & common.GENERAL_APPS:
        full_ban_set = full_ban_set.union(mappings.GENERAL_UE2E_SCENARIOS)
    if apps_set & common.NAVI_APPS:
        full_ban_set = full_ban_set.union(mappings.NAVI_UE2E_SCENARIOS)
    return full_ban_set


def prepare_flattened_sessions(input_stream, apps=None):
    filters = [
        qf.or_(  # this was filter_input_types_by_app()
            qf.equals('input_type', 'voice'),
            qf.and_(qf.equals('input_type', 'text'), qf.one_of('app', common.GENERAL_APPS))
        )
    ]
    filters.extend(common.get_common_filters())
    if apps:
        filters.append(qf.one_of('app', apps))

    output_stream = input_stream \
        .project(
            'req_id', 'uuid', 'server_time_ms', 'session_id', 'session_sequence',
            'location', 'client_tz', 'client_time', 'do_not_use_user_logs',
            'generic_scenario', 'intent', 'app', 'device', 'query', 'reply',
            'testids', 'fielddate', 'voice_text',
            input_type=ne.custom(common.fix_click_input_type, 'input_type', 'query').with_type(qt.Optional[qt.String])
        ) \
        .filter(*filters)

    return output_stream


def main(start_date, end_date, generic_scenarios=None, intents_path=None, apps=None, size=7000, pool=None,
         preselected_basket=None,
         tmp_path=None, output_table=None, vicinity_requests_timedelta=20, memory_limit=8192,
         do_sample=True, unique_uuid=False, new_iot_users=False,
         checkpoints_path=None, new_ue2e_scenarios_sample_size=250):
    date_str = '{' + start_date + '..' + end_date + '}'
    random_part = common.random_part(16)

    templates = dict(job_root='//tmp/robot-voice-qa', date=date_str)
    if tmp_path is not None:
        templates['tmp_files'] = tmp_path
    if checkpoints_path:
        templates['checkpoints_root'] = checkpoints_path

    cluster = hahn_with_deps(
                pool=pool,
                use_yql=True,
                templates=templates,
                neighbours_for=__file__,
                neighbour_names=[
                    'visualize_quasar_sessions.py', 'generic_scenario_to_human_readable.py',
                    'intents_to_human_readable.py', 'slices_mapping.py', 'generic_scenarios_in_basket.py',
                    'standardize_answer.py'
                ],
                include_utils=True
              )
    io_option = {"table_writer": {"max_row_weight": 134217728}}
    job = cluster.job().env(default_memory_limit=memory_limit,
                            yt_spec_defaults={"job_io": io_option, "partition_job_io": io_option,
                                              "merge_job_io": io_option, "map_job_io": io_option,
                                              "reduce_job_io": io_option, "sort_job_io": io_option,
                                              'enable_job_splitting': False},
                            )

    output_table = output_table or "//tmp/robot-voice-qa/quasar_quality_task_" + random_part

    expboxes_stream = job.table('//home/alice/dialog/prepared_logs_expboxes/@date')

    if intents_path:
        # Уникализация по uuid нужна, т.к. запросы потом отправятся в аннотирование
        # и мы не можем аннотировать запросы от одного пользователя
        # ToDo: придумать что-то более умное, чтоб сохранялось распределение по активности пользователей
        # data from intents markup
        intents_tables = job.table(intents_path + "/@date")
        intents_sample = intents_tables \
            .filter(nf.custom(is_fresh_query, 'toloka_intent')) \
            .project('toloka_intent', req_id='reqid', request_source=ne.const('intents'))\
            .checkpoint(random_part + '_fresh_query_reqids')

        flattened_sessions = prepare_flattened_sessions(expboxes_stream, apps)

        intents_sample_with_session_ids = flattened_sessions \
            .project('req_id', 'client_time', 'session_id', 'session_sequence', 'uuid') \
            .join(intents_sample, by='req_id',
                  assume_small_right=True, allow_undefined_keys=False, assume_defined=True, force_unique=True) \
            .unique('uuid') \
            .random(count=size - new_ue2e_scenarios_sample_size) \
            .checkpoint(random_part + '_intents_sample_with_session_ids')

        new_scenarios_sample = flattened_sessions \
            .filter(qf.not_(qf.one_of('generic_scenario', get_basket_scenarios_ban_set(apps)))) \
            .project(ne.all(), toloka_intent=ne.const('other'), request_source=ne.const('logs')) \
            .join(intents_sample_with_session_ids, by='uuid', type='left_only',
                  assume_small_right=True, assume_unique_right=True) \
            .unique('uuid') \
            .random(count=new_ue2e_scenarios_sample_size) \
            .checkpoint(random_part + '_new_scenarios_sample')

        reqids_sample = job.concat(intents_sample_with_session_ids, new_scenarios_sample) \
            .project(
                'session_id', 'toloka_intent', 'request_source', 'uuid',
                main_req_id='req_id',
                main_session_sequence='session_sequence',
                main_ts='client_time'
            )

    elif generic_scenarios:

        flattened_sessions = prepare_flattened_sessions(expboxes_stream, apps)

        reqids_sample = flattened_sessions \
            .filter(qf.one_of('generic_scenario', generic_scenarios)) \
            .project('session_id', 'uuid', main_req_id='req_id',
                     main_session_sequence='session_sequence', main_ts='client_time',
                     toloka_intent=ne.const(None).with_type(qt.Optional[qt.String]),)

        if new_iot_users:
            vins_logs_filtered = job.table("//home/voice/vins/logs/dialogs/@date", ignore_missing=True) \
                .project(
                    'analytics_info',
                    fielddate=ne.custom(lambda x: common.get_date_from_ts(x / 1000), 'server_time_ms').with_type(qt.Optional[qt.String]),
                    req_id_vins=ne.custom(lambda request: request.get('request_id'), 'request').with_type(qt.Optional[qt.String]),
                ) \
                .filter(
                    nf.custom(common.is_new_iot_user, 'analytics_info', 'fielddate')
                ) \
                .project('req_id_vins')

            reqids_sample = reqids_sample.join(vins_logs_filtered, by_left='main_req_id', by_right='req_id_vins')

        if do_sample:
            reqids_sample = reqids_sample.random(count=2*size)

        if unique_uuid:
            reqids_sample = reqids_sample.unique('uuid')

        if do_sample:
            reqids_sample = reqids_sample.random(count=size)

    elif preselected_basket:
        """эта опция позволяет подать на вход готовую выборку запросов с полями
        'session_id', 'uuid', 'req_id', 'session_sequence' ,'client_time'
        """
        reqids_sample = job.table(preselected_basket) \
            .project('session_id', 'uuid', main_req_id='req_id',
                     main_session_sequence='session_sequence', main_ts='client_time',
                     toloka_intent=ne.const(None).with_type(qt.Optional[qt.String]), )

        if do_sample:
            reqids_sample = reqids_sample.random(count=2*size)

        if unique_uuid:
            reqids_sample = reqids_sample.unique('uuid')

        if do_sample:
            reqids_sample = reqids_sample.random(count=size)

        basket_uuids = job.table(preselected_basket) \
            .project('uuid').unique('uuid').sort('uuid')

        expboxes_stream = expboxes_stream \
            .join(basket_uuids, by='uuid',
                  assume_unique_right=True, assume_small_right=True,)

        flattened_sessions = prepare_flattened_sessions(expboxes_stream, apps)

    else:
        raise ValueError("Please define generic_scenarios or directory with toloka intents tables")

    reqids_sample.checkpoint(random_part + '_reqids_sample')

    requests_with_context = flattened_sessions \
        .join(reqids_sample, by=('uuid', 'session_id')) \
        .checkpoint(random_part + '_before_get_context') \
        .cast(server_time_ms=qt.Optional[qt.UInt64]) \
        .groupby('session_id', 'main_session_sequence') \
        .reduce(partial(get_context, timedelta=vicinity_requests_timedelta)) \
        .checkpoint(random_part + '_after_get_context') \
        .project(ne.all(exclude=('input_type',)),
                 input_type=ne.custom(common.fix_click_input_type, 'input_type', 'query')
                              .with_type(qt.Optional[qt.String])
                 )

    requests_with_context.put(output_table)

    with job.driver.transaction():
        job.run(store_checkpoints=checkpoints_path is not None)

    return {"cluster": "hahn", "table": output_table}


if __name__ == '__main__':
    call_as_operation(main)
