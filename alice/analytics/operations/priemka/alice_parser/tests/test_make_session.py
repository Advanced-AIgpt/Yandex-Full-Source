# coding: utf-8

import os

import yatest.common
from nile.api.v1.clusters import MockYQLCluster
from alice.analytics.utils.testing_utils.nile_testing_utils import local_nile_run
from alice.analytics.operations.priemka.alice_parser.lib.alice_parser import AliceParser
from alice.analytics.operations.priemka.alice_parser.lib.make_session import filter_tech_actions

from .utils import BEFORE_MAKE_SESSIONS_SCHEMA, JOINED_BASKET_DOWNLOADER_SCHEMA


def data_path(filename, is_source_local=True):
    if is_source_local:
        test_folder_name = os.path.basename(__file__).replace('.py', '')
        return yatest.common.test_source_path(os.path.join(test_folder_name, filename))
    else:
        return yatest.common.runtime.work_path(filename)


def test_facts_side_speech_context_01():
    ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

    job = MockYQLCluster().job()
    job.table('dummy').label('input').call(ap.make_session).label('output')

    input_path = data_path('01_session_facts_side_speech.in.json')

    output_path = local_nile_run(job, input_path, schema=BEFORE_MAKE_SESSIONS_SCHEMA)

    return yatest.common.canonical_file(output_path, local=True)


def test_facts_side_speech_main_02():
    ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

    job = MockYQLCluster().job()
    job.table('dummy').label('input').call(ap.make_session).label('output')

    input_path = data_path('02_session_facts_side_speech.in.json')

    output_path = local_nile_run(job, input_path, schema=BEFORE_MAKE_SESSIONS_SCHEMA)

    return yatest.common.canonical_file(output_path, local=True)


def test_all_ue2e_baskets_intents_directives_03():
    """
    Canonized тест на визуализацию всех возможных интентов и директив из всех ue2e корзинок
    На вход принимает файл с json массивом с данными before_visualuze 686 запросов (42Мб, хранится в sandbox)
    На выходе возвращает файл с json массивом объектов в формате pulsar_results с визуализацией и сессией (тоже sandbox)
    """
    ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

    job = MockYQLCluster().job()
    job.table('dummy').label('input').call(ap.prepare_results).label('output')

    input_path = data_path('test_make_session/03_all_ue2e_baskets_intents_directives.in.json', is_source_local=False)

    output_path = local_nile_run(job, input_path, schema=JOINED_BASKET_DOWNLOADER_SCHEMA)

    return yatest.common.canonical_file(output_path, local=False)


def test_filter_tech_actions_01():
    records_list = [{"input_type": "voice", "action": {"answer": "These are the news."}, "state": {"state1": "state1"}},
                    {"input_type": "tech", "action": {"answer": "news1."}, "state": {"state2": "state2"}},
                    {"input_type": "tech", "action": {"answer": "news2."}, "state": {"state3": "state3"}},
                    {"input_type": "voice", "action": {"answer": "text1"}, "state": {"state4": "state4"}},
                    {"input_type": "voice", "action": {"answer": "text2"}, "state": {"state5": "state5"}},
                    {"input_type": "voice", "action": {"answer": "text3"}, "state": {"state6": "state6"}},
                    {"input_type": "voice", "action": {"answer": "text4"}, "state": {"state7": "state7"}}]

    output = [{'action': {'answer': 'These are the news. news1. news2.'}, 'input_type': 'voice', 'state': {'state1': 'state1'}},
              {'action': {'answer': 'text1'}, 'input_type': 'voice', 'state': {'state4': 'state4'}},
              {'action': {'answer': 'text2'}, 'input_type': 'voice', 'state': {'state5': 'state5'}},
              {'action': {'answer': 'text3'}, 'input_type': 'voice', 'state': {'state6': 'state6'}},
              {'action': {'answer': 'text4'}, 'input_type': 'voice', 'state': {'state7': 'state7'}}]

    result = filter_tech_actions(records_list)
    assert result == output


def test_session_with_context_len_in_eosp_query_04():
    """
    Тест проверяет:
    * корректный учёт context_len в сессии - в json данных теста  запросов больше чем требуется
    * учёт и context_len и <EOSp> тега для построения двух сессий с требуемой длиной контекстов

    При этом context_len тут в тесте на 1 меньше, чем разметили асессоры для охвата первого случая
    """
    ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

    job = MockYQLCluster().job()
    job.table('dummy').label('input').call(ap.make_session).label('output')

    input_path = data_path('04_context_len_in_eosp_query.in.json')

    output_path = local_nile_run(job, input_path, schema=BEFORE_MAKE_SESSIONS_SCHEMA)

    return yatest.common.canonical_file(output_path, local=True)


def test_session_with_shortcuts_05():
    """
    Тест проверяет:
    * дополнительные данные open_url
    * данные шортката из alice.apps_fixlist
    """
    ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

    job = MockYQLCluster().job()
    job.table('dummy').label('input').call(ap.make_session).label('output')

    input_path = data_path('05_shortcut_apps_fixlist.in.json')

    output_path = local_nile_run(job, input_path, schema=BEFORE_MAKE_SESSIONS_SCHEMA)

    return yatest.common.canonical_file(output_path, local=True)
