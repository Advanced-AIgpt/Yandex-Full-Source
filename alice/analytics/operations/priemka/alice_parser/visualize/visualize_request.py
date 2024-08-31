# coding: utf-8

import json

import sys
reload(sys)  # noqa
sys.setdefaultencoding('utf8')  # noqa

import six
from alice.analytics.utils.json_utils import get_path_str
from alice.analytics.operations.priemka.alice_parser.utils.queries_utils import clean_voice_text_from_tags

from .visualize_directive import _get_visualized_action, VOLUME_LEVEL_STR, get_human_readable
from .generic_scenario_to_human_readable import generic_scenario_to_human_readable
from .visualize_alarms_timers import _get_cur_date, ALARM_REMINDER_PREFIX
from .visualize_state import _get_visualized_extra_states, _get_visualized_volume_state
from .visualize_video import _get_visualized_state_screen_name
from .visualize_music import get_music_entity
from .visualize_answer_standard import answer_standard
from .visualize_iot import (
    is_smart_home_user,
    IOT_CONFIG_SCENATIOS,
    _get_iot_action,
    _get_iot_extra_states,
)


def _get_visualized_query(record):
    """
    Возвращает запрос пользователя к Алисе
    :param dict record: объект с инфой о запросе
    :return str:
    """
    # запроса может не быть и он может быть пустой строкой
    return record.get('_query') if '_query' in record else record.get('query')


def _get_visualized_hypos(record):
    """
    Возвращает гипотезы ASR по голосовому запросу Алисы
    :param dict record: объект с инфой о запросе
    :return str:
    """
    # запроса может не быть и он может быть пустой строкой
    return record.get('asr_hypos')


def choose_reply(cards_reply, voice_text, generic_scenario, always_voice_text=False):
    # boltalka adopts tts texts, sometimes the answers look orphographically not correct, leave cards_reply for them
    if voice_text and (not (cards_reply and generic_scenario in ('general_conversation', 'external_skill_gc', 'how_to_spell')) or always_voice_text):
        return clean_voice_text_from_tags(voice_text)

    return cards_reply or 'EMPTY'


def _get_visualized_answer(record, always_voice_text=False):
    """
    Возвращает текстовый ответ Алисы пользователю.
    Чаще всего эта та же самая фраза, что и произнесла Алиса голосом voice_text
    :param dict record: объект с инфой о запросе
    :return str:
    """
    query = _get_visualized_query(record)
    cards_reply = record.get('_reply') if '_reply' in record else record.get('reply')
    reply = choose_reply(cards_reply, record.get('voice_text'), record.get('generic_scenario'), always_voice_text=always_voice_text)
    analytics_info = get_path_str(record, 'analytics_info.analytics_info', {})

    if query is None and record.get('callback') and record['callback'].get('form_name') == 'personal_assistant.scenarios.alarm_reminder':
        return ALARM_REMINDER_PREFIX + reply
    if reply != 'EMPTY':
        return reply
    if analytics_info and analytics_info.get('Transcription'):
        return get_human_readable(analytics_info)


def get_device_state_visualize_data(
    record,
    is_quasar=True,
    is_dsat=False,
    only_smart_speakers=False,
    visualized_directive=None
):
    """
    Формирование state: состояние устройства
    :param dict record: объект с распаршенной информацией по запросу
    :param bool is_quasar: флаг того, что это колонка/навигатор, а не Поисковое Приложение (ПП)/Я.Бро
    :param bool is_dsat: флаг того, что нужны дополнительные поля для dsat в визуализации device_state
    :param bool only_smart_speakers: если True, то оставить только колонки в конфиге УД. Иначе - все устройства
    :param Optional[str] visualized_directive: визуализация директивы (action), при наличии
    :return dict:
    """
    state = {
        'time': _get_cur_date(record),
        'extra': _get_visualized_extra_states(record)
    }

    if is_dsat:
        state['red_id'] = record.get('req_id')
        state['intent'] = record.get('intent')

    if is_quasar:
        state['screen'] = _get_visualized_state_screen_name(record)

    if (
        (
            visualized_directive and isinstance(visualized_directive, six.string_types)
            and VOLUME_LEVEL_STR in visualized_directive
        )
        or record['generic_scenario'] == 'sound_commands'
    ):
        state['volume'] = _get_visualized_volume_state(record)

    # IoT
    if is_smart_home_user(record.get('analytics_info')) or only_smart_speakers or \
            record.get('generic_scenario') in IOT_CONFIG_SCENATIOS:
        state['extra'] += _get_iot_extra_states(record, only_smart_speakers)

    if record.get('toloka_extra_state'):
        state['extra'].append(record['toloka_extra_state'])

    # ADI-188: removing symbol u0000 to prevent error 'u0000 cannot be converted to text'
    state = json.loads(json.dumps(state).replace('\\u0000', ''))
    return state


