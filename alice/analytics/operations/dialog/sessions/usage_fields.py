#!/usr/bin/env python
# encoding: utf-8
"""
Получение полей из logs/dialogs в формате пригодном для скармливания в sessions
"""
import json
from operator import itemgetter
import re
import copy

import google.protobuf.json_format

from alice.megamind.protos.analytics.megamind_analytics_info_pb2 import (
    TMegamindAnalyticsInfo
)
from alice.library.client.protos.client_info_pb2 import (
    TClientInfoProto
)

from alice.megamind.protos.speechkit.response_pb2 import (
    TSpeechKitResponseProto
)

from alice.wonderlogs.sdk.python.getters import (
    get_intent as get_intent_sdk,
    get_product_scenario_name as get_product_scenario_name_sdk,
    get_music_answer_type as get_music_answer_type_sdk,
    get_music_genre as get_music_genre_sdk,
    get_filters_genre as get_filters_genre_sdk,
    smart_home_user as smart_home_user_sdk,
    get_app as get_app_sdk,
    get_platform as get_platform_sdk,
    get_version as get_version_sdk,
    get_sound_level as get_sound_level_sdk,
    get_path as get_path_sdk,
    form_changed as form_changed_sdk,
    get_slots as get_slots_sdk,
    parse_cards as parse_cards_sdk
)


def get_dict_path(data, path, default=None):
    try:
        for item in path:
            data = data[item]
        return data
    except (KeyError, TypeError, IndexError):
        return default


def is_correct_record(form_name, app_id, mm_scenario):
    # убрать сценарии здесь, когда убедимся, что можно не проверять на пустой form_name
    if form_name is None and mm_scenario not in ('Miles', 'MailCiao'):
        return 0
    elif app_id not in ['pa', 'stroka']:
        return 0
    elif form_name == 'stroka.stroka.dont_understand':
        return 0
    else:
        return 1


def get_country_id(request):
    if not request.get('laas_region'):
        return -1
    return request['laas_region'].get('country_id_by_ip') or -1


def get_request_id(request):
    return request['request_id']


def get_icookie(request):
    return get_dict_path(request, ['additional_options', 'icookie'])


def get_voice_text(response):
    if response and 'voice_text' in response:
        return response['voice_text']
    return None


def get_testids(request):
    return map(str, request.get("test_ids", []))


def get_expboxes(request):
    if request and request.get('additional_options'):
        return request['additional_options'].get('expboxes')
    return None


def get_enviroment_state(request):
    return (request or {}).get('enviroment_state')


def restore_form_name(analytics_info_proto, callback_args_json, callback_args):
    try:
        path = None
        # запрос на обновление формы, не гарантирует, что сработает именно этот интент
        if 'form_update' in callback_args:
            path = callback_args['form_update']['name']
        if isinstance(path, str):
            return path
    except (TypeError, KeyError):
        pass  # Это норм. Просто нужных ключей нет.

    path = get_path_sdk(analytics_info_proto, callback_args_json)

    return path


def extract_intent(form_name, form, response, callback_args, cards, analytics_info, skill_info, slot_name, slots,
                   analytics_info_proto, callback_args_json):
    if form_name is None:
        return "EMPTY"

    new_intent = get_intent_sdk(analytics_info_proto)
    mm_scenario = extract_mm_scenario(analytics_info)

    if new_intent and mm_scenario not in ("alice.vins", "Vins"):
        return new_intent.replace('__', '\t').replace('.', '\t')

    path = get_path_sdk(analytics_info_proto, callback_args_json)

    return normalize_scenario_name(path, form, response, cards, skill_info, callback_args, slot_name, slots)


def get_restored_intent(form, response, callback_args, cards, skill_info, slot_name, slots,
                        analytics_info_proto, callback_args_json):
    path = restore_form_name(analytics_info_proto, callback_args_json, callback_args)
    return normalize_scenario_name(path, form, response, cards, skill_info, callback_args, slot_name, slots)


def get_slot_name(form_name):
    return 'slot' if form_name != 'Vins' else 'name'


def byteify(input):
    import six
    if isinstance(input, dict):
        return {byteify(key): byteify(value)
                for key, value in input.iteritems()}
    elif isinstance(input, list):
        return [byteify(element) for element in input]
    elif isinstance(input, six.text_type):
        return input.encode('utf-8')
    else:
        return input


