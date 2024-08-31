# -*-coding: utf8 -*-

from os import path
from copy import deepcopy
from collections import OrderedDict
from nile.api.v1 import (
    Record,
    with_hints,
    extended_schema,
    filters as nf,
    extractors as ne,
)
from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
from utils.yt.listing import run_for_date_range
from qb2.api.v1 import typing as qt

SCHEMA = OrderedDict([
    ("other", qt.Optional[qt.Json]),
    ("uuid", qt.Optional[qt.String]),
    ("icookie", qt.Optional[qt.String]),
    ("puid", qt.Optional[qt.String]),
    ("user_id", qt.Optional[qt.String]),
    ("cohort", qt.Optional[qt.String]),
    ("is_new", qt.Optional[qt.String]),
    ("first_day", qt.Optional[qt.String]),
    ("app", qt.Optional[qt.String]),
    ("platform", qt.Optional[qt.String]),
    ("version", qt.Optional[qt.String]),
    ("country_id", qt.Optional[qt.Int64]),
    ("lang", qt.Optional[qt.String]),
    ("is_exp_changed", qt.Optional[qt.Bool]),
    ("session_id", qt.Optional[qt.String]),
    ("session_sequence", qt.Optional[qt.Int64]),
    ("req_id", qt.Optional[qt.String]),
    ("fielddate", qt.Optional[qt.String]),
    ("testids", qt.Optional[qt.Json]),
    ("expboxes", qt.Optional[qt.String]),
    ("query", qt.Optional[qt.String]),
    ("reply", qt.Optional[qt.String]),
    ("intent", qt.Optional[qt.String]),
    ("generic_scenario", qt.Optional[qt.String]),
    ("mm_scenario", qt.Optional[qt.String]),
    ("skill_id", qt.Optional[qt.String]),
    ("input_type", qt.Optional[qt.String]),
    ("client_time", qt.Optional[qt.Int64]),
    ("server_time_ms", qt.Optional[qt.Int64]),
    ("alice_speech_end_ms", qt.Optional[qt.Int64]),
    ("is_interrupted", qt.Optional[qt.Bool]),
    ("child_confidence", qt.Optional[qt.Float]),
    ("do_not_use_user_logs", qt.Optional[qt.Bool]),
    ("device_id", qt.Optional[qt.String]),
    ("device_revision", qt.Optional[qt.String]),
    ("device", qt.Optional[qt.String]),
    ("is_tv_plugged_in", qt.Optional[qt.Bool]),
    ("screen", qt.Optional[qt.String]),
    ("sound_level", qt.Optional[qt.Float]),
    ("sound_muted", qt.Optional[qt.Bool]),
    ("analytics_info", qt.Optional[qt.Json]),
    ("music_answer_type", qt.Optional[qt.String]),
    ("music_genre", qt.Optional[qt.String]),
    ("voice_text", qt.Optional[qt.String]),
    ("is_smart_home_user", qt.Optional[qt.Bool]),
    ("client_tz", qt.Optional[qt.String]),
    ("location", qt.Optional[qt.Json]),
    ("subscription", qt.Optional[qt.String]),
    ("parent_req_id", qt.Optional[qt.String]),
    ("parent_scenario", qt.Optional[qt.String]),
    ("request_act_type", qt.Optional[qt.String]),
    ("trash_or_empty_request", qt.Optional[qt.Bool]),
    ("message_id", qt.Optional[qt.String]),
    ("enrollment_headers", qt.Optional[qt.Json]),
    ("guest_data", qt.Optional[qt.Json]),
])

PERSESSION_PARAMS = [
    'fielddate', 'app', 'platform', 'version', 'uuid', 'icookie', 'puid', 'user_id', 'lang',
    'cohort', 'country_id', 'is_new', 'first_day', 'session_id', 'is_exp_changed',
    'device_id', 'device_revision', 'device', 'subscription'
]
PERQUERY_PARAMS = {
    '_query', '_reply', 'type', 'intent', 'mm_scenario', 'generic_scenario', 'ts', 'child_confidence',
    'skill_id', 'req_id', 'test_ids', 'expboxes', 'ms_server_time', 'alice_speech_end_ms', 'is_interrupted',
    'do_not_use_user_logs', 'is_tv_plugged_in', 'screen', 'analytics_info', 'music_answer_type', 'music_genre',
    'voice_text', 'is_smart_home_user', 'client_tz', 'location', 'sound_level', 'sound_muted', 'parent_req_id',
    'parent_scenario', 'request_act_type', 'trash_or_empty_request', 'message_id', 'enrollment_headers',
    'guest_data'
}
PARAM_MAP = {
    '_query': 'query',
    '_reply': 'reply',
    'type': 'input_type',
    'ts': 'client_time',
    'ms_server_time': 'server_time_ms',
    'test_ids': 'testids'
}

# даты, в которые сломан client_time: https://st.yandex-team.ru/ALICE-3260#5dc9896529af1d001e04615d
BROKEN_CLIENTTIME_DAYS = ["2019-07-02", "2019-07-03", "2019-07-04", "2019-07-05"]


