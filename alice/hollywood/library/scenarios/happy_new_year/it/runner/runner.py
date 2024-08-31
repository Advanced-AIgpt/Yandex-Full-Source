from alice.hollywood.library.scenarios.happy_new_year.it.test_cases import TEST_RUN_PARAMS, TESTS_DATA_PATH, SCENARIO_HANDLE, \
    TESTS_DATA, SCENARIO_NAME, hollywood
from alice.hollywood.library.python.testing.integration.conftest import create_test_function

assert hollywood  # To satisfy the linter

test_run = create_test_function(TESTS_DATA_PATH, TEST_RUN_PARAMS, SCENARIO_HANDLE, tests_data=TESTS_DATA,
                                scenario_name=SCENARIO_NAME)