def get_slot_value(form_name, slot):
    if form_name != 'Vins':
        return slot['value']

    slot_value = slot["typed_value"]["string"]

    if slot_value == 'null':
        return None

    try:
        json_value = byteify(json.loads(slot_value))
        if isinstance(json_value, dict):
            slot_value = json_value
    except:
        pass

    return slot_value


def normalize_scenario_name(path, form, response, cards, skill_info, callback_args,
                            slot_name, slots):

    if path is None:
        return 'EMPTY'

    if path.startswith("personal_assistant.scenarios.search"):
        path = path.replace('search_internet', 'search')

        # не патчим, если есть намёк на редирект в серп или на карту
        if not path.endswith("serp") or path.endswith("show_on_map"):
            # хитрая функция парсинга слотов для поиска, которая позволяет детализировать показ
            # (вот бы отказаться от неё в пользу нормальных product_scenario_name!)
            for slot in slots:
                if slot[slot_name] == "nav_url" and get_slot_value(form['form'], slot):
                    path = path.replace('scenarios.search',
                                        'scenarios.search_nav_url')
                elif slot[slot_name] == "search_results" and get_slot_value(form['form'], slot):
                    postfix = "serp"
                    for k in get_slot_value(form['form'], slot).keys():
                        if k != 'serp':
                            if k == 'nav':
                                k = 'nav_url'
                            postfix = k
                    if postfix:
                        if not response.get('directives') and \
                                len(cards) > 1 and cards[1].get('card_id') == "relevant_skills":
                            path = path.replace('scenarios.search',
                                                'scenarios.search__relevant_skills')
                        else:
                            path = path.replace('scenarios.search',
                                                'scenarios.search__' + postfix)
                        break
    elif 'external_skill' in path:
        if get_skill_id(skill_info) == "bd7c3799-5947-41d0-b3d3-4a35de977111":
            path = path.replace('external_skill', 'external_skill_gc')

    elif path.startswith("personal_assistant.scenarios.market") and "scenarios.market_beru" not in path:
        for slot in slots:
            if slot[slot_name] == "choice_market_type" and get_slot_value(form['form'], slot) in ('BLUE', '\"BLUE\"'):
                path = path.replace('scenarios.market',
                                    'scenarios.market_beru')

        # VA-1092
        if "scenarios.market_beru" not in path and \
           ((callback_args.get('suggest_block', {}) or {})
                          .get('form_update', {}) or {}).get('name', '').endswith('.market__market') and \
           any(slot.get('name') == 'choice_market_type' and slot.get('value') in ('BLUE', '\"BLUE\"')
               for slot in callback_args['suggest_block']['form_update'].get('slots', [])):
            path = path.replace('scenarios.market',
                                'scenarios.market_beru')

    # VA-466
    elif path == "personal_assistant.scenarios.skill_recommendation":
        log_id_type = None
        for card in response['cards']:
            if card.get('type') == 'div_card':
                for _, _, action_name in _parse_el(card['body'], True):
                    if action_name != "skill_recommendation__all_skills":
                        log_id_type = action_name.split('__')[1]
                        break
        if log_id_type:
            path = path + '.' + log_id_type
        else:
            if form is not None and 'slots' in form and form['form'] == "personal_assistant.scenarios.skill_recommendation":
                for slot in slots:
                    if slot.get('value'):
                        path = path + '.' + get_slot_value(form['form'], slot)
                        break

    # VA-553 афиши карточек организации
    elif path == "personal_assistant.scenarios.find_poi":
        has_org_card, has_events = False, False
        for card in response['cards']:
            if 'body' in card and 'vins_log_info' in card['body']:
                if card['body']['vins_log_info']['card_name'] == 'found_poi_one':
                    has_org_card = True
                if card['body']['vins_log_info']['card_name'] == 'found_poi_events':
                    has_events = True
        if has_org_card:
            path += '__org_card'
        if has_events:
            path += '__events'

    # if path.startswith('stroka.stroka'):
        # path = 'personal_assistant.' + '.'.join(path.split('.')[1:])
    return path.replace('__', '\t').replace('.', '\t')


