import alice.hollywood.library.scenarios.music.it.tests_data_hardcoded_music as tests_data
from alice.hollywood.library.scenarios.music.it.srcrwr_hardcoded import srcrwr_params
from alice.hollywood.library.scenarios.music.it.tests_data_hardcoded_music import oauth_token_plus
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import create_run_request_generator_fun
from alice.hollywood.library.python.testing.integration.conftest import (
    create_localhost_bass_stubber_fixture, create_stubber_fixture, StubberEndpoint
)

# To satisfy the linter
assert srcrwr_params
assert oauth_token_plus

bass_stubber = create_localhost_bass_stubber_fixture(tests_data.TESTS_DATA_PATH)

datasync_stubber = create_stubber_fixture(
    tests_data.TESTS_DATA_PATH,
    'api-mimino.dst.yandex.net',
    8080,
    [
        StubberEndpoint('/v1/personality/profile/alisa/kv/morning_show', ['GET', 'PUT'], idempotent=False),
    ],
    stubs_subdir='datasync_stubs',
)

test = create_run_request_generator_fun(
    tests_data.SCENARIO_NAME,
    tests_data.SCENARIO_HANDLE,
    tests_data.TEST_GEN_PARAMS,
    tests_data.TESTS_DATA_PATH,
    tests_data.TESTS_DATA,
    tests_data.DEFAULT_EXPERIMENTS,
    use_oauth_token_fixture='oauth_token_plus',
    usefixtures=['srcrwr_params'])