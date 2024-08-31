import alice.hollywood.library.scenarios.music.it.thin_client_common as common

from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import voice
from alice.hollywood.library.python.testing.integration.conftest import create_oauth_token_fixture, create_hollywood_fixture
from alice.library.python.testing.auth import auth


TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/music/it/thin_client_no_audio_client/'

SCENARIO_NAME = common.SCENARIO_NAME
SCENARIO_HANDLE = common.SCENARIO_HANDLE
DEFAULT_APP_PRESETS = common.DEFAULT_APP_PRESETS
DEFAULT_EXPERIMENTS = common.DEFAULT_EXPERIMENTS

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

DEVICE_STATE_AUDIO_PLAYER = common.make_device_state_idle()

TESTS_DATA = {
    'play_artist': {
        'input_dialog': [
            voice('включи queen'),
        ]
    },
    'play_track': {
        'input_dialog': [
            voice('включи песню sia cheap thrills'),
        ]
    },
    'play_album': {
        'input_dialog': [
            voice('включи альбом dark side of the moon'),
        ]
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)
TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

music_back_stubber = common.create_music_back_stubber_fixture(TESTS_DATA_PATH)
music_back_dl_info_stubber = common.create_music_back_dl_info_stubber_fixture(TESTS_DATA_PATH)
music_mds_stubber = common.create_music_mds_stubber_fixture(TESTS_DATA_PATH)

oauth_token_plus = create_oauth_token_fixture(auth.YaPlusMusicLikes)
