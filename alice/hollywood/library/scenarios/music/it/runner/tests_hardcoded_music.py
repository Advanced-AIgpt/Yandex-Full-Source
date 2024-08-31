from alice.hollywood.library.scenarios.music.it.srcrwr_hardcoded import srcrwr_params
from alice.hollywood.library.scenarios.music.it.tests_data_hardcoded_music import TEST_RUN_PARAMS, TESTS_DATA_PATH, \
    SCENARIO_HANDLE, hollywood
from alice.hollywood.library.python.testing.integration.conftest import (
    create_test_function, create_bass_stubber_fixture, create_stubber_fixture, StubberEndpoint
)

# To satisfy the linter
assert srcrwr_params
assert hollywood

bass_stubber = create_bass_stubber_fixture(TESTS_DATA_PATH)

datasync_stubber = create_stubber_fixture(
    TESTS_DATA_PATH,
    'api-mimino.dst.yandex.net',
    8080,
    [
        StubberEndpoint('/v1/personality/profile/alisa/kv/morning_show', ['GET', 'PUT'], idempotent=False),
    ],
    stubs_subdir='datasync_stubs',
)

test_hardcoded_music = create_test_function(
    TESTS_DATA_PATH,
    TEST_RUN_PARAMS,
    SCENARIO_HANDLE,
    usefixtures=['srcrwr_params'],
)
