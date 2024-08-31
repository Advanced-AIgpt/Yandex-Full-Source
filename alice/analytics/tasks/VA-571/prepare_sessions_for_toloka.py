# -*-coding: utf8 -*-
from nile.api.v1 import Record, extractors as ne, filters as nf, aggregators as na
from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
from functools import partial

from visualize_quasar_sessions import get_json_tasks, random_hash, get_path


def get_sessions(groups):
    for key, records in groups:
        log_item = {
            'session_id': key.req_id,
            'session': []
        }
        for record in records:
            rec = record.to_dict()
            rec['parse_iot'] = True
            log_item['session'].append(rec)
        log_item['session'].sort(key=lambda state: state['client_time'])
        if len(log_item['session']) < 2:
            log_item['session_id'] = 'no_session_' + log_item['session_id']
        log_item['app'] = log_item['session'][-1].get('app')
        yield Record(**log_item)


def group_sessions(groups):
    for key, records in groups:
        log_item = {
            'tasks': []
        }
        for record in records:
            log_item['tasks'].append({
                'inputValues': {
                    'input': record.to_dict()
                }
            })
        log_item['tasks'].sort(key=lambda state: state['inputValues']['input']['client_time'])

        for i, rec in enumerate(log_item['tasks']):
            if i != 0:
                log_item['tasks'][i]['inputValues']['input']['action0'] = \
                    log_item['tasks'][i - 1]['inputValues']['input']['action1']

        yield Record(**log_item)


def get_asr_hypos(asr, add_asr_hypos):
    if not asr or not add_asr_hypos or not asr.get('data'):
        return None
    hypos = list()
    recognition = asr['data'].get('hypotheses', [])
    for ind, el in enumerate(recognition):
        if ind < 10:
            current_hypo = " ".join(el['words'])
            hypos.append(str(ind) + ". " + current_hypo)
    return "\n".join(hypos)


def main(start_date, end_date, session_ids=None, sessions=None, pool=None, tmp_path=None, output_table=None, max_session_length=100,
         remove_strategy='on_job_finish', user_logs_policy=True, add_asr_hypos=False):
    date_str = '{' + start_date + '..' + end_date + '}'
    templates = dict(job_root='//home/voice/robot-voice-qa/tmp', date=date_str)
    if tmp_path is not None:
        templates['tmp_files'] = tmp_path

    cluster = hahn_with_deps(pool=pool,
                             templates=templates,
                             neighbours_for=__file__,
                             neighbour_names=['visualize_quasar_sessions.py', 'generic_scenario_to_human_readable.py',
                                              'intents_to_human_readable.py', 'standardize_answer.py'])
    io_option = {"table_writer": {"max_row_weight": 134217728}}
    job = cluster.job().env(yt_spec_defaults={"job_io": io_option, "partition_job_io": io_option,
                                              "merge_job_io": io_option, "map_job_io": io_option,
                                              "reduce_job_io": io_option, "sort_job_io": io_option},
                            remove_strategy=dict(remote_tmp_tables=remove_strategy)
                            )

    output_table = output_table or "//tmp/robot-voice-qa/quasar_sessions_tasks_" + random_hash()

    if session_ids is not None:
        flattened_sessions = job.table('//home/alice/dialog/prepared_logs_expboxes/@date') \
            .project(ne.all(exclude='analytics_info'))
        session_ids_project = job.table(session_ids).project('session_id')
        requests_in_sessions = flattened_sessions \
            .join(session_ids_project, by='session_id')
    elif sessions is not None:
        # table with selected sessions where each row is a single request
        requests_in_sessions = job.table(sessions).project(ne.all(exclude=['analytics_info']))
    else:
        raise ValueError('no sessions or session_ids provided')

    wonder_logs = job.table('//home/alice/wonder/logs/@date', ignore_missing=True) \
        .filter(
            nf.not_(nf.equals('speechkit_response', None)),
            nf.custom(lambda spotter: spotter is None or (spotter and spotter.get('false_activation') is not True), 'spotter'),
            nf.custom(lambda asr: asr is None or (asr and asr.get('trash_or_empty') is not True), 'asr')
        ) \
        .project(
            req_id='_megamind_request_id',
            asr_hypos=ne.custom(partial(get_asr_hypos, add_asr_hypos=add_asr_hypos), 'asr'),
            #device_state=ne.custom(lambda request: request.get('request', {}).get('device_state') or {}, 'speechkit_request'), # нужен импорт из протобуфа
            #directives=ne.custom(lambda response: response.get('response', {}).get('directives'), 'speechkit_response'),
            mds_key=ne.custom(lambda asr: '/'.join(get_path(asr, ['voice_by_uniproxy', 'mds'], '').split('/')[-2:]) if get_path(asr, ['voice_by_uniproxy', 'mds']) else None),
            voice_url=ne.custom(lambda mds_key: 'https://speechbase-yt.voicetech.yandex.net/getaudio/{}?norm=1'.format(mds_key) if mds_key else None, 'mds_key')
        ) \
        .unique('req_id')

    vins_logs = job.table("//home/voice/vins/logs/dialogs/@date", ignore_missing=True) \
        .project(
            'analytics_info',
            req_id=ne.custom(lambda request: request.get('request_id'), 'request'),
            directives=ne.custom(lambda response: response.get('directives'), 'response'),
            device_state=ne.custom(lambda request: request.get('device_state') or {}, 'request'),
        ) \
        .filter(nf.not_(nf.equals('req_id', None)))


    requests = requests_in_sessions \
        .join(wonder_logs, by='req_id') \
        .join(vins_logs, by='req_id') \
        .unique('req_id')

    small_sessions = requests \
        .groupby('session_id') \
        .aggregate(cnt=na.count()) \
        .filter(nf.custom(lambda cnt: cnt <= max_session_length, 'cnt')) \
        .project('session_id')

    req_sessions = requests.join(small_sessions, by='session_id')

    tasks_table, _ = req_sessions \
        .groupby("req_id") \
        .reduce(get_sessions) \
        .map(partial(get_json_tasks, user_logs_policy=user_logs_policy))

    tasks_table \
        .join(req_sessions.project('req_id', 'session_id', 'client_time'), by='req_id') \
        .groupby('session_id') \
        .reduce(group_sessions) \
        .put(output_table)

    job.run()
    return {"cluster": "hahn", "table": output_table}


if __name__ == '__main__':
    call_as_operation(main)
