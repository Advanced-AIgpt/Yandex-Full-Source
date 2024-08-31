from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import create_run_request_generator_fun
from alice.hollywood.library.scenarios.tr_navi.show_route.it.test_cases import TEST_GEN_PARAMS, TESTS_DATA_PATH, TESTS_DATA, \
    DEFAULT_EXPERIMENTS, SCENARIO_NAME, SCENARIO_HANDLE, bass_stubber, srcrwr_params

# To satisfy the linter
assert bass_stubber
assert srcrwr_params

test_run_request_generator = create_run_request_generator_fun(
    SCENARIO_NAME,
    SCENARIO_HANDLE,
    TEST_GEN_PARAMS,
    TESTS_DATA_PATH,
    TESTS_DATA,
    DEFAULT_EXPERIMENTS,
    usefixtures=['srcrwr_params'],
    lang='tr-TR')
