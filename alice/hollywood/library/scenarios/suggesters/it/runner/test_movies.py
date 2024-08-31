from alice.hollywood.library.python.testing.integration.conftest import create_test_function
from alice.hollywood.library.scenarios.suggesters.it.test_cases_movies import TESTS_DATA_PATH,\
    TEST_RUN_PARAMS, SCENARIO_HANDLE, hollywood

assert hollywood  # To satisfy the linter

test_movies = create_test_function(TESTS_DATA_PATH, TEST_RUN_PARAMS, SCENARIO_HANDLE)