def get_music_answer_type_from_slots(form_name, intent, slot_name, slots):
    for slot in slots:
        if slot.get(slot_name) and slot.get(slot_name) == "answer":
            if form_name == 'Vins':
                vins_slot_value = get_slot_value(form_name, slot)
                if vins_slot_value and vins_slot_value != 'null':
                    return 'music_result'
            elif slot.get('type'):
                return slot.get('type')
            elif slot.get('value_type'):
                if isinstance(slot.get('value_type'), str):
                    return slot.get('value_type')
                elif isinstance(slot.get('value_type'), dict):
                    return slot.get('value_type').get('type')
    return None


def get_music_answer_type_from_analytics_info(analytics_info):
    return get_music_answer_type_sdk(convert_analytics_info_to_proto(analytics_info or {}))


def get_music_answer_type(analytics_info_proto, form_name, intent, slot_name, slots):
    answer_type = get_music_answer_type_sdk(analytics_info_proto)

    if answer_type is None and re.search(r'music_|morning_show|meditation|podcast', intent):
        answer_type = get_music_answer_type_from_slots(form_name, intent, slot_name, slots)

    if answer_type is not None:
        answer_type = answer_type.lower()

    return answer_type


def get_filters_genre(analytics_info):
    return get_filters_genre_sdk(convert_analytics_info_to_proto(analytics_info or {}))


def get_skill_id(skill_info):
    if skill_info:
        return skill_info.get('skill_id')
    else:
        return None


def get_skill_info(form, form_name, analytics_info, slot_name, slots):
    if form_name == 'Dialogovo':
        skill = analytics_info.get('analytics_info', {}) \
                              .get('Dialogovo', {}) \
                              .get('scenario_analytics_info', {}) \
                              .get('objects', [{}])[0] \
                              .get('skill', {})
        return dict(
            skill_id=skill.get('id'),
            name=skill.get('name'),
            developer_name=skill.get('developer_name'),
            developer_type=skill.get('developer_type'),
            category=skill.get('category'),
            voice=skill.get('voice'),
            ad_block_id=skill.get('ad_block_id')
        )

    for slot in slots:
        if slot[slot_name] == "skill_id":
            skill_id = get_slot_value(form_name, slot)
            if skill_id == 'null':
                skill_id = None
            return dict(skill_id=skill_id and skill_id.strip())

    return None


def get_user_id_from_cookies(request):
    if request.get('additional_options') and request['additional_options'].get('bass_options'):
        if request['additional_options']['bass_options'].get('cookies'):
            for cookie in request['additional_options']['bass_options']['cookies']:
                if cookie and cookie.startswith('user_id='):
                    return cookie.split('user_id=')[1]
    return ''


def get_external_session_id_seq(form, slot_name, slots):

    for slot in slots:
        if slot[slot_name] == "session":
            return get_slot_value(form['form'], slot)
    return None


def get_error(form_name, analytics_info, response):
    if form_name == 'Vins':
        for obj in analytics_info.get('analytics_info', {}
                                      ).get('Vins', {}
                                            ).get('scenario_analytics_info', {}
                                                  ).get('objects', {}):
            return obj.get('vins_error_meta', {}).get('type')
    elif response.get('meta'):
        for block in response['meta']:
            if block.get('type') == "error" and 'error_type' in block:
                return block['error_type']
    return None


def get_query(form, slot_name, slots):
    for slot in slots:
        if slot[slot_name] == "query":
            if get_slot_value(form['form'], slot) is None:
                return ''
            else:
                return get_slot_value(form['form'], slot)
    return ""


def get_url(form, slot_name, slots):
    for slot in slots:
        if slot[slot_name] == "search_results":
            url = get_slot_value(form['form'], slot)["nav"]["url"]
            text = get_slot_value(form['form'], slot)["nav"]["text"]
            if url.startswith('intent://'):
                return 'приложение ' + text
            else:
                return 'сайт ' + url
    return ""


SKILL_LIST_RESPONSES = {'Вот, что я могу:',
                        'Вот, что я умею.',
                        'Вот игры, в которые можно поиграть.'}


