# -*-coding: utf8 -*-
from nile.api.v1 import Record, extractors as ne
from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps
import utils.yt.basket_common as common
from functools import partial

from visualize_quasar_sessions import get_json_tasks, random_hash, get_path
from basket_configs import get_basket_param

import sys
import json

reload(sys)

sys.setdefaultencoding('utf8')


def get_new_alice_answer(records, override_basket_params=None, parse_iot=False, new_format=True):
    for rec in records:
        session_0 = rec['session_0'] if rec.get('session_0') else {}
        if rec['vins_response'] is None:
            sys.stderr.write('There is no vins response in record with id "%s". Record is skipped.' % rec['request_id'])
            continue
        vins_response = json.loads(rec['vins_response'], encoding='utf-8')['directive']['payload']

        if 'text' in rec:
            session_0['_query'] = rec['text']

        cards = get_path(vins_response, ['response', 'cards'], [])
        cards_reply = None

        if cards and len(cards) > 0 and 'text' in cards[0]:
            cards_reply = cards[0]['text']

        session_0['_reply'] = cards_reply
        session_0['voice_text'] = get_path(vins_response, ['voice_response', 'output_speech', 'text'])

        meta = session_0.get('meta', [])
        if vins_response.get('response') and 'meta' in vins_response['response']:
            meta = vins_response['response']['meta']
        session_0['meta'] = meta

        directives = session_0.get('directives', [])

        # Думаю, что можно будет вообще дропнуть после отрыва meta MEGAMIND-1685, @irinfox
        if vins_response.get('response') and 'directives' in vins_response['response']:
            directives = vins_response['response']['directives']
        if directives and directives[0]['name'] == "music_play":
            if 'morning_show' in rec.get('intent'):
                # VA-1514
                directives[0]['payload']['first_track_id'] = ""
            elif not directives[0]['payload'].get('first_track_id'):
                if meta and meta[0].get('form', {}).get('slots'):
                    for slot in meta[0]['form']['slots']:
                        if slot.get('types') and slot['types'][0] == "music_result":
                            directives[0]['payload']['first_track_id'] = \
                                slot['value'].get('uri', '').split('?')[0].rstrip('/')
        session_0['directives'] = directives

        iot_config = rec.get('iot_config')
        if not iot_config and rec.get('device_state'):
            iot_config = rec['device_state'].get('iot_config')

        session_0['intent'] = rec.get('intent')
        session_0['generic_scenario'] = rec.get('generic_scenario')

        session_0['req_id'] = rec['request_id']
        if rec.get('ts'):
            session_0['ts'] = rec['ts']
        session_0['tz'] = rec.get('tz') or "Europe/Moscow"
        session_0['type'] = "voice"
        session_0['iot_config'] = iot_config

        if rec.get('device_state'):
            device_state = rec['device_state']
        else:
            device_state = rec['session_0'] if rec.get('session_0') else {}
        session_0['alarm_state'] = device_state.get('alarm_state', {})
        session_0['is_tv_plugged_in'] = device_state.get('is_tv_plugged_in', True)
        session_0['music'] = device_state.get('music', {})
        session_0['radio'] = device_state.get('radio', {})
        session_0['audio_player'] = device_state.get('audio_player', {})
        session_0['sound_level'] = device_state.get('sound_level', 1)
        session_0['sound_muted'] = device_state.get('sound_muted', False)
        session_0['timers'] = device_state.get('timers', {"active_timers": []})
        session_0['video'] = device_state.get('video', {'current_screen': 'main'})
        session_0['navigator'] = device_state.get('navigator', {})
        session_0['filtration_level'] = common.get_filtration_level(device_state, rec.get('additional_options'))
        session_0['analytics_info'] = vins_response.get('megamind_analytics_info', {})
        session_0['location'] = rec.get('location')  # human-readable строка из корзины
        session_0['toloka_extra_state'] = rec.get('toloka_extra_state')  # дополнительные данные из корзины для отображения в состоянии станции в Толоке
        session_0['messenger_call'] = device_state.get('messenger_call')

        if 'basket' not in rec or rec['basket'] == 'input_basket':
            session_0['new_format'] = new_format
            session_0['parse_iot'] = parse_iot
        else:
            session_0['new_format'] = get_basket_param('new_format', basket_alias=rec['basket'], override_basket_params=override_basket_params)
            session_0['parse_iot'] = get_basket_param('parse_iot', basket_alias=rec['basket'], override_basket_params=override_basket_params)

        if rec.get('prev_response') or rec.get('prev_reqid') or rec.get('session_2', None):
            session_2 = rec['session_2'] if rec.get('session_2') else {}
            if rec.get('prev_response'):
                prev_response = json.loads(rec['prev_response'])

            else:
                prev_response = {}

            if 'prev_text' in rec:
                session_2['_query'] = rec['prev_text']

            prev_reply = session_2.get('_reply', "EMPTY")
            reply_changed = False
            if prev_response.get('response') and prev_response['response'].get('card'):
                if prev_response['response']['card'].get('text'):
                    prev_reply = prev_response['response']['card']['text']
                    reply_changed = True
            if not reply_changed and prev_response.get('voice_response') and \
                    prev_response['voice_response'].get('output_speech') \
                    and prev_response['voice_response']['output_speech'].get('text'):
                prev_reply = prev_response['voice_response']['output_speech']['text']
            session_2['_reply'] = prev_reply

            prev_meta = session_2.get('meta', [])
            if prev_response.get('response') and 'meta' in prev_response['response']:
                prev_meta = prev_response['response']['meta']
            session_2['meta'] = prev_meta

            # Думаю, что можно будет вообще дропнуть после отрыва meta MEGAMIND-1685, @irinfox
            prev_directives = session_2.get('directives', [])
            if prev_response.get('response') and 'directives' in prev_response['response']:
                prev_directives = prev_response['response']['directives']
            if prev_directives and isinstance(prev_directives, list) and \
                    prev_directives[0]['name'] == "music_play" and not prev_directives[0]['payload'].get('first_track_id'):
                if prev_meta and prev_meta[0].get('form', {}).get('slots'):
                    for slot in prev_meta[0]['form']['slots']:
                        if slot.get('types') and slot['types'][0] == "music_result":
                            prev_directives[0]['payload']['first_track_id'] = \
                                slot['value'].get('uri', '').split('?')[0].rstrip('/')
            session_2['directives'] = prev_directives

            session_2['intent'] = rec.get('prev_intent')
            session_2['generic_scenario'] = rec.get('prev_generic_scenario')

            if 'prev_reqid' in rec:
                session_2['req_id'] = rec['prev_reqid']
            if not session_2.get('req_id'):
                session_2['req_id'] = ''

            if rec.get('prev_ts'):
                session_2['ts'] = rec['prev_ts']
            if not session_2.get('ts'):
                session_2['ts'] = session_0['ts'] - 3600
            session_2['tz'] = rec.get('prev_tz') or "Europe/Moscow"
            session_2['type'] = "voice"

            if rec.get('prev_device_state'):
                prev_device_state = rec['prev_device_state']
            else:
                prev_device_state = rec['session_2'] if rec.get('session_2') else {}
            session_2['alarm_state'] = prev_device_state.get('alarm_state', {})
            session_2['is_tv_plugged_in'] = prev_device_state.get('is_tv_plugged_in', True)
            session_2['music'] = prev_device_state.get('music', {})
            session_2['radio'] = prev_device_state.get('radio', {})
            session_2['audio_player'] = prev_device_state.get('audio_player', {})
            session_2['sound_level'] = prev_device_state.get('sound_level', 1)
            session_2['sound_muted'] = prev_device_state.get('sound_muted', False)
            session_2['timers'] = prev_device_state.get('timers', {"active_timers": []})
            session_2['video'] = prev_device_state.get('video', {'current_screen': 'main'})
            session_2['navigator'] = prev_device_state.get('navigator', {})
            session_2['filtration_level'] = common.get_filtration_level(prev_device_state, rec.get('prev_additional_options'))
            session_2['analytics_info'] = prev_response.get('megamind_analytics_info', {})
            session_2['location'] = rec.get('prev_location')  # human-readable строка из корзины
            session_2['messenger_call'] = prev_device_state.get('messenger_call')

            if 'basket' not in rec or rec['basket'] == 'input_basket':
                session_2['new_format'] = new_format
                session_2['parse_iot'] = parse_iot
            else:
                session_2['new_format'] = get_basket_param('new_format', basket_alias=rec['basket'], override_basket_params=override_basket_params)
                session_2['parse_iot'] = get_basket_param('parse_iot', basket_alias=rec['basket'], override_basket_params=override_basket_params)

            if 'request_id' in rec:
                session_id = session_2['req_id'] + "__" + session_0['req_id']
            else:
                session_id = rec['context_2'] + "____" + rec['context_0']
            yield Record(**{
                "session": [session_2, session_0],
                "session_id": session_id,
                "app": rec.get('app')
            })
        else:
            session_id = rec['request_id'] if 'request_id' in rec else rec['context_2'] + "____" + rec['context_0']
            yield Record(**{
                "session": [session_0],
                "session_id": 'no_session_' + session_id,
                "app": rec.get('app')
            })


