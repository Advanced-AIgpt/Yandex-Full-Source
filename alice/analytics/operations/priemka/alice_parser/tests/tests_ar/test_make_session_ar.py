# coding: utf-8

import os

import yatest.common
from nile.api.v1.clusters import MockYQLCluster
from alice.analytics.utils.testing_utils.nile_testing_utils import local_nile_run
from alice.analytics.operations.priemka.alice_parser.lib.alice_parser import AliceParser

from qb2.api.v1 import (
    typing as qt,
)

JOINED_BASKET_DOWNLOADER_SCHEMA = {
    'additional_options': qt.Optional[qt.Json],
    'analytics_info': qt.Optional[qt.Json],
    'app': qt.Optional[qt.String],
    'asr_text': qt.Optional[qt.String],
    'basket': qt.Optional[qt.String],
    'basket_location': qt.Optional[qt.Json],
    'client_ip': qt.Optional[qt.String],
    'device_state': qt.Optional[qt.Json],
    'directives': qt.Optional[qt.Json],
    'exact_location': qt.Optional[qt.String],
    'filtration_level': qt.Optional[qt.String],
    'generic_scenario': qt.Optional[qt.String],
    'intent': qt.Optional[qt.String],
    'location': qt.Optional[qt.String],
    'location_by_client_ip': qt.Optional[qt.String],
    'location_by_coordinates': qt.Optional[qt.String],
    'location_by_region_id': qt.Optional[qt.String],
    'mm_scenario': qt.Optional[qt.String],
    'meta': qt.Optional[qt.Json],
    'new_format': qt.Bool,
    'parse_iot': qt.Bool,
    'product_scenario': qt.Optional[qt.String],
    'query': qt.Optional[qt.String],
    'region_id': qt.Optional[qt.Int32],
    'reply': qt.Optional[qt.String],
    'req_id': qt.Optional[qt.String],
    'result': qt.Optional[qt.String],
    'reversed_session_sequence': qt.Optional[qt.Int64],
    'session_id': qt.Optional[qt.String],
    'session_sequence': qt.Optional[qt.Int64],
    'setrace_url': qt.Optional[qt.String],
    'text': qt.Optional[qt.String],
    'toloka_extra_state': qt.Optional[qt.Json],
    'toloka_intent': qt.Optional[qt.String],
    'ts': qt.Optional[qt.Int64],
    'tz': qt.Optional[qt.String],
    'vins_response': qt.Optional[qt.Json],
    'voice_text': qt.Optional[qt.String],
    'voice_url': qt.Optional[qt.String],
}


def data_path(filename, is_source_local=True):
    if is_source_local:
        test_folder_name = os.path.basename(__file__).replace('.py', '')
        return yatest.common.test_source_path(os.path.join(test_folder_name, filename))
    else:
        return yatest.common.runtime.work_path(filename)


def test_all_ue2e_baskets_intents_directives_ar():
    """
    Canonized тест на визуализацию всех возможных интентов и директив из всех ue2e корзинок
    На вход принимает файл с json массивом с данными before_visualuze 686 запросов (42Мб, хранится в sandbox)
    На выходе возвращает файл с json массивом объектов в формате pulsar_results с визуализацией и сессией (тоже sandbox)
    """
    ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

    job = MockYQLCluster().job()
    job.table('dummy').label('input').call(ap.prepare_results).label('output')

    input_path = data_path('test_make_session_ar/03_all_ue2e_baskets_intents_directives.in.json', is_source_local=False)

    output_path = local_nile_run(job, input_path, schema=JOINED_BASKET_DOWNLOADER_SCHEMA)

    return yatest.common.canonical_file(output_path, local=False)
