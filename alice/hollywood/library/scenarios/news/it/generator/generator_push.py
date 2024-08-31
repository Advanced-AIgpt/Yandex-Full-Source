import alice.hollywood.library.scenarios.news.it.tests_data_push as tests_data
from alice.hollywood.library.python.testing.integration.conftest import create_localhost_bass_stubber_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import create_run_request_generator_fun
from alice.hollywood.library.scenarios.news.it.tests_data_push import srcrwr_params

# To satisfy the linter
assert srcrwr_params

bass_stubber = create_localhost_bass_stubber_fixture(tests_data.TESTS_DATA_PATH)

test_data_quasar = create_run_request_generator_fun(
    tests_data.SCENARIO_NAME,
    tests_data.SCENARIO_HANDLE,
    tests_data.TEST_GEN_PARAMS,
    tests_data.TESTS_DATA_PATH,
    tests_data.TESTS_DATA,
    tests_data.DEFAULT_EXPERIMENTS,
    usefixtures=['srcrwr_params'])