def map_events(records):
    for rec in records:
        base_session_params = {}
        for param in PERSESSION_PARAMS:
            base_session_params[param] = rec[param]

        for idx, item in enumerate(rec['session']):
            new_record = deepcopy(base_session_params)

            # скопировать важные параметры PERQUERY_PARAMS из каждого запроса в session
            for param_ in PERQUERY_PARAMS:
                param_name = PARAM_MAP.get(param_, param_)
                param_value = item.get(param_)
                if rec['fielddate'] in BROKEN_CLIENTTIME_DAYS and param_ == 'ts':
                    param_value = item["ms_server_time"] / 1000

                new_record[param_name] = param_value

            # неважные параметры переносятся в контейнер `other`
            other = {}
            for param in item:
                if param not in PERQUERY_PARAMS:
                    other[param] = item[param]

            # skill recommendation cards
            for card in other.get('cards', []):
                if card.get('card_id') == "skill_recommendation" and card.get('actions'):
                    other['card_type'] = 'skill_recommendation'
                    for slot in rec.get('analytics_info', {}).get('analytics_info', {}).get('semantic_frame', {}) \
                        .get('slots', []):
                        if slot['name'] == "card_name":
                            other['card_name'] = slot['typed_value']['string']
                    if other.get('card_name') is None:
                        other['card_name'] = card.get('actions')[0].split('__')[1]
                elif card.get('card_id') == "relevant_skills" and card.get('actions'):
                    other['card_type'] = 'skill_discovery'
                    other['card_name'] = 'skill_discovery'

            # click on recommendation card
            if 'callback' in other and 'action_name' in other['callback'] \
                and 'req_id' in other['callback'] and isinstance(other['callback']['req_id'], str):
                new_record['click_parent_req_id'] = other['callback']['req_id']
                new_record['activated_action'] = other['callback']['action_name']
            else:
                new_record['click_parent_req_id'] = 'empty_req_id'
                new_record['activated_action'] = 'empty_action_name'

            new_record["other"] = other
            new_record["session_sequence"] = idx
            yield Record(**new_record)


@with_hints(output_schema=dict(
    card_req_id=qt.Optional[qt.String],
    card_action=qt.Optional[qt.String],
    card_position=qt.Int64,
    card_name=qt.Optional[qt.String],
    card_type=qt.Optional[qt.String],
))
def map_cards(records):
    for rec in records:
        for card in rec['other'].get('cards', []):
            # onboarding or discovery
            if card.get('actions') and card.get('card_id') in ["skill_recommendation", "relevant_skills"]:
                for i, action in enumerate(card.get('actions')):
                    yield Record(
                        card_req_id=rec['req_id'],
                        card_action=action,
                        card_position=i + 1,
                        card_name=rec['other'].get('card_name'),
                        card_type=rec['other'].get('card_type'),
                    )


def enrich_callback(other, card_position, card_name, card_type):
    if not card_name and other.get('callback').get('action_name'):
        action_name_splitted = other['callback']['action_name'].split('__')
        if len(action_name_splitted) > 1:
            card_name = action_name_splitted[1]
    card_position = card_position or other.get('callback').get('item_number')
    card_type = card_type or other.get('callback').get('card_id')
    other['callback']['card_position'] = card_position
    other['callback']['card_name'] = card_name
    other['callback']['card_type'] = card_type
    return other


def get_events_schema():
    schema = {k: v for k, v in SCHEMA.items()}
    schema['click_parent_req_id'] = qt.Optional[qt.String]
    schema['activated_action'] = qt.Optional[qt.String]
    return schema


def prepare_table(date, sessions_root, expboxes_root, pool):
    cluster = hahn_with_deps(pool=pool, use_yql=True).env(
        compression_level=dict(
            final_tables='normal',
            tmp_tables='lightest',
        ),
    )
    job = cluster.job('Prepare logs expboxes ' + date).env(
        yt_auto_merge={"final_tables": "disabled", "tmp_tables": "disabled"},
        merge_strategy={"final_tables": "never", "tmp_tables": "never"})

    session_events = job.table(path.join(sessions_root, date)) \
        .map(with_hints(output_schema=extended_schema(**get_events_schema()))(map_events))

    card_actions = session_events \
        .map(map_cards)

    clicks, no_clicks = session_events \
        .split(nf.equals('click_parent_req_id', "empty_req_id"))

    # joining clicks to shows and merging with records without clicks
    clicks \
        .join(
            card_actions,
            type='left',
            by_left=('click_parent_req_id', 'activated_action'),
            by_right=('card_req_id', 'card_action'),
            assume_unique_right=True
        ) \
        .project(
            ne.all(),
            other=ne.custom(enrich_callback, 'other', 'card_position', 'card_name', 'card_type') \
                .with_type(qt.Optional[qt.Json])
        ) \
        .concat(no_clicks) \
        .project(*SCHEMA.keys()) \
        .sort('uuid', 'device_id', 'puid', 'session_id', 'session_sequence') \
        .put(path.join(expboxes_root, date), schema=SCHEMA)

    job.run()


def main(date,
         sessions_root='//home/voice/dialog/sessions',
         expboxes_root='//home/alice/dialog/prepared_logs_expboxes',
         pool='voice'
         ):
    if isinstance(date, (list, tuple)):
        start_date, end_date = date
        make_by_date_range(start_date, end_date, sessions_root, expboxes_root=expboxes_root, pool=pool)
        return {'expboxes_first_path': path.join(expboxes_root, start_date),
                'expboxes_last_path': path.join(expboxes_root, end_date)}

    prepare_table(date, sessions_root, expboxes_root, pool)
    return {'expboxes_path': path.join(expboxes_root, date)}


def make_by_date_range(from_date, to_date, sessions_root, **kwargs):
    # Перегенерация таблиц сессий по указанным датам
    run_for_date_range(from_date, to_date, main, sessions_root, **kwargs)


if __name__ == '__main__':
    call_as_operation(main)
