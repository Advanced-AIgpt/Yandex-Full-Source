from alice.hollywood.library.scenarios.transform_face.it.test_cases import TEST_RUN_PARAMS, TESTS_DATA_PATH, SCENARIO_HANDLE, \
    TESTS_DATA, SCENARIO_NAME, srcrwr_params, cbir_features_stubber, hollywood
from alice.hollywood.library.python.testing.integration.conftest import create_test_function

assert srcrwr_params
assert cbir_features_stubber
assert hollywood

test_run = create_test_function(TESTS_DATA_PATH, TEST_RUN_PARAMS, SCENARIO_HANDLE, tests_data=TESTS_DATA,
                                scenario_name=SCENARIO_NAME, usefixtures=['srcrwr_params'])
