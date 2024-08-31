# -*-coding: utf8 -*-
import json


def extract_hyp_from_asr_responses(asr_responses):
    recognition = asr_responses[-1]['directive']['payload']['recognition']
    if recognition:
        words = recognition[0]['words']
        return ' '.join([word['value'] for word in words]).lower()
    else:
        return ''


def get_asr(VinsResponse, asr_responses, ignore=False):
    if VinsResponse:
        vins_loaded = json.loads(VinsResponse)
        if not ignore or (ignore and 'original_utterance' in vins_loaded['directive']['payload']['megamind_analytics_info']):
            return vins_loaded['directive']['payload']['megamind_analytics_info']['original_utterance']
        elif ignore and asr_responses:
            return extract_hyp_from_asr_responses(json.loads(asr_responses))
    return ''


def get_chosen(VinsResponse):
    if VinsResponse:
        return json.loads(VinsResponse)['directive']['payload']['megamind_analytics_info'].get('chosen_utterance', '')
    return ''
