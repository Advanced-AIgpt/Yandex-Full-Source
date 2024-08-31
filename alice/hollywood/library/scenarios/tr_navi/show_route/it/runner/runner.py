from alice.hollywood.library.scenarios.tr_navi.show_route.it.test_cases import (
    TEST_RUN_PARAMS,
    TESTS_DATA_PATH,
    SCENARIO_HANDLE,
    bass_stubber,
    srcrwr_params,
    hollywood,
)
from alice.hollywood.library.python.testing.integration.conftest import create_test_function

# To satisfy the linter
assert bass_stubber
assert srcrwr_params
assert hollywood

test_run = create_test_function(
    TESTS_DATA_PATH,
    TEST_RUN_PARAMS,
    SCENARIO_HANDLE,
    usefixtures=['srcrwr_params'],
)