def get_reply(response, form, intent, slot_name, slots, analytics_info=None):
    try:
        reply = response['cards'][0]['text']
    except (IndexError, KeyError, TypeError):
        return 'EMPTY'
    if intent.endswith('serp'):
        reply += ' [ Открывается поиск по запросу <' + get_query(form, slot_name, slots) + '> ]'

    if intent.endswith('nav_url'):
        reply += ' [ Открывается ' + get_url(form, slot_name, slots) + ']'

    if reply == '...':
        for slot in slots:
            if slot[slot_name] == "search_results" and get_slot_value(form['form'], slot):
                for k in get_slot_value(form['form'], slot).keys():
                    if k in ['object', 'factoid'] and 'text' in get_slot_value(form['form'], slot)[k]:
                        reply = get_slot_value(form['form'], slot)[k]['text']

    if reply == '...' and analytics_info is not None:
        try:
            for obj in analytics_info['analytics_info']['Search']['scenario_analytics_info']['objects']:
                if obj['id'] == 'selected_fact':
                    fact = json.loads(obj['name'])
                    if 'text' in fact:
                        reply = fact['text']
        except KeyError:
            pass

    if reply == '...':
        if response['voice_text'] == '...' and response.get('voice_response', {}).get('should_listen', None) is None:
            reply = None
        else:
            reply = response['voice_text']

    if reply in SKILL_LIST_RESPONSES:
        for card in response['cards']:
            if card["type"] == 'div_card':
                if len(card['body']['states']) < 1:
                    return reply
                for block in card['body']['states'][0]['blocks']:
                    if block['type'] == 'div-universal-block':
                        reply += ('\n- ' + block['text'])
    return reply


def list_suggests(response):
    if not response or 'suggests' not in response:
        return []
    try:
        return map(itemgetter('title'), response['suggests'])  # TODO: add suggest payload?
    except KeyError:
        return []


def get_directives(response, app):
    if app in ('navigator', 'auto'):
        if response.get('directives'):
            return response['directives']
    return None


# Карточки

def list_cards(response):
    if not response or not response.get('cards'):
        return []

    serialized_cards = []
    for card in response['cards']:
        serialized_cards.append(convert_card_to_proto(card))

    return parse_cards_sdk(serialized_cards)


def _parse_el(element, only_log_id=False):
    if isinstance(element, dict):
        for key, val in element.iteritems():
            if key == "action" and val:
                yield _parse_action(val, only_log_id)
            else:
                for act in _parse_el(val, only_log_id):
                    yield act
    elif isinstance(element, list):
        for sub_el in element:
            for act in _parse_el(sub_el, only_log_id):
                yield act
    else:
        pass


def _parse_action(action, only_log_id=False):
    import urlparse
    from urllib import quote
    url = action.get('url') or ''
    # temporary hack for VA-1108
    if ';' in url or '@' in url or '+%5C+' in url:
        url = url.replace(';', quote(';')).replace('@', quote('@')).replace('+%5C+', quote('+%5C+'))

    parts = urlparse.urlsplit(url)
    if parts.path.startswith('?'):
        # Некоторые версии urlparse не допускают пустого пути, а в экшенах он повсеместен
        query = urlparse.parse_qs(parts.path[1:])
    else:
        query = urlparse.parse_qs(parts.query)
    if only_log_id or 'directives' not in query:
        return None, None, action.get('log_id')

    # temporary hack for VA-919, just to fix sessions-creation while they fix the bug: BROWSER-104685
    try:
        directives = json.loads(query['directives'][0])
        for d in directives:
            if d.get('name') == 'on_card_action':
                pl = d['payload']
                return (pl['card_id'],
                        pl['intent_name'],
                        pl.get('action_name') or pl.get('case_name'))
    except ValueError:
        if "yabro-action" in url and "open_recomendaited_video" in url:
            return None, None, None
        else:
            raise
    return None, None, None  # Так и не встретился on_card_action


def _get_gc_protocol_analytics_info(analytics_info):
    gc_info = get_dict_path(analytics_info, ['analytics_info', 'GeneralConversation', 'scenario_analytics_info', 'objects'], None)
    if gc_info:
        return gc_info
    return get_dict_path(analytics_info, ['analytics_info', 'GeneralConversationHeavy', 'scenario_analytics_info', 'objects'], [])


