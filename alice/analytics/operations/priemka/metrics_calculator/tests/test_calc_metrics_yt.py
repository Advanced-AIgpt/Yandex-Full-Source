# coding: utf-8

import os
import six
from functools import partial

import yatest.common
from nile.api.v1.clusters import MockYQLCluster
from alice.analytics.utils.testing_utils.nile_testing_utils import local_nile_run
from alice.analytics.operations.priemka.metrics_calculator.calc_metrics_yt import calc_metrics_nile
from qb2.api.v1 import (
    typing as qt,
)


def data_path(filename, prepend_path=None):
    test_folder_name = os.path.basename(__file__).replace('.py', '')
    path_items = [test_folder_name, filename]
    if prepend_path:
        path_items = [prepend_path] + path_items
    return yatest.common.test_source_path(os.path.join(*path_items))


def _get_metrics_extended_schema_tests():
    return {
        'metric_alarms_timers': qt.Optional[qt.Float],
        'metric_commands': qt.Optional[qt.Float],
        'metric_integral': qt.Optional[qt.Float],
        'metric_integral_iot': qt.Optional[qt.Float],
        'metric_main': qt.Optional[qt.Float],
        'metric_music': qt.Optional[qt.Float],
        'metric_no_commands': qt.Optional[qt.Float],
        'metric_radio': qt.Optional[qt.Float],
        'metric_search': qt.Optional[qt.Float],
        'metric_toloka_gc': qt.Optional[qt.Float],
        'metric_translate': qt.Optional[qt.Float],
        'metric_video': qt.Optional[qt.Float],
        'metric_weather': qt.Optional[qt.Float],
    }


def _get_sessions_schema():
    return {
        'result': qt.Optional[qt.String],
        'fraud': qt.Optional[qt.Bool],
        'req_id': qt.Optional[qt.String],
        'hashsum': qt.Optional[qt.String],
        'action': qt.Optional[qt.String],
        'answer': qt.Optional[qt.String],
        'session': qt.Optional[qt.Json],
        'text': qt.Optional[qt.String],
        'toloka_intent': qt.Optional[qt.String],
        'app': qt.Optional[qt.String],
        'basket': qt.Optional[qt.String],
        'answer_standard': qt.Optional[qt.String],
        'asr_text': qt.Optional[qt.String],
        'generic_scenario': qt.Optional[qt.String],
        'generic_scenario_human_readable': qt.Optional[qt.String],
        'intent': qt.Optional[qt.String],
        'mm_scenario': qt.Optional[qt.String],
        'screenshot_url': qt.Optional[qt.String],
        'session_id': qt.Optional[qt.String],
        'setrace_url': qt.Optional[qt.String],
        'voice_url': qt.Optional[qt.String],
    }


def test_calc_metrics_yt_01():
    job = MockYQLCluster().job().env(bytes_decode_mode='strict' if six.PY3 else 'never')
    job.table('dummy').label('input').call(
        partial(
            calc_metrics_nile,
            metrics_extended_schema=_get_metrics_extended_schema_tests()
        )
    ).label('output')

    input_path = data_path('01_sample_yt_results.in.json')

    output_path = local_nile_run(job, input_path, schema=_get_sessions_schema())

    return yatest.common.canonical_file(output_path, local=True)


def test_calc_metrics_yt_eosp_02():
    job = MockYQLCluster().job().env(bytes_decode_mode='strict' if six.PY3 else 'never')
    job.table('dummy').label('input').call(
        partial(
            calc_metrics_nile,
            metrics_extended_schema=_get_metrics_extended_schema_tests()
        )
    ).label('output')

    input_path = data_path('02_eosp_yt_results.in.json')

    output_path = local_nile_run(job, input_path, schema=_get_sessions_schema())

    return yatest.common.canonical_file(output_path, local=True)
