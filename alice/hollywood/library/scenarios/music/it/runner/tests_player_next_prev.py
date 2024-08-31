from alice.hollywood.library.scenarios.music.it.srcrwr_base import srcrwr_params
from alice.hollywood.library.scenarios.music.it.tests_player_prev_next import TEST_RUN_PARAMS, TESTS_DATA_PATH, SCENARIO_HANDLE,\
    hollywood
from alice.hollywood.library.python.testing.integration.conftest import create_test_function, create_bass_stubber_fixture

# To satisfy the linter
assert srcrwr_params
assert hollywood

bass_stubber = create_bass_stubber_fixture(TESTS_DATA_PATH)

test_player_prev_next = create_test_function(TESTS_DATA_PATH, TEST_RUN_PARAMS, SCENARIO_HANDLE, usefixtures=['srcrwr_params'])
