# coding: utf-8
from qb2.api.v1 import (
    typing as qt,
)

from alice.analytics.operations.priemka.alice_parser.utils.hash_utils import get_hash_for_json, get_hashable_data
from alice.analytics.operations.priemka.alice_parser.utils.errors import get_most_session_unanswer, get_render_error_status

from .utils import (
    get_open_uri_url,
    get_shortcut_nav_url,
)

FULL_SESSION_SCHEMA = {
    'hashsum': qt.Optional[qt.String],
    'hashable': qt.Optional[qt.Json],
    'session': qt.Optional[qt.Json],
    'answer_standard': qt.Optional[qt.String],
    'app': qt.Optional[qt.String],
    'generic_scenario': qt.Optional[qt.String],
    'intent': qt.Optional[qt.String],
    'req_id': qt.Optional[qt.String],
    'session_id': qt.Optional[qt.String],
    'music_entity': qt.Optional[qt.Json],
    'action0': qt.Optional[qt.Json],
    'action1': qt.Optional[qt.Json],
    'action2': qt.Optional[qt.Json],
    'action3': qt.Optional[qt.Json],
    'action4': qt.Optional[qt.Json],
    'action5': qt.Optional[qt.Json],
    'action6': qt.Optional[qt.Json],
    'action7': qt.Optional[qt.Json],
    'action8': qt.Optional[qt.Json],
    'action9': qt.Optional[qt.Json],
    'action10': qt.Optional[qt.Json],
    'state0': qt.Optional[qt.Json],
    'state1': qt.Optional[qt.Json],
    'state2': qt.Optional[qt.Json],
    'state3': qt.Optional[qt.Json],
    'state4': qt.Optional[qt.Json],
    'state5': qt.Optional[qt.Json],
    'state6': qt.Optional[qt.Json],
    'state7': qt.Optional[qt.Json],
    'state8': qt.Optional[qt.Json],
    'state9': qt.Optional[qt.Json],
    'state10': qt.Optional[qt.Json],
    'answer': qt.Optional[qt.String],
    'action': qt.Optional[qt.String],
    'generic_scenario_human_readable': qt.Optional[qt.String],
    'mm_scenario': qt.Optional[qt.String],
    'toloka_intent': qt.Optional[qt.String],
    'result': qt.Optional[qt.String],
    'fraud': qt.Optional[qt.String],
    'is_negative_query': qt.Optional[qt.Int32],
    'text': qt.Optional[qt.String],
    'query_eosp': qt.Optional[qt.String],
    'asr_text': qt.Optional[qt.String],
    'chosen_text': qt.Optional[qt.String],
    'voice_url': qt.Optional[qt.String],
    'setrace_url': qt.Optional[qt.String],
    'basket': qt.Optional[qt.String],
    'open_uri_url': qt.Optional[qt.String],
    'shortcut_nav_url': qt.Optional[qt.String],
}

LONG_SESSION_SCHEMA = dict(FULL_SESSION_SCHEMA, **{
    'action11': qt.Optional[qt.Json],
    'action12': qt.Optional[qt.Json],
    'action13': qt.Optional[qt.Json],
    'action14': qt.Optional[qt.Json],
    'action15': qt.Optional[qt.Json],
    'state11': qt.Optional[qt.Json],
    'state12': qt.Optional[qt.Json],
    'state13': qt.Optional[qt.Json],
    'state14': qt.Optional[qt.Json],
    'state15': qt.Optional[qt.Json],
})

DSAT_SESSION_SCHEMA = dict(LONG_SESSION_SCHEMA, **{
    'uuid': qt.Optional[qt.String],
})

TOLOKA_TASKS_EXCLUDE_FIELDS = [
    'answer', 'action', 'generic_scenario_human_readable', 'mm_scenario', 'toloka_intent', 'chosen_text',
    'result', 'fraud', 'is_negative_query', 'text', 'asr_text', 'voice_url', 'setrace_url', 'basket', 'hashable', 'query_eosp',
    'open_uri_url', 'shortcut_nav_url'
]

PULSAR_RESULTS_EXCLUDE_FIELDS = [
    'music_entity', 'hashable',
    'action0', 'action1', 'action2', 'action3', 'action4', 'action5', 'action6', 'action7', 'action8',
    'action9', 'action10', 'action11', 'action12', 'action13', 'action14', 'action15',
    'state1', 'state2', 'state3', 'state4', 'state5', 'state6', 'state7', 'state8', 'state9', 'state10',
    'state11', 'state12', 'state13', 'state14', 'state15',
]


