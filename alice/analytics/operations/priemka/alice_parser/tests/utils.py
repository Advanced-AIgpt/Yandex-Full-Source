# coding: utf-8

import os
import json

import yatest

from qb2.api.v1 import (
    typing as qt,
)


def common_canonized_test(fn, input_data_path, is_source_local=True, is_dest_local=True, **kwargs):
    """
    Вызывает функцию `fn` на каждый объект в json'е в файле `input_data_path`
    :param fn: функция (лямбда), которая будет применяться к каждому объекту во входном файле и сохранять результат
    :param str input_data_path: путь к файлу
    :param bool is_source_local: - файл брать локально или из sandbox
    :param bool is_dest_local: - результат загружать локально или в sandbox
    :return str: - результат для канонизации
    """
    if is_source_local:
        test_path = yatest.common.test_source_path(input_data_path)
    else:
        test_path = yatest.common.runtime.work_path(input_data_path)

    with open(test_path) as f:
        input_data = json.load(f)

    def perform(fun):
        return fun()

    output = [perform(lambda: fn(item, **kwargs)) for item in input_data]
    output_filename = os.path.basename(input_data_path).replace('in', 'out')
    output_path = yatest.common.output_path(output_filename)

    json.dump(
        output,
        open(output_path, "wb"),
        ensure_ascii=False,
        indent=4,
        sort_keys=True,
    )

    return yatest.common.canonical_file(output_path, local=is_dest_local)


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

BEFORE_MAKE_SESSIONS_SCHEMA = dict(JOINED_BASKET_DOWNLOADER_SCHEMA, **{
    'action': qt.Optional[qt.Json],
    'hashable': qt.Optional[qt.Json],
    'answer_standard': qt.Optional[qt.String],
    'music_entity': qt.Optional[qt.Json],
    'state': qt.Optional[qt.Json],
    'context_len': qt.Optional[qt.Int64],
})
