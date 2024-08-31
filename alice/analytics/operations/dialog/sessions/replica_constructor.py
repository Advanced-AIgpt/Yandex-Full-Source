#!/usr/bin/env python
# encoding: utf-8
"""
Формирование словарей, соответствующих отдельным репликам и встраивание их в список session
"""
from collections import OrderedDict

from nile.api.v1 import (
    Record,
    modified_schema,
    with_hints,
)
from qb2.api.v1 import typing as qt

from intent_scenario_mapping import get_generic_scenario, get_pa_intent, MAPPED_SKILLS_SCENARIOS

REPLICA_SCHEMA = OrderedDict([
    ('cohort', qt.Optional[qt.String]),
])

# fielddate, testids, etc. are needed for calculation per session params
# but not for per query params that will be in the 'session' field.
# callback_name is needed for dividing session by 'on_reset_session' callback
PER_SESSION_PARAMS = ['lang', 'fielddate', 'uuid', 'app', 'country_id', 'testids', 'icookie', 'platform', 'puid', 'version', 'device', 'experiments',
                      'device_id', 'device_revision', 'user_id_from_cookies', 'callback_name', 'subscription', 'enviroment_state']


def make_replica(record):
    """
    :param dict record: Запись в виде словаря полученного из yt Record
    :rtype: dict
    :return: Словарь со свойствами реплики для помещения в список session
    """
    replica = {
        '_query': record.get('utterance_text') or get_original_utterance(record['analytics_info']),
        '_reply': record['reply'],
        'req_id': record['req_id'],
        'callback': get_callback(record),
        'intent': record['intent'],
        'mm_scenario': record['mm_scenario'],
        'ts': record['client_time'],
        'ms_server_time': record['ms_server_time'],
        'suggests': record['suggests'],
        'cards': record['cards'],
        'test_ids': record['testids'],
        'expboxes': record['expboxes'],
        'do_not_use_user_logs': record['do_not_use_user_logs'],
        'is_tv_plugged_in': record['is_tv_plugged_in'],
        'music_answer_type': record['music_answer_type'],
        'music_genre': record['music_genre'],
        'screen': record['screen'],
        'sound_level': record['sound_level'],
        'sound_muted': record['sound_muted'],
        'analytics_info': record['analytics_info'],
        'alice_speech_end_ms': record['alice_speech_end_ms'],
        'is_interrupted': record['is_interrupted'],
        'child_confidence': record['child_confidence'],
        'voice_text': record['voice_text'],
        'is_smart_home_user': record['is_smart_home_user'],
        'client_tz': record['client_tz'],
        'location': record['location'],
        'parent_req_id': record['parent_req_id'],
        'parent_scenario': record['parent_scenario'],
        'type': record['type'],
        'request_act_type': record['request_act_type'],
        'trash_or_empty_request': record['trash_or_empty_request'],
        'message_id': record['message_id'],
        'enrollment_headers': record['enrollment_headers'],
        'guest_data': record['guest_data'],
    }

    # если в реплике только callback, то необходимо выставить его интент, а не предыдующей фразы
    if replica['_query'] is None and replica['_reply'] == "EMPTY" \
            and replica['callback'] and replica['callback'].get('intent_name', None) is not None:
        if replica['callback']['intent_name'] == "personal_assistant.scenarios.skill_recommendation":
            if not replica['intent'].startswith("personal_assistant\tscenarios\tskill_recommendation"):
                splitted_action_name = replica['callback']['action_name'].split('__')
                if len(splitted_action_name) > 1:
                    action_name = splitted_action_name[1].split('#')[0].replace("_static", "")
                else:
                    action_name = replica['callback']['action_name']
                if action_name not in ("get_greetings", "games_onboarding", "first_session_onboarding"):
                    action_name = "onboarding"
                replica['intent'] = "personal_assistant\tscenarios\tskill_recommendation\t" + action_name
        elif replica['callback']['intent_name'] != "skill_discovery_ru":
            replica['intent'] = replica['callback']['intent_name'].replace('.', '\t').replace('__', '\t')

    replica['generic_scenario'] = get_generic_scenario(replica['intent'],
                                                       replica['mm_scenario'],
                                                       record['product_scenario_name'],
                                                       replica['music_genre'],
                                                       record.get('skill_id'),
                                                       replica['trash_or_empty_request'],
                                                       replica['music_answer_type'],
                                                       record['filters_genre'])
    replica['intent'] = get_pa_intent(replica['intent'])

    if record['form_changed']:
        replica['restored'] = record['restored_intent']

    if record.get('directives'):
        replica['directives'] = record['directives']

    if replica['generic_scenario'].lower() == 'dialogovo' or replica['generic_scenario'] in MAPPED_SKILLS_SCENARIOS:
        replica['skill_id'] = record['skill_id']
        replica['skill_info'] = record['skill_info']

    if record['external_session_id_seq'] and isinstance(record['external_session_id_seq'], dict):
        replica['external_session_id'] = record['external_session_id_seq'].get('id')
        replica['external_session_seq'] = record['external_session_id_seq'].get('seq')

    if record['error_type']:
        replica['error_type'] = record['error_type']

    if 'external_skill_gc' in record['intent'] and not record['intent'].endswith('deactivate'):
        replica['gc_intent'] = record.get('gc_intent', None)

    if 'external_skill_gc' in record['intent'] or 'general_conversation' in record['intent']:
        replica['gc_source'] = record.get('gc_source', None)

    return replica


