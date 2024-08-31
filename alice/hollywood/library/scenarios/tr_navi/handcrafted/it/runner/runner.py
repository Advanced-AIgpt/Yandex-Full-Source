from alice.hollywood.library.scenarios.tr_navi.handcrafted.it.test_cases import (
    TEST_RUN_PARAMS,
    TESTS_DATA_PATH,
    SCENARIO_HANDLE,
    hollywood
)
from alice.hollywood.library.python.testing.integration.conftest import create_test_function

assert hollywood  # To satisfy the linter

test_run = create_test_function(
    TESTS_DATA_PATH,
    TEST_RUN_PARAMS,
    SCENARIO_HANDLE,
)
