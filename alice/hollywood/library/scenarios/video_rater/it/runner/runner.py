from alice.hollywood.library.python.testing.integration.conftest import create_test_function
from alice.hollywood.library.scenarios.video_rater.it.conftest import srcrwr_params
from alice.hollywood.library.scenarios.video_rater.it.test_cases import\
    TEST_RUN_PARAMS, TESTS_DATA_PATH, SCENARIO_HANDLE, hollywood
from alice.hollywood.library.scenarios.video_rater.proto.video_rater_state_pb2 import TVideoRaterState

assert srcrwr_params  # To satisfy the linter
assert TVideoRaterState  # To satisfy the linter
assert hollywood  # To satisfy the linter

test_run = create_test_function(TESTS_DATA_PATH, TEST_RUN_PARAMS, SCENARIO_HANDLE,
                                usefixtures=['srcrwr_params'])
