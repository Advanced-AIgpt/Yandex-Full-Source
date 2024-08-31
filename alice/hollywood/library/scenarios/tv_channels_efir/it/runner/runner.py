from alice.hollywood.library.python.testing.integration.conftest import create_test_function
from alice.hollywood.library.scenarios.tv_channels_efir.it.conftest import srcrwr_params
from alice.hollywood.library.scenarios.tv_channels_efir.it.test_cases import\
    TEST_RUN_PARAMS, TESTS_DATA_PATH, SCENARIO_HANDLE, hollywood

assert srcrwr_params  # To satisfy the linter
assert hollywood  # To satisfy the linter

test_run = create_test_function(TESTS_DATA_PATH, TEST_RUN_PARAMS, SCENARIO_HANDLE,
                                usefixtures=['srcrwr_params'])