def get_action_visualize_data(
    record,
    is_quasar=True,
    only_smart_speakers=False,
):
    """
    Формирование action: действие колонки, формируется по директивам запроса
    :param dict record: объект с распаршенной информацией по запросу
    :param bool is_quasar: флаг того, что это колонка/навигатор, а не Поисковое Приложение (ПП)/Я.Бро
    :param bool only_smart_speakers: если True, то оставить только колонки в конфиге УД. Иначе - все устройства
    :return Optional[str]:
    """
    action = _get_visualized_action(record, is_quasar=is_quasar)
    if action:
        return action

    iot_action = _get_iot_action(record, only_smart_speakers)
    if iot_action:
        return iot_action

    return None


def get_request_visualize_data(
    record,
    only_smart_speakers=False,
    is_quasar=True,
    need_voice_url=False,
    is_dsat=False,
    always_voice_text=False
):
    """
    Возвращает данные для визуализации по одному запросу из сессии
    :param dict record: объект с распаршенной информацией по запросу
    :param bool only_smart_speakers: флаг того, нужно ли оставить только колонки в конфиге умного дома
    :param bool is_quasar: флаг того, что это колонка/навигатор, а не Поисковое Приложение (ПП)/Я.Бро
    :return dict:
    """

    if record.get('result') in ['UNIPROXY_ERROR', 'EMPTY_VINS_RESPONSE']:
        return {
            'state': {'EMPTY': 'EMPTY'},
            'action': {'EMPTY': 'EMPTY'},
            'req_id': record['req_id'],
            'intent': record['intent'],
            'generic_scenario': record.get('generic_scenario'),
            'music_entity': None,
            'answer_standard': None,
        }

    # 1. Формирование объекта actioni: основных отображаемых полей ответа Алисы
    actioni = {}

    query = _get_visualized_query(record)
    if query:
        actioni['query'] = query

    hypos = _get_visualized_hypos(record)
    if hypos:
        actioni['asr_hypos'] = hypos

    answer = _get_visualized_answer(record, always_voice_text=always_voice_text)
    if answer:
        actioni['answer'] = answer

    action_str = get_action_visualize_data(record, is_quasar, only_smart_speakers)
    if action_str:
        actioni['action'] = action_str

    if record.get('generic_scenario') and record['generic_scenario'] in generic_scenario_to_human_readable:
        actioni['scenario'] = generic_scenario_to_human_readable[record['generic_scenario']]

    if (need_voice_url or is_dsat) and record.get('voice_url'):
        actioni['voice_url'] = record['voice_url']

    if record.get('screenshot_url') and is_quasar is False:
        actioni['url'] = record['screenshot_url']

    return {
        'state': get_device_state_visualize_data(record, is_quasar, is_dsat, only_smart_speakers, action_str),
        'action': actioni,
        'req_id': record['req_id'],
        'intent': record['intent'],
        'generic_scenario': record.get('generic_scenario'),
        'music_entity': get_music_entity(record.get('analytics_info'), record.get('generic_scenario')),
        'answer_standard': answer_standard(record.get('generic_scenario'), actioni.get('action'), actioni.get('answer'),
                                           record.get('app')),
    }