def get_gc_intent(intent, form_name, response, analytics_info, analytics_info_proto):
    if 'external_skill_gc' in intent:
        gc_intent = None
        try:
            for obj in _get_gc_protocol_analytics_info(analytics_info):
                info = obj.get('gc_response_info', None)
                if info:
                    gc_intent = info['gc_intent']
        except KeyError:
            pass
        if gc_intent is None:
            for meta in response.get('meta', []):
                if meta.get('type') == 'form_restored':
                    gc_intent = meta['overriden_form']
                    break
            else:
                gc_intent = form_name if form_name != 'Vins' else get_intent_sdk(analytics_info_proto)
        if gc_intent:
            return gc_intent.replace('__', '\t').replace('.', '\t')


def get_gc_source(intent, response, analytics_info):
    try:
        for obj in _get_gc_protocol_analytics_info(analytics_info):
            info = obj.get('gc_response_info', None)
            if info:
                return info['source']
    except KeyError:
        pass

    try:
        for obj in analytics_info['analytics_info']['Vins']['scenario_analytics_info']['objects']:
            info = obj.get('vins_gc_meta', None)
            if info:
                return info['source']
    except KeyError:
        pass
    if response and response.get('meta'):
        if intent.startswith("personal_assistant\tgeneral_conversation\tgeneral_conversation") \
                or intent.startswith("personal_assistant\tscenarios\texternal_skill_gc"):
            for item in response['meta']:
                if item.get('type') == 'gc_source':
                    return item['source']


def get_ms_server_time(server_time_ms, server_time):
    if server_time_ms:
        return int(server_time_ms)
    return 1000 * int(server_time)


def get_device_id(request, app, device_id):
    if app in ["quasar", "small_smart_speakers", "tv"] and request["device_state"]:
        return request["device_state"].get("device_id")
    return device_id


def get_device(request):
    device_manufacturer = request["app_info"].get("device_manufacturer", "")
    if device_manufacturer is None:
        device_manufacturer = ""
    device_model = request["app_info"].get("device_model", "")
    if device_model is None:
        device_model = ""
    return " ".join([device_manufacturer, device_model])


def get_tv_state(request):
    tv_state = False
    if request.get("device_state") and request["device_state"].get("is_tv_plugged_in"):
        tv_state = True
    return tv_state


def get_screen(request):
    screen = get_dict_path(request, ['device_state', 'video', 'current_screen'])
    webview_screen = get_dict_path(request, ['device_state', 'video', 'view_state', 'currentScreen'])
    if screen and webview_screen:
        screen += '.{}'.format(webview_screen)
    return screen


def get_sound_level(request):
    return get_sound_level_sdk(get_dict_path(request, ['device_state', 'sound_level']), get_dict_path(request, ['app_info', 'app_id']))


def get_sound_muted(request):
    return get_dict_path(request, ['device_state', 'sound_muted'])


def get_subscription(analytics_info):
    subscription = None
    if get_dict_path(analytics_info, ['user_profile', 'has_yandex_plus']):
        subscription = 'plus'
    return subscription


def extract_mm_scenario(analytics_info):
    if analytics_info and analytics_info.get("analytics_info"):
        # g-kostin@ говорит, что не бывает 2 сценариев и в этом поле всегда лежит название сценария,
        # а вот scenario_analytics_info может и не быть, т.к. поле не обязательное
        info = analytics_info.get("analytics_info")
        if info:
            return list(info.keys())[0]
    return None


def patch_analytics_info(analytics_info_):
    analytics_info = copy.copy(analytics_info_)
    if analytics_info:
        analytics_info.pop("tunneller_raw_responses", None)
        # remove iot_user_info except platform of device
        devices = []
        for device in get_dict_path(analytics_info, ['iot_user_info', 'devices'], []):
            platform = get_dict_path(device, ['quasar_info', 'platform'])
            if platform:
                devices.append({'quasar_info': {'platform': platform}})

        analytics_info['iot_user_info'] = {'devices' : devices}
    return analytics_info


def get_alice_speech_end_ms(request_stat, server_time_ms):
    # calculation details: VA-1396, VA-1725
    result = None
    timestamps = request_stat.get('timestamps', {})
    if 'onSoundPlayerEndTime' in timestamps and 'onVinsResponseTime' in timestamps:
        result = server_time_ms - int(timestamps['onVinsResponseTime']) + int(timestamps['onSoundPlayerEndTime'])
    elif 'onInterruptionPhraseSpottedTime' in timestamps and 'onVinsResponseTime' in timestamps:
        result = server_time_ms - int(timestamps['onVinsResponseTime']) + int(timestamps['onInterruptionPhraseSpottedTime'])

    if result is not None and result < 0:
        result = 0
    return result