def main(input_table=None, pool=None, tmp_path=None, old_format_table=None, new_format_table=None,
         override_basket_params=None, parse_iot=False, new_format=True):
    templates = {"job_root": "//tmp/robot-voice-qa"}
    if tmp_path:
        templates['tmp_files'] = tmp_path

    cluster = hahn_with_deps(
                pool=pool,
                templates=templates,
                neighbours_for=__file__,
                neighbour_names=['visualize_quasar_sessions.py', 'generic_scenario_to_human_readable.py',
                                 'intents_to_human_readable.py', 'basket_configs.py', 'standardize_answer.py'],
                include_utils=True
              )
    job = cluster.job()

    tasks_table, long_sessions_table = job.table(input_table) \
        .map(partial(
            get_new_alice_answer,
            override_basket_params=override_basket_params,
            parse_iot=parse_iot,
            new_format=new_format
        )) \
        .map(get_json_tasks)

    old_format_table = old_format_table or "//tmp/robot-voice-qa/fix_basket_quasar_task_" + random_hash()
    tasks_table.put(old_format_table)
    new_format_table = new_format_table or "//tmp/robot-voice-qa/fix_basket_quasar_task_" + random_hash()
    long_sessions_table.put(new_format_table)

    job.run()

    return [{"cluster": "hahn", "table": old_format_table}, {"cluster": "hahn", "table": new_format_table}]


if __name__ == '__main__':
    call_as_operation(main)
