from alice.hollywood.library.scenarios.music.it.srcrwr_thin import srcrwr_params
from alice.hollywood.library.scenarios.music.it.tests_thin_client_no_plus import music_back_stubber, \
    music_back_dl_info_stubber, music_mds_stubber, oauth_token_no_plus, \
    SCENARIO_NAME, SCENARIO_HANDLE, TEST_GEN_PARAMS, TESTS_DATA_PATH, TESTS_DATA, \
    DEFAULT_EXPERIMENTS, DEFAULT_SUPPORTED_FEATURES, DEVICE_STATE_AUDIO_PLAYER
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import create_run_request_generator_fun
from alice.hollywood.library.python.testing.integration.conftest import create_localhost_bass_stubber_fixture

# To satisfy the linter
assert music_back_stubber
assert music_back_dl_info_stubber
assert music_mds_stubber
assert srcrwr_params
assert oauth_token_no_plus

bass_stubber = create_localhost_bass_stubber_fixture(TESTS_DATA_PATH, stubs_subdir='bass')

test = create_run_request_generator_fun(
    SCENARIO_NAME,
    SCENARIO_HANDLE,
    TEST_GEN_PARAMS,
    TESTS_DATA_PATH,
    TESTS_DATA,
    DEFAULT_EXPERIMENTS,
    default_supported_features=DEFAULT_SUPPORTED_FEATURES,
    use_oauth_token_fixture='oauth_token_no_plus',
    usefixtures=['srcrwr_params'],
    default_device_state=DEVICE_STATE_AUDIO_PLAYER)
