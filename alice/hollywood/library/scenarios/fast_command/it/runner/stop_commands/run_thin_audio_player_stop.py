from alice.hollywood.library.scenarios.fast_command.it.stop_commands.thin_audio_player_stop import\
    TEST_RUN_PARAMS, TESTS_DATA_PATH, SCENARIO_HANDLE, hollywood
from alice.hollywood.library.python.testing.integration.conftest import create_test_function

assert hollywood  # To satisfy the linter

test_run = create_test_function(TESTS_DATA_PATH, TEST_RUN_PARAMS, SCENARIO_HANDLE)
