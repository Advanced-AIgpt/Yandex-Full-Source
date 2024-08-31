import alice.hollywood.library.scenarios.music.it.thin_client_common as common
from alice.hollywood.library.scenarios.music.it.thin_client_common import make_device_state_idle, \
    make_device_state_playing, make_device_state_stopped

from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import voice, Scenario
from alice.hollywood.library.python.testing.integration.conftest import create_oauth_token_fixture, create_hollywood_fixture
from alice.library.python.testing.auth import auth


TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/music/it/thin_client/'

SCENARIO_NAME = common.SCENARIO_NAME
SCENARIO_HANDLE = common.SCENARIO_HANDLE
DEFAULT_APP_PRESETS = common.DEFAULT_APP_PRESETS
DEFAULT_EXPERIMENTS = common.DEFAULT_EXPERIMENTS
DEFAULT_SUPPORTED_FEATURES = common.DEFAULT_SUPPORTED_FEATURES

hollywood = create_hollywood_fixture([SCENARIO_HANDLE, 'fast_command'])

# TODO(vitvlkv): This should be just an initial device state which should be MODIFIED after every dialog phrase
DEVICE_STATE_AUDIO_PLAYER = make_device_state_idle()

TESTS_DATA = {
    'play_artist_next_track_not_owner': {
        'input_dialog': [
            voice('включи queen'),
            # Now another scenario takes over the audio player...
            voice('следующий трек', device_state=make_device_state_playing(owner='not_music', stream_id='FooBar123456')),
        ],
    },
    'play_artist_pause_continue_not_owner': {
        'input_dialog': [
            voice('включи queen'),
            voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command'),
                  device_state=make_device_state_playing()),
            # Now another scenario takes over the audio player...
            voice('продолжи', device_state=make_device_state_stopped(owner='not_music', stream_id='FooBar123456')),
        ],
    },
    'play_artist_like_not_owner': {
        'input_dialog': [
            voice('включи queen'),
            # Now another scenario takes over the audio player...
            voice('поставь лайк', device_state=make_device_state_playing(owner='not_music')),
        ],
    },
    'play_artist_hollywood_189': {
        'input_dialog': [
            voice('включи mark knopfler')
        ]
    },
    'play_track_that_cannot_be_found': {
        'input_dialog': [
            voice('алиса включи крошка моя я по тебе скучаю')
        ]
    },
    'play_fixlist_track': {
        'input_dialog': [
            voice('включи фикслист для тестов это не должны спросить в проде кейс трек по запросу')
        ]
    },
    'play_fixlist_search_text': {
        'input_dialog': [
            voice('включи арзамас')
        ]
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)
TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

music_back_stubber = common.create_music_back_stubber_fixture(TESTS_DATA_PATH)
music_back_dl_info_stubber = common.create_music_back_dl_info_stubber_fixture(TESTS_DATA_PATH)
music_mds_stubber = common.create_music_mds_stubber_fixture(TESTS_DATA_PATH)

oauth_token_plus = create_oauth_token_fixture(auth.YaPlusMusicLikes)
