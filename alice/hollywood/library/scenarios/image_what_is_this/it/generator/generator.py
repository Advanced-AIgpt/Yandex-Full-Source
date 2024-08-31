from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import create_run_request_generator_fun
from alice.hollywood.library.scenarios.image_what_is_this.it.test_cases import TEST_GEN_PARAMS, TESTS_DATA_PATH, TESTS_DATA, \
    SCENARIO_NAME, SCENARIO_HANDLE, DEFAULT_EXPERIMENTS, DEFAULT_ADDITIONAL_OPTIONS


test_run_request_generator = create_run_request_generator_fun(
    SCENARIO_NAME,
    SCENARIO_HANDLE,
    TEST_GEN_PARAMS,
    TESTS_DATA_PATH,
    TESTS_DATA,
    DEFAULT_EXPERIMENTS,
    usefixtures=['srcrwr_params'],
    default_additional_options=DEFAULT_ADDITIONAL_OPTIONS)