@with_hints(output_schema=modified_schema(
    exclude=['app_id', 'callback_args', 'form', 'form_changed', 'form_name', 'product_scenario_name', 'response',
             'utterance_source', 'utterance_text'],
    extend={'external_session_id': qt.Optional[qt.String], 'generic_scenario': qt.Optional[qt.String],
            '_query': qt.Optional[qt.String], 'external_session_seq': qt.Optional[qt.Int64],
            'test_ids': qt.Optional[qt.List[qt.String]], 'callback': qt.Optional[qt.Json]},
    rename={'ts': 'client_time', 'restored': 'restored_intent', '_reply': 'reply'}))
def merge_callbacks(groups):
    for key, records in groups:
        postclicked = None  # Нереализованный кандидат на мёрдж
        prev_replica = None
        for record in records:
            replica = {key: record[key] for key in PER_SESSION_PARAMS}
            replica.update(make_replica(record))
            new_postclicked = None

            if is_postclick(replica):
                if prev_replica and is_clicks_connected(prev_replica, replica):
                    merge_clicks(prev_replica, replica)
                else:
                    new_postclicked = replica
                    if prev_replica:
                        yield Record(**prev_replica)
                    prev_replica = replica
            elif postclicked and is_clicks_connected(replica, postclicked):
                # Случай, когда события записаны в лог в обратном порядке
                merge_clicks(replica, postclicked)
                prev_replica = replica
            elif prev_replica and is_broken(prev_replica, replica):
                merge_clicks_broken(prev_replica, replica)
            else:
                if prev_replica:
                    yield Record(**prev_replica)
                prev_replica = replica
            postclicked = new_postclicked
        if prev_replica:
            yield Record(**prev_replica)


def get_original_utterance(analytics_info):
    if analytics_info:
        return analytics_info.get('original_utterance')
    return None


def get_callback(record):
    name = record['callback_name']
    if not name:
        return {}
    cb = {'name': name}
    args = record['callback_args']

    if name == 'on_reset_session':
        cb['mode'] = args.get('mode')

    elif name == 'on_suggest':
        block = args.get("suggest_block", {})
        cb['caption'] = args.get('caption')
        cb['suggest_type'] = block.get('suggest_type')
        cb['utterance'] = args.get('user_utterance') or args.get('utterance')
        cb['uri'] = args.get('uri')  # Присутствует при нажатиях на карточки

        upd = block.get('form_update')
        if upd and isinstance(upd, dict) and 'name' in upd:
            cb['form_update'] = form = {'name': upd['name']}
            if 'resubmit' in upd:
                form['resubmit'] = upd['resubmit']
            if 'slots' in upd:
                form['slots'] = {s.get('name') or s['slot']: s['value']
                                 for s in filter(lambda x: ('name' in x or 'slot' in x) and 'value' in x, upd['slots'])}

    elif name == 'update_form':
        if record['error_type'] == 'incorrect_form_update':
            return {}

        upd = args.get("form_update", {})
        cb['form_name'] = upd.get("name")

        if not isinstance(cb['form_name'], str):
            return {}

        slots = upd.get("slots", [])

        if not isinstance(slots, list):
            slots = [slots]

        cb['slots'] = {s.get('name') or s['slot']: s.get('value', '')
                       for s in slots}
        cb['resubmit'] = args.get('resubmit')

        push_id = upd.get('push_id', None)
        if push_id:
            cb['push_id'] = push_id

    elif name == 'on_card_action':
        cb['action_name'] = args.get('action_name') or args.get('case_name')
        cb['req_id'] = args.get('request_id') or args.get('@request_id')
        cb['card_id'] = args.get('card_id')
        cb['intent_name'] = args.get('intent_name')
        cb['item_number'] = args.get('item_number')
        # cb['uri'] = args.get('action_uri')

    elif name == 'on_external_button':
        cb['btn_data'] = args.get('button_data')

    # Примеры: клик по предложению discovery, карточке в ПП, навыку в каталоге, deeplink
    elif name == 'external_source_action':
        cb['source'] = args.get('source')
        cb['utm_source'] = args.get('utm_source')
        cb['utm_medium'] = args.get('utm_medium')
        cb['skill_id'] = args.get('skill_id')

    if '@request_id' in args:
        cb['@request_id'] = args['@request_id']

    return cb


