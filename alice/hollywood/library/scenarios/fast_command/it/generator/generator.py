from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import create_run_request_generator_fun

import alice.hollywood.library.scenarios.fast_command.it.stop_commands.alarm_stop as alarm_stop
import alice.hollywood.library.scenarios.fast_command.it.stop_commands.music_player_stop as music_player_stop
import alice.hollywood.library.scenarios.fast_command.it.stop_commands.navi_stop as navi_stop
import alice.hollywood.library.scenarios.fast_command.it.stop_commands.simple_stop as simple_stop
import alice.hollywood.library.scenarios.fast_command.it.stop_commands.thin_audio_player_stop as thin_audio_player_stop
import alice.hollywood.library.scenarios.fast_command.it.stop_commands.timer_stop as timer_stop

import alice.hollywood.library.scenarios.fast_command.it.sound_commands.simple_sound as simple_sound

test_data_alarm_stop = create_run_request_generator_fun(
    alarm_stop.SCENARIO_NAME,
    alarm_stop.SCENARIO_HANDLE,
    alarm_stop.TEST_GEN_PARAMS,
    alarm_stop.TESTS_DATA_PATH,
    alarm_stop.TESTS_DATA,
    alarm_stop.DEFAULT_EXPERIMENTS,
    alarm_stop.DEFAULT_DEVICE_STATE)

test_data_music_player_stop = create_run_request_generator_fun(
    music_player_stop.SCENARIO_NAME,
    music_player_stop.SCENARIO_HANDLE,
    music_player_stop.TEST_GEN_PARAMS,
    music_player_stop.TESTS_DATA_PATH,
    music_player_stop.TESTS_DATA,
    music_player_stop.DEFAULT_EXPERIMENTS,
    music_player_stop.DEFAULT_DEVICE_STATE)

test_data_thin_audio_player_stop = create_run_request_generator_fun(
    thin_audio_player_stop.SCENARIO_NAME,
    thin_audio_player_stop.SCENARIO_HANDLE,
    thin_audio_player_stop.TEST_GEN_PARAMS,
    thin_audio_player_stop.TESTS_DATA_PATH,
    thin_audio_player_stop.TESTS_DATA,
    thin_audio_player_stop.DEFAULT_EXPERIMENTS,
    thin_audio_player_stop.DEFAULT_DEVICE_STATE,
    default_supported_features=thin_audio_player_stop.DEFAULT_SUPPORTED_FEATURES)

test_data_navi_stop = create_run_request_generator_fun(
    navi_stop.SCENARIO_NAME,
    navi_stop.SCENARIO_HANDLE,
    navi_stop.TEST_GEN_PARAMS,
    navi_stop.TESTS_DATA_PATH,
    navi_stop.TESTS_DATA,
    navi_stop.DEFAULT_EXPERIMENTS)

test_data_simple_stop = create_run_request_generator_fun(
    simple_stop.SCENARIO_NAME,
    simple_stop.SCENARIO_HANDLE,
    simple_stop.TEST_GEN_PARAMS,
    simple_stop.TESTS_DATA_PATH,
    simple_stop.TESTS_DATA,
    simple_stop.DEFAULT_EXPERIMENTS)

test_data_timer_stop = create_run_request_generator_fun(
    timer_stop.SCENARIO_NAME,
    timer_stop.SCENARIO_HANDLE,
    timer_stop.TEST_GEN_PARAMS,
    timer_stop.TESTS_DATA_PATH,
    timer_stop.TESTS_DATA,
    timer_stop.DEFAULT_EXPERIMENTS,
    timer_stop.DEFAULT_DEVICE_STATE)

test_data_simple_sound_0 = create_run_request_generator_fun(
    simple_sound.SCENARIO_NAME,
    simple_sound.SCENARIO_HANDLE,
    simple_sound.TEST_GEN_PARAMS,
    simple_sound.TESTS_DATA_PATH,
    simple_sound.TESTS_DATA,
    simple_sound.DEFAULT_EXPERIMENTS,
    simple_sound.DEVICE_STATE_SOUND_0)

test_data_simple_sound_5 = create_run_request_generator_fun(
    simple_sound.SCENARIO_NAME,
    simple_sound.SCENARIO_HANDLE,
    simple_sound.TEST_GEN_PARAMS,
    simple_sound.TESTS_DATA_PATH,
    simple_sound.TESTS_DATA,
    simple_sound.DEFAULT_EXPERIMENTS,
    simple_sound.DEVICE_STATE_SOUND_5)

test_data_simple_sound_10 = create_run_request_generator_fun(
    simple_sound.SCENARIO_NAME,
    simple_sound.SCENARIO_HANDLE,
    simple_sound.TEST_GEN_PARAMS,
    simple_sound.TESTS_DATA_PATH,
    simple_sound.TESTS_DATA,
    simple_sound.DEFAULT_EXPERIMENTS,
    simple_sound.DEVICE_STATE_SOUND_10)

test_data_simple_sound_music = create_run_request_generator_fun(
    simple_sound.SCENARIO_NAME,
    simple_sound.SCENARIO_HANDLE,
    simple_sound.TEST_GEN_PARAMS,
    simple_sound.TESTS_DATA_PATH,
    simple_sound.TESTS_DATA,
    simple_sound.DEFAULT_EXPERIMENTS,
    simple_sound.DEVICE_STATE_MUSIC)
