from alice.hollywood.library.scenarios.draw_picture.it.test_cases import TEST_RUN_PARAMS, TESTS_DATA_PATH, SCENARIO_HANDLE, \
    TESTS_DATA, SCENARIO_NAME, srcrwr_params, cbir_features_stubber, hollywood
from alice.hollywood.library.python.testing.integration.conftest import create_test_function

assert srcrwr_params  # To satisfy the linter
assert cbir_features_stubber  # To satisfy the linter
assert hollywood  # To satisfy the linter

test_run = create_test_function(TESTS_DATA_PATH, TEST_RUN_PARAMS, SCENARIO_HANDLE, tests_data=TESTS_DATA,
                                scenario_name=SCENARIO_NAME, usefixtures=['srcrwr_params'])