# Слияние записей, относящихся к одному клику
# https://st.yandex-team.ru/VA-34

def is_postclick(replica):
    # Клик, по которому могло уже прийти действие
    if (not replica['_query'] and
        replica['_reply'] == 'EMPTY' and
        replica['type'] in ('click', 'tech') and
        not replica['cards'] and
            not replica['suggests']):

        cb = replica.get('callback', {})
        return cb.get('name') == 'on_suggest'

    return False


def is_clicks_connected(prev_replica, replica):
    # Действительно ли реплики относятся к одному и тому же клику
    # Подразумевается, что is_postclick(replica) уже дал положительный результат
    if prev_replica['type'] not in ('click', 'tech'):
        return False

    prev_cb = prev_replica.get('callback', {})
    cb = replica.get('callback', {})

    # Случай, когда клик привёл к обновлению формы
    if prev_cb.get('name') == 'update_form':
        prev_intent = prev_replica.get('restored') or prev_replica['intent']
        cur_intent = replica.get('restored') or replica['intent']
        return (cur_intent == prev_intent and
                'form_update' in cb and
                cb['form_update']['name'] == prev_cb['form_name']
                ) or (prev_cb.get('@request_id') and
                      prev_cb.get('@request_id') == cb.get('@request_id'))

    if cb.get('uri'):
        return prev_cb and cb == prev_cb  # Просто дублирующиеся записи - вторая по факту окажется выкинутой

    # Случай, когда клик привёл к отправке utterance
    caption = cb.get('caption')
    utterance = cb.get('utterance')
    # Предложение должно совпадать с предыдущим либо введённым руками, либо через клик
    # Либо в клике на саджест не содержится никаких предложений, а предложение есть только в предыдущей реплике
    if caption and (caption == prev_replica['_query'] or
                    caption == prev_cb.get('caption')):
        return True
    elif utterance and utterance == prev_replica['_query']:
        return True
    elif not caption and not utterance and prev_replica['_query']:
        return True
    elif prev_cb and cb == prev_cb:  # Просто дублирующиеся записи - вторая по факту окажется выкинутой
        return True

    return False


def merge_clicks(prev_replica, click_replica):
    if not prev_replica.get('skill_id'):
        prev_replica['skill_id'] = click_replica.get('skill_id')

    cb = click_replica['callback']

    if not prev_replica['callback']:
        prev_replica['callback'] = cb
        if (not cb.get('caption') and
            prev_replica['_query'] and
                cb['name'] == "on_suggest"):  # проверка избыточная, но оставлена на случай,
            # что мы начнём обрабатывать что-то кроме on_suggest

            # Обозначаем, что пользователь "сказал" это с помощью саджеста:
            cb['caption'] = prev_replica['_query']

    elif prev_replica['callback'].get('name') == 'update_form':
        prev_replica['callback'] = cb

    if prev_replica['type'] == 'tech':
        prev_replica['type'] = click_replica['type']


# Слияние записей, в которых запрос и ответ лежат в разных событиях
# https://st.yandex-team.ru/VA-207

def is_broken(prev_replica, replica):
    return (replica['_query'] is None and
            replica['_reply'] != 'EMPTY' and
            replica['cards'] and
            replica['callback'] and
            replica['type'] in ('click', 'tech') and
            prev_replica['_query'] is not None and
            prev_replica['_reply'] == 'EMPTY' and
            not prev_replica['cards'] and
            not prev_replica['callback'] and
            replica['intent'] == prev_replica['intent'])


def merge_clicks_broken(prev_replica, click_replica):
    if not prev_replica.get('skill_id') and click_replica.get('skill_id'):
        prev_replica['skill_id'] = click_replica.get('skill_id')
    if set(prev_replica['suggests']).issubset(set(click_replica['suggests'])):
        prev_replica['suggests'] = click_replica['suggests']
    prev_replica['_reply'] = click_replica['_reply']
    prev_replica['cards'] = click_replica['cards']
    prev_replica['callback'] = click_replica['callback']
    prev_replica['ts'] = click_replica['ts']

    if prev_replica['type'] == 'tech':
        prev_replica['type'] = click_replica['type']
