from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import \
    create_run_request_generator_fun
from alice.hollywood.library.scenarios.tv_channels.it.test_cases import TEST_GEN_PARAMS, TESTS_DATA_PATH, TESTS_DATA, \
    DEFAULT_EXPERIMENTS, SCENARIO_NAME, SCENARIO_HANDLE, DEFAULT_DEVICE_STATE

test_run_request_generator = create_run_request_generator_fun(
    SCENARIO_NAME,
    SCENARIO_HANDLE,
    TEST_GEN_PARAMS,
    TESTS_DATA_PATH,
    TESTS_DATA,
    DEFAULT_EXPERIMENTS,
    DEFAULT_DEVICE_STATE,
    usefixtures=['srcrwr_params'])