def get_last_request_main_data(request_data, is_dsat=False):
    PASS_FIELDS = [
        'session_id',
        'req_id',
        'intent',
        'app',
        'generic_scenario',
        'answer_standard',
        'music_entity',
        'mm_scenario',
        'chosen_text',
        'toloka_intent',
        'result',
        'asr_text',
        'text',
        'voice_url',
        'setrace_url',
        'basket',
        'is_negative_query',
        'query_eosp',
    ]
    if is_dsat:
        PASS_FIELDS.append('uuid')
    return {k: v
            for k, v in list(request_data.items())
            if k in PASS_FIELDS}


def get_requests_flattened_data(records_list, is_quasar=True):
    """
    Из массива объектов с полями action, state делает объект с полями action0, state0, action1, state1, ...
    Для ПП (general) добавляет поле hashsum1, hashsum2, ... содержащее hashsum от скриншота,
        чтобы по нему (вместо mds урлов) считать общий hashsum запроса вместо сс
    Если в объекте есть поле hashable — то поле добавится в объект, чтобы по нему считать hashsum целиком
    """
    result = {}
    for i, r in enumerate(reversed(records_list)):
        result['action{}'.format(i)] = r['action']
        result['state{}'.format(i)] = r['state']
        if is_quasar is False and r.get('hashsum'):
            result['hashsum{}'.format(i)] = r['hashsum']
        if r.get('hashable'):
            # поле, по которому целиком считается hashsum вместо dict'а action
            result['hashable{}'.format(i)] = r['hashable']
    return result


def filter_tech_actions(records_list):
    """
    Удаляет записи для input_type == "tech" (технические события)
    Тексты ответов технических событий, если они были, сохраняет для добавления к нетехническому
    запросу перед ними
    """
    current_not_tech_ind = None

    for i, r in enumerate(records_list):
        if r.get('input_type') == 'tech':
            if r.get('action') and r['action'].get('answer') and (current_not_tech_ind is not None) and records_list[
                    current_not_tech_ind].get('action'):
                last_text = records_list[current_not_tech_ind]['action'].get('answer', '')
                if last_text:
                    records_list[current_not_tech_ind]['action']['answer'] = last_text + ' ' + r['action']['answer']
                else:
                    records_list[current_not_tech_ind]['action']['answer'] = r['action']['answer']
        else:
            current_not_tech_ind = i

    return [x for x in records_list if x.get('input_type') != 'tech']


def get_session_list_from_records(records_list):
    session = []
    for r in records_list:
        session = session + [r['state'], r['action']]
    return session


def filter_voice(result):
    for key in list(result.keys()):
        if key.startswith('action') and result[key].get('voice_url'):
            del result[key]['voice_url']
    return result


def make_deep_session(records_list, is_quasar=True, need_voice_url=False, is_dsat=False, merge_tech_queries=False):
    """
    Формирует объект 'глубокий запрос', содержащий как массив session
        со всеми стейтами и экшнами всех контекстов и запросов
    В результирующем объекте находятся все необходимые колонки из корзинок и результатов прокачки,
        чтобы дальше сформировать любой нужный результат
    :param records_list: массив из dict'ов с данными про запрос, и визуализированными
    :param is_quasar: флаг для режима станции или устройств с экраном
    :param need_voice_url: нужен ли voice_url в заданиях Толоки
    :param is_dsat: флаг для режима dsat
    :param merge_tech_queries: флаг объединения answer технических событий (input_type == "tech")
    :return dict:
    """

    # у последнего запроса может быть EMPTY_SIDESPEECH_RESPONSE / NONEMPTY_SIDESPEECH_RESPONSE
    last_record = records_list[-1]
    result = get_last_request_main_data(last_record, is_dsat=is_dsat)
    result['session'] = get_session_list_from_records(records_list)

    # у любого запроса/контекста в сессии может быть UNIPROXY_ERROR / EMPTY_VINS_RESPONSE
    most_session_unanswer = get_most_session_unanswer(records_list)
    if most_session_unanswer is not None:
        result['result'] = most_session_unanswer

    # у последнего запроса в сессии может отсутствовать скриншот —> ставим RENDER_ERROR
    if is_quasar is False:
        render_error = get_render_error_status(last_record)
        if render_error is not None:
            result['result'] = render_error

    if result.get('result') is None:
        # если в result нет ошибок, то формируем задание для толоки
        if merge_tech_queries:
            records_list = filter_tech_actions(records_list)
        requests_flattened_data = get_requests_flattened_data(records_list, is_quasar)
        if not is_dsat and not need_voice_url:
            requests_flattened_data = filter_voice(requests_flattened_data)

        result['hashable'] = get_hashable_data(requests_flattened_data)
        result['hashsum'] = get_hash_for_json(result['hashable'])
        result.update(requests_flattened_data)

    result['open_uri_url'] = get_open_uri_url(last_record.get('directives'))
    result['shortcut_nav_url'] = get_shortcut_nav_url(last_record.get('analytics_info'))

    # 'sorting_hash': str(random()) + random_hash(),  # для сортировки в make_quasar_quality_tasks.py

    return result
