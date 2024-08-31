import alice.hollywood.library.scenarios.music.it.tests_data as tests_data
from alice.hollywood.library.scenarios.music.it.srcrwr_base import srcrwr_params
from alice.hollywood.library.scenarios.music.it.tests_data import oauth_token_plus
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import create_run_request_generator_fun
from alice.hollywood.library.python.testing.integration.conftest import create_localhost_bass_stubber_fixture

# To satisfy the linter
assert srcrwr_params
assert oauth_token_plus

bass_stubber = create_localhost_bass_stubber_fixture(tests_data.TESTS_DATA_PATH)

test = create_run_request_generator_fun(
    tests_data.SCENARIO_NAME,
    tests_data.SCENARIO_HANDLE,
    tests_data.TEST_GEN_PARAMS,
    tests_data.TESTS_DATA_PATH,
    tests_data.TESTS_DATA,
    tests_data.DEFAULT_EXPERIMENTS,
    use_oauth_token_fixture='oauth_token_plus',
    usefixtures=['srcrwr_params'])
