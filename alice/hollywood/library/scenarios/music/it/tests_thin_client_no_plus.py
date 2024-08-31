import alice.hollywood.library.scenarios.music.it.thin_client_common as common

from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import voice
from alice.hollywood.library.python.testing.integration.conftest import create_oauth_token_fixture, create_hollywood_fixture
from alice.library.python.testing.auth import auth


TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/music/it/thin_client_no_plus/'

SCENARIO_NAME = common.SCENARIO_NAME
SCENARIO_HANDLE = common.SCENARIO_HANDLE
DEFAULT_APP_PRESETS = common.DEFAULT_APP_PRESETS
DEFAULT_EXPERIMENTS = common.DEFAULT_EXPERIMENTS
DEFAULT_SUPPORTED_FEATURES = common.DEFAULT_SUPPORTED_FEATURES

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# TODO(vitvlkv): This should be just an initial device state which should be MODIFIED after every dialog phrase
DEVICE_STATE_AUDIO_PLAYER = common.make_device_state_idle()

TESTS_DATA = {
    # TODO: Fix test when weekly promo suggestion works with thin client radio
    'play_radio_no_promo': {
        'input_dialog': [
            voice('включи грустную музыку'),
        ],
        'experiments': [
            'hw_music_thin_client',
            'hw_music_thin_client_playlist',
            'mm_enable_stack_engine',
            'music_suggest_weekly_promo',
            'music_check_plus_promo',
            'test_music_skip_plus_promo_check',
            'test_music_skip_weekly_promo_activation_check',
            'test_promo_mock_billing_requests',
        ],
        'app_presets': {
            'only': ['yandexmini']
        }
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)
TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

music_back_stubber = common.create_music_back_stubber_fixture(TESTS_DATA_PATH)
music_back_dl_info_stubber = common.create_music_back_dl_info_stubber_fixture(TESTS_DATA_PATH)
music_mds_stubber = common.create_music_mds_stubber_fixture(TESTS_DATA_PATH)

oauth_token_no_plus = create_oauth_token_fixture(auth.NoYaPlus)