def is_interrupted(request_stat):
    # calculation details: VA-1725
    timestamps = request_stat.get('timestamps', {})
    if 'onSoundPlayerEndTime' in timestamps:
        return False
    if 'onInterruptionPhraseSpottedTime' in timestamps:
        return True
    return None


def get_child_confidence(biometry_classification):
    # VA-1429
    for obj in biometry_classification.get('scores', []):
        if obj.get('classname') == 'child':
            return obj['confidence']


def convert_analytics_info_to_proto(analytics_info):
    return google.protobuf.json_format.ParseDict(analytics_info, TMegamindAnalyticsInfo(), ignore_unknown_fields=True).SerializeToString()


def convert_app_info_to_proto(request, lang):
    app_info_copy = copy.copy(request['app_info'])
    app_info_copy['lang'] = lang
    return google.protobuf.json_format.ParseDict(app_info_copy, TClientInfoProto(), ignore_unknown_fields=True).SerializeToString()


def convert_card_to_proto(card):
    return google.protobuf.json_format.ParseDict(card, TSpeechKitResponseProto.TResponse.TCard(), ignore_unknown_fields=True).SerializeToString()


def get_analytics_info_values(analytics_info):
    if analytics_info and analytics_info.get("analytics_info"):
        info = analytics_info.get("analytics_info")
        for key, val in info.items():
            return val
    return dict()


def get_parent_req_id(analytics_info):
    return get_analytics_info_values(analytics_info).get("parent_request_id")


def get_parent_scenario(analytics_info):
    return get_analytics_info_values(analytics_info).get("parent_product_scenario_name")


def get_location(request, analytics_info):
    if analytics_info and analytics_info.get('location'):
        return analytics_info.get('location')
    return request.get('location')


def get_type(type_, utterance_source, callback_name, callback_args, app):
    # see https://wiki.yandex-team.ru/users/ilnur/alice/inputtype/
    if utterance_source == 'suggested' or (
        callback_name and (
            # todo: list with names will decreased
            # after gluing with simultaneous 'on_suggest'
            # and detecting click by presence of 'button_id' in callback_args
            callback_name in (
                'on_reset_session',
                'on_card_action',
                'on_suggest',
                'external_skill__on_external_button',  # todo: glue with simultaneous 'on_suggest'
                'new_dialog_session',
                'face_transform_request'  # todo: glue with simultaneous 'on_suggest'
            ) or callback_name.startswith('alice.')  # todo: glue with simultaneous 'on_suggest'
            or (
                callback_name == '@@mm_semantic_frame'
                and get_dict_path(callback_args, ['analytics', 'origin'], '') == 'RemoteControl'
                and 'setup_rcu' not in get_dict_path(callback_args, ['analytics', 'product_scenario'], '')
            )
            or (
                callback_name == 'update_form'
                and get_dict_path(callback_args, ['form_update', 'name'], '') == 'personal_assistant.scenarios.onboarding_image_search'
            )
        )
    ):
        result = 'click'
    elif type_ == 'UTTERANCE':
        result = utterance_source
    elif callback_name == 'update_form' and 'push_id' in callback_args.get('form_update', {}):
        result = 'push'  # only one case: weather push in 'launcher' app
    elif callback_name == 'on_get_greetings' or (
        app in ('quasar', 'small_smart_speakers') and
        callback_name in ('update_form', '@@mm_semantic_frame') and
        get_dict_path(callback_args, ['analytics', 'product_scenario'], '') != 'mordovia'
    ):
        # '@@mm_semantic_frame' in scenarios:
        #   messenger_call (message 'somebody is calling'),
        #   link_a_remote (message 'linking success')
        # 'update_form' in scenarios:
        #   taxi ('car is coming')
        #   reminder (reminder at an appointed time)
        result = 'scenario'
    else:
        result = 'tech'

    return result


def get_request_act_type(parent_req_id, req_id, type_):
    if parent_req_id:
        if parent_req_id == req_id:
            return 'activation'
        elif type_ != 'tech':
            return 'navigation'
        else:
            return 'other'

    return None


