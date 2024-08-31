# coding: utf-8

from functools import partial

import yatest
from nile.api.v1.clusters import MockYQLCluster

from alice.analytics.utils.testing_utils.nile_testing_utils import local_nile_run
from alice.analytics.operations.priemka.alice_parser.lib.alice_parser import AliceParser
from alice.analytics.operations.priemka.alice_parser.visualize.visualize_request import get_request_visualize_data
from alice.analytics.operations.priemka.alice_parser.visualize.visualize_directive import _prepare_directives_list
from .utils import BEFORE_MAKE_SESSIONS_SCHEMA

from .utils import common_canonized_test


def prepare_directives(directive_names):
    """Обёртка над _prepare_directives_list, куда можно передавать только имена директив, возвращает тоже имена"""
    return [x.get('name') for x in _prepare_directives_list([{'name': name} for name in directive_names])]


def test_prepare_directives_list():
    assert prepare_directives([]) == []
    assert prepare_directives(['111', '222']) == ['111', '222']
    assert prepare_directives(['player_pause', 'tts_play_placeholder', '@@mm_stack_engine_get_next']) == [
        'tts_play_placeholder', '@@mm_stack_engine_get_next']
    assert prepare_directives(['mordovia_show', 'tts_play_placeholder']) == ['mordovia_show', 'tts_play_placeholder']
    assert prepare_directives(['player_pause', 'audio_stop', 'tts_play_placeholder', '@@mm_stack_engine_get_next']) == [
        'tts_play_placeholder', '@@mm_stack_engine_get_next']
    assert prepare_directives(['player_pause', 'clear_queue']) == ['player_pause']
    assert prepare_directives(['player_pause']) == ['player_pause']
    assert prepare_directives(['player_pause', 'tts_play_placeholder']) == ['tts_play_placeholder']
    assert prepare_directives(['mordovia_show', 'tts_play_placeholder', 'clear_queue']) == [
        'mordovia_show', 'tts_play_placeholder']
    assert prepare_directives(['audio_play']) == ['audio_play']
    assert prepare_directives(['@@mm_stack_engine_get_next']) == ['@@mm_stack_engine_get_next']
    assert prepare_directives(['audio_play', '@@mm_stack_engine_get_next']) == ['audio_play']


def test_nile_visualize_01():
    ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

    job = MockYQLCluster().job()
    job.table('dummy').label('input').call(ap.visualize).label('output')

    input_path = yatest.common.runtime.work_path('tests_data/01_input_data/01_visualize.in.json')

    output_path = local_nile_run(job, input_path, schema=BEFORE_MAKE_SESSIONS_SCHEMA)

    return yatest.common.canonical_file(output_path)


def test_nile_visualize_video_play_02():
    ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

    job = MockYQLCluster().job()
    job.table('dummy').label('input').call(ap.visualize).label('output')

    input_path = yatest.common.runtime.work_path('test_hashable/02_hashable_video_play.json')

    output_path = local_nile_run(job, input_path, schema=BEFORE_MAKE_SESSIONS_SCHEMA)

    return yatest.common.canonical_file(output_path)


def test_nile_visualize_eosp_pauses_01():
    ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

    job = MockYQLCluster().job()
    job.table('dummy').label('input').call(ap.visualize).label('output')

    input_path = yatest.common.test_source_path('test_visualize/09_eosp.in.json')

    output_path = local_nile_run(job, input_path, schema=BEFORE_MAKE_SESSIONS_SCHEMA)

    return yatest.common.canonical_file(output_path, local=True)


def test_nile_visualize_zen_scenario_02():
    ap = AliceParser('dummy', 'dummy', 'dummy', 'dummy', token='dummy')

    job = MockYQLCluster().job()
    job.table('dummy').label('input').call(ap.visualize).label('output')

    input_path = yatest.common.test_source_path('test_visualize/15_zen_test.in.json')

    output_path = local_nile_run(job, input_path, schema=BEFORE_MAKE_SESSIONS_SCHEMA)

    return yatest.common.canonical_file(output_path, local=True)


def test_visualize_quasar_sessions():
    return common_canonized_test(
        get_request_visualize_data,
        'tests_data/02_input_data/02_visualize_quasar_sessions.in.json',
        is_source_local=False,
    )


def test_visualize_general_sessions():
    return common_canonized_test(
        partial(get_request_visualize_data, is_quasar=False),
        'test_general/03_general_actions.in.json',
        is_source_local=False,
        is_dest_local=False,
    )


def test_visualize_fairy_tales_common_1():
    return common_canonized_test(
        get_request_visualize_data,
        'test_visualize/04_fairytale_common_music_app_or_site_play.in.json'
    )


def test_visualize_fairy_tales_common_2():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/05_fairytales_common_music_play.in.json')


def test_visualize_fairy_tales_common_3():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/06_fairytale_common_not_found.in.json')


def test_visualize_fairy_tales_on_demand_1():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/07_fairytale_on_demand.in.json')


def test_visualize_iot_1():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/08_iot_simple.in.json')


def test_visualize_video_top3():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/10_video_top3.in.json')


def test_visualize_audio_player():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/11_audio_player.in.json')


def test_visualize_audio_player_v2():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/12_audio_player.in.json')


def test_visualize_stop_directives():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/13_stop_directives.in.json')


def test_visualize_radio():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/14_radio.in.json')


def test_only_smart_speakers():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/11_only_smart_speakers.in.json',
                                 only_smart_speakers=True)


def test_visualize_start_image_recognizer():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/16_start_image_recognizer.in.json')


def test_visualize_redirect_to_thin_player():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/17_redirect_to_thin_player.in.json')


def test_visualize_sound_commands():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/18_sound_commands.in.json')


def test_visualize_with_new_start_multiroom_directive_format():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/19_new_start_multiroom_format.in.json',
                                 only_smart_speakers=True)


def test_visualize_iot_new_analytics_info():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/iot_new_analytics_info_test.in.json',
                                 is_source_local=False)


def test_visualize_tv():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/20_visualize_tv_directives.in.json')


def test_visualize_multiple_directives():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/21_multiple_directives.in.json')


def test_visualize_legatus():
    return common_canonized_test(get_request_visualize_data, 'test_visualize/22_legatus.in.json')
