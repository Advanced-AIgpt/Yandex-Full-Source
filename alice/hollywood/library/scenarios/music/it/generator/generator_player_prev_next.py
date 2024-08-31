import alice.hollywood.library.scenarios.music.it.tests_player_prev_next as tests_player_prev_next
from alice.hollywood.library.scenarios.music.it.srcrwr_base import srcrwr_params
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import create_run_request_generator_fun
from alice.hollywood.library.python.testing.integration.conftest import create_localhost_bass_stubber_fixture

assert srcrwr_params

bass_stubber = create_localhost_bass_stubber_fixture(tests_player_prev_next.TESTS_DATA_PATH)

test = create_run_request_generator_fun(
    tests_player_prev_next.SCENARIO_NAME,
    tests_player_prev_next.SCENARIO_HANDLE,
    tests_player_prev_next.TEST_GEN_PARAMS,
    tests_player_prev_next.TESTS_DATA_PATH,
    tests_player_prev_next.TESTS_DATA,
    tests_player_prev_next.DEFAULT_EXPERIMENTS,
    tests_player_prev_next.DEFAULT_DEVICE_STATE,
    usefixtures=['srcrwr_params'])
