import alice.hollywood.library.scenarios.music.it.tests_player_shuffle as tests_player_shuffle
from alice.hollywood.library.scenarios.music.it.srcrwr_base import srcrwr_params
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import create_run_request_generator_fun
from alice.hollywood.library.python.testing.integration.conftest import create_localhost_bass_stubber_fixture

assert srcrwr_params

bass_stubber = create_localhost_bass_stubber_fixture(tests_player_shuffle.TESTS_DATA_PATH)

test = create_run_request_generator_fun(
    tests_player_shuffle.SCENARIO_NAME,
    tests_player_shuffle.SCENARIO_HANDLE,
    tests_player_shuffle.TEST_GEN_PARAMS,
    tests_player_shuffle.TESTS_DATA_PATH,
    tests_player_shuffle.TESTS_DATA,
    tests_player_shuffle.DEFAULT_EXPERIMENTS,
    tests_player_shuffle.DEFAULT_DEVICE_STATE,
    usefixtures=['srcrwr_params'])
