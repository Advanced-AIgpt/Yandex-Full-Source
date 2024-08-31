# coding: utf-8

from alice.analytics.operations.priemka.alice_parser.lib.hashing import get_hashable
from alice.analytics.operations.priemka.alice_parser.visualize.visualize_request import get_request_visualize_data

from .utils import common_canonized_test


def run_tests_get_hashable(input_data_path, is_source_local=True, is_dest_local=True, is_quasar=True):
    return common_canonized_test(
        lambda r: get_hashable(r, get_request_visualize_data(r).get('action'), is_quasar),
        input_data_path,
        is_source_local=is_source_local,
        is_dest_local=is_dest_local
    )


def test_hashable_tv_stream_01():
    return run_tests_get_hashable('test_hashable/01_hashable_tv_stream.json', is_source_local=False)


def test_hashable_weather_01():
    return run_tests_get_hashable('test_hashable/01_hashable_weather.json', is_source_local=False)


def test_hashable_weather_02():
    return run_tests_get_hashable('test_hashable/hashable_weather_pp.json', is_source_local=False, is_quasar=False)


def test_hashable_video_play_02():
    return run_tests_get_hashable('test_hashable/02_hashable_video_play.json', is_source_local=False)