# Списки полей
def get_extractors(date):
    from qb2.api.v1 import (
        extractors as se,
        typing as qt,
    )

    return [
        se.log_fields('form_name',
                      'app_id',
                      'callback_name',
                      'callback_args',
                      'utterance_source',
                      'utterance_text',
                      'experiments',
                      'device_revision',
                      'uuid',
                      'lang',
                      'puid',
                      'form',
                      'response',
                      'client_tz',
                      'trash_or_empty_request',
                      'message_id',
                      'enrollment_headers',
                      'guest_data'
                      ),
        se.log_field('do_not_use_user_logs', False),
        se.integer_log_field('client_time'),
        se.log_field('request').hide(),
        se.custom('app_info_proto', convert_app_info_to_proto, 'request', 'lang').hide(),
        se.log_field('analytics_info').rename('analytics_info_').hide(),
        se.log_field('request_stat').hide(),
        se.log_field('biometry_classification').hide(),
        se.log_field('device_id').rename('device_id_').hide(),
        se.log_fields('server_time', 'server_time_ms').hide(),
        se.log_field('type').rename('type_').hide(),
        se.const('fielddate', date).with_type(qt.String),
        se.custom('cards', list_cards, 'response').with_type(qt.Optional[qt.Json]),
        se.custom('slots', lambda ai: byteify(get_slots_sdk(ai)), 'analytics_info_proto').hide(),
        se.custom('slot_name', get_slot_name, 'form_name').hide(),
        se.custom('reply',
                  get_reply,
                  'response',
                  'form',
                  'intent',
                  'slot_name',
                  'slots',
                  'analytics_info').with_type(qt.Optional[qt.String]),
        se.custom('voice_text', get_voice_text, 'response').with_type(qt.Optional[qt.String]),
        se.custom('suggests', list_suggests, 'response').with_type(qt.Optional[qt.Json]),
        se.custom('req_id', get_request_id, 'request').with_type(qt.Optional[qt.String]),
        se.custom('icookie', get_icookie, 'request').with_type(qt.Optional[qt.String]),
        se.custom('enviroment_state', get_enviroment_state, 'request').with_type(qt.Optional[qt.Json]),
        # Inside happens shallow copy. Has to be deepcopy
        se.custom('analytics_info', patch_analytics_info, 'analytics_info_').with_type(qt.Optional[qt.Json]),
        # That's why we convert to proto here
        se.custom('analytics_info_proto', convert_analytics_info_to_proto, 'analytics_info_').hide(),
        se.custom('callback_args_json', lambda ca: json.dumps(ca) if ca else '{}', 'callback_args').hide(),
        se.custom('alice_speech_end_ms', get_alice_speech_end_ms, 'request_stat', 'server_time_ms').with_type(qt.Optional[qt.Int64]),
        se.custom('is_interrupted', is_interrupted, 'request_stat').with_type(qt.Optional[qt.Bool]),
        se.custom('child_confidence', get_child_confidence, 'biometry_classification').with_type(qt.Optional[qt.Float]),
        se.custom('skill_info',
                  get_skill_info,
                  'form',
                  'form_name',
                  'analytics_info',
                  'slot_name',
                  'slots').with_type(qt.Optional[qt.Json]),
        se.custom('skill_id', get_skill_id, 'skill_info').with_type(qt.Optional[qt.String]),
        se.custom('intent',
                  extract_intent,
                  'form_name',
                  'form',
                  'response',
                  'callback_args',
                  'cards',
                  'analytics_info',
                  'skill_info',
                  'slot_name',
                  'slots',
                  'analytics_info_proto',
                  'callback_args_json'
                  ).allow_null_dependency().with_type(qt.Optional[qt.String]),
        se.custom('restored_intent',
                  get_restored_intent,
                  'form',
                  'response',
                  'callback_args',
                  'cards',
                  'skill_info',
                  'slot_name',
                  'slots',
                  'analytics_info_proto',
                  'callback_args_json'
                  ).allow_null_dependency().with_type(qt.Optional[qt.String]),
        se.custom('mm_scenario',
                  extract_mm_scenario,
                  'analytics_info'
                  ).allow_null_dependency().with_type(qt.Optional[qt.String]),
        se.custom('product_scenario_name', get_product_scenario_name_sdk, 'analytics_info_proto').allow_null_dependency().with_type(qt.Optional[qt.String]),
        se.custom('form_changed',
                  form_changed_sdk,
                  'analytics_info_proto',
                  'callback_args_json',
                  ).allow_null_dependency().with_type(qt.Optional[qt.Int64]),
        se.custom('external_session_id_seq',
                  get_external_session_id_seq,
                  'form',
                  'slot_name',
                  'slots').with_type(qt.Optional[qt.Json]),
        se.custom('error_type',
                  get_error,
                  'form_name',
                  'analytics_info',
                  'response').with_type(qt.Optional[qt.String]),
        se.custom('user_id_from_cookies', get_user_id_from_cookies, 'request').with_type(qt.Optional[qt.String]),
        se.custom('app', get_app_sdk, 'app_info_proto').with_type(qt.Optional[qt.String]),
        se.custom('country_id', get_country_id, 'request').with_type(qt.Optional[qt.Int64]),
        se.custom('directives', get_directives, 'response', 'app').with_type(qt.Optional[qt.Json]),
        se.custom('platform', get_platform_sdk, 'app_info_proto').with_type(qt.Optional[qt.String]),
        se.custom('version', get_version_sdk, 'app_info_proto').with_type(qt.Optional[qt.String]),
        se.custom('testids', get_testids, 'request').with_type(qt.Optional[qt.Json]),
        se.custom('expboxes', get_expboxes, 'request').with_type(qt.Optional[qt.String]),
        se.custom('correct_record',
                  is_correct_record,
                  'form_name',
                  'app_id',
                  'mm_scenario'
                  ).allow_null_dependency().hide(),
        se.custom('gc_intent',
                  get_gc_intent,
                  'intent',
                  'form_name',
                  'response',
                  'analytics_info',
                  'analytics_info_proto'
                  ).with_type(qt.Optional[qt.String]),
        se.custom('gc_source',
                  get_gc_source,
                  'intent',
                  'response',
                  'analytics_info'
                  ).allow_null_dependency().with_type(qt.Optional[qt.String]),
        se.custom('ms_server_time',
                  get_ms_server_time,
                  'server_time_ms',
                  'server_time'
                  ).with_type(qt.Optional[qt.Int64]),
        se.custom('device_id', get_device_id, 'request', 'app', 'device_id_').with_type(qt.Optional[qt.String]),
        se.custom('device', get_device, 'request').with_type(qt.Optional[qt.String]),
        se.custom('is_tv_plugged_in', get_tv_state, 'request').with_type(qt.Optional[qt.Bool]),
        se.custom('screen', get_screen, 'request').with_type(qt.Optional[qt.String]),
        se.custom('sound_level', get_sound_level, 'request').with_type(qt.Optional[qt.Float]),
        se.custom('sound_muted', get_sound_muted, 'request').with_type(qt.Optional[qt.Bool]),
        se.custom('music_answer_type',
                  get_music_answer_type,
                  'analytics_info_proto',
                  'form_name',
                  'intent',
                  'slot_name',
                  'slots').with_type(qt.Optional[qt.String]),
        se.custom('music_genre', get_music_genre_sdk, 'analytics_info_proto').with_type(qt.Optional[qt.String]),
        se.custom('filters_genre', get_filters_genre_sdk, 'analytics_info_proto').with_type(qt.Optional[qt.String]),
        se.custom('is_smart_home_user', smart_home_user_sdk, 'analytics_info_proto').with_type(qt.Optional[qt.Bool]),
        se.custom('location', get_location, 'request', 'analytics_info').with_type(qt.Optional[qt.Json]),
        se.custom('subscription', get_subscription, 'analytics_info').with_type(qt.Optional[qt.String]),
        se.custom('parent_req_id', get_parent_req_id, 'analytics_info').with_type(qt.Optional[qt.String]),
        se.custom('parent_scenario', get_parent_scenario, 'analytics_info').with_type(qt.Optional[qt.String]),
        se.custom('type',
                  get_type,
                  'type_',
                  'utterance_source',
                  'callback_name',
                  'callback_args',
                  'app').allow_null_dependency().with_type(qt.Optional[qt.String]),
        se.custom('request_act_type',
                  get_request_act_type,
                  'parent_req_id',
                  'req_id',
                  'type'
                  ).allow_null_dependency().with_type(qt.Optional[qt.String])
    ]


def get_filters():
    from qb2.api.v1 import (
        filters as sf,
    )

    return [
        sf.equals('correct_record', 1),
    ]
