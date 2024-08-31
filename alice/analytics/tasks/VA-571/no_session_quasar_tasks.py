# -*-coding: utf8 -*-
from nile.api.v1 import Record
from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps

from visualize_quasar_sessions import get_json_tasks, random_hash

import json
import re


def levenshtein_distance(a, b):
    n, m = len(a), len(b)
    if n > m:
        a, b, n, m = b, a, m, n
    current_row = range(n + 1)
    for i in range(1, m + 1):
        previous_row, current_row = current_row, [i] + [0] * n
        for j in range(1, n + 1):
            add, delete, change = previous_row[j] + 1, current_row[j - 1] + 1, previous_row[j - 1]
            if a[j - 1] != b[i - 1]:
                change += 1
            current_row[j] = min(add, delete, change)
    return current_row[n]


def get_new_alice_answer(records):
    for rec in records:
        if rec['vins_response']:
            reply, directives, meta = "EMPTY", [], []
            intent_str_start = rec['vins_response'][rec['vins_response'].find('intent'):][10:]
            intent = intent_str_start[: intent_str_start.find('"')].replace('.', '\t').replace('__', '\t')
            try:
                vins_response = json.loads(rec['vins_response'], encoding='utf-8')['directive']['payload']

                if vins_response.get('voice_response') and vins_response['voice_response'].get('output_speech'):
                    reply = vins_response['voice_response']['output_speech'].get('text', "EMPTY")
                    regexp = r'''#acc|#gen|#dat|#instr|#loc|#nom|sil<\[[0-9]*\]>|sil *<\[ *[0-9]* *\]>|
                                <speaker voice="[a-zA-Z .]*">|<\[[\a-zA-Z =\~]*\]>|<speaker[^>]*>|sil <\[[0-9]*\]>'''
                    reply = re.sub(regexp, '', reply).replace(".\n\n.sil ", ".\n\n").replace(".sil ", ". ") \
                        .replace(" .", ".").replace("  ", " ").strip()
                cards_reply = "EMPTY"
                if vins_response.get('response') and vins_response['response'].get('cards') and \
                        len(vins_response['response']['cards']) > 0 and 'text' in vins_response['response']['cards'][0]:
                    cards_reply = vins_response['response']['cards'][0]['text']
                if reply == "EMPTY":
                    reply = cards_reply
                if reply != cards_reply and "+" in reply:
                    if levenshtein_distance(cards_reply, reply) <= 2 * len(re.findall(r'\+', reply)) + 1:
                        reply = cards_reply
                    else:
                        splitted = reply.split('+')
                        reply = splitted[0]
                        if len(splitted) > 1:  # remove "+" before russian vowels
                            for el in splitted[1:]:
                                if (el.startswith("а") or el.startswith("е") or el.startswith("ё") or el.startswith("и")
                                        or el.startswith("о") or el.startswith("у") or el.startswith("ы")
                                        or el.startswith("э") or el.startswith("ю") or el.startswith("я")
                                        or el.startswith("А") or el.startswith("Е") or el.startswith("Ё")
                                        or el.startswith("И") or el.startswith("О") or el.startswith("У")
                                        or el.startswith("Э") or el.startswith("Ю") or el.startswith("Я")):
                                    reply += el
                                else:
                                    reply += "+" + el

                directives = vins_response['response']['directives']
                meta = vins_response['response'].get('meta', [])

                if directives and directives[0].get('name') == "music_play" and \
                        not directives[0]['payload']['first_track_id']:
                    if meta and meta[0].get('form', {}).get('slots'):
                        for slot in meta[0]['form']['slots']:
                            if slot['types'][0] == "music_result":
                                directives[0]['payload']['first_track_id'] = \
                                    slot['value'].get('uri', '').split('?')[0].rstrip('/')

                for meta_obj in meta:
                    if meta_obj['type'] == 'analytics_info':
                        if 'intent' in meta_obj:
                            vins_intent = meta_obj['intent'].replace('.', '\t').replace('__', '\t')
                        elif 'intent' in meta_obj.get('payload', {}):
                            vins_intent = meta_obj['payload']['intent'].replace('.', '\t').replace('__', '\t')
                        if not (vins_intent == "personal_assistant\tscenarios\tsearch" and intent != ""):
                            intent = vins_intent
                            break

            except (ValueError, TypeError):
                continue

            session_0 = {
                '_query': rec['text'],
                '_reply': reply,
                'alarm_state': rec['device_state'].get('alarm_state', {}),
                'directives': directives,
                'meta': meta,
                'intent': intent.replace('.', '\t').replace('__', '\t'),
                'is_tv_plugged_in': rec['device_state'].get('is_tv_plugged_in', True),
                'music': rec['device_state'].get('music', {}),
                'radio': rec['device_state'].get('radio', {}),
                'req_id': rec['request_id'],
                'sound_level': rec['device_state'].get('sound_level', 1),
                'sound_muted': rec['device_state'].get('sound_muted', False),
                'timers': rec['device_state'].get('timers', {"active_timers": []}),
                'ts': rec['ts'],
                'tz': "Europe/Moscow",
                'type': 'voice',
                'video': rec['device_state'].get('video', {'current_screen': 'main'})
            }
            yield Record(**{
                "session": [session_0],
                "session_id": 'no_session_' + rec['request_id']
            })


def main(input_table=None, pool=None, tmp_path=None, output_table=None):
    templates = {"job_root": "//tmp/robot-voice-qa"}
    if tmp_path:
        templates['tmp_files'] = tmp_path

    cluster = hahn_with_deps(pool=pool,
                             templates=templates,
                             neighbours_for=__file__,
                             neighbour_names=['visualize_quasar_sessions.py', 'generic_scenario_to_human_readable.py',
                                              'intents_to_human_readable.py', 'standardize_answer.py'])
    job = cluster.job()

    output_table = output_table or "//tmp/robot-voice-qa/no_session_quasar_task_" + random_hash()

    job.table(input_table) \
        .map(get_new_alice_answer) \
        .map(get_json_tasks) \
        .put(output_table)

    job.run()
    return {"cluster": "hahn", "table": output_table}


if __name__ == '__main__':
    call_as_operation(main)
