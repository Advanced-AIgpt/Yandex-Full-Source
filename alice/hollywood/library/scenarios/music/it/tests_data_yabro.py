from alice.hollywood.library.python.testing.integration.conftest import create_oauth_token_fixture, create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.library.python.testing.auth import auth

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/music/it/data_yabro/'

SCENARIO_NAME = 'HollywoodMusic'
SCENARIO_HANDLE = 'music'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['browser_prod']

DEFAULT_EXPERIMENTS = []

TESTS_DATA = {
    # bass provider: yamusic
    # in slots: search_text
    # out slots: search_text + answer.albom + answer.artist + answer.type==track
    'play_song_taet_led': {
        'input_dialog': [
            text('включи песню тает лед'),
        ],
    },

    # bass provider: yamusic
    # in slots: search_text
    # out slots: search_text + answer.artists + answer.type==albom
    'play_albom_dark_side_of_the_moon': {
        'input_dialog': [
            text('включи альбом dark side of the moon'),
        ],
    },

    # bass provider: yaradio
    # in slots: activity==run
    # out slots: activity==run + answer.type=playlist
    'play_music_for_jogging': {
        'experiments': ['new_music_radio_nlg'],
        'input_dialog': [
            text('включи музыку для бега'),
        ],
    },

    # bass provider: yamusic
    # in slots: novelty==new
    # out slots: novelty==new + answer.type=playlist
    'play_novelties': {
        'input_dialog': [
            text('включи новинки'),
        ],
    },

    # bass provider: yamusic
    # in slots: playlist='подборку queen'
    # bass out slots: playlist='подборку queen' + answer.type==playlist
    'play_playlist_queen': {
        'input_dialog': [
            text('включи подборку queen'),
        ],
    },

    # bass provider: yamusic
    # in slots: search_text
    # out slots: search_text + answer.type==artist
    'play_queen': {
        'input_dialog': [
            text('включи queen'),
        ],
    },

    # bass provider: yamusic
    # in slots: search_text + mood==sad
    # out slots: search_text + mood==sad + answer.type==artist
    'play_queens_sad_songs': {
        'input_dialog': [
            text('включи грустные песни queen'),
        ],
    },

    # bass provider: yaradio
    # in slots: genre==rock
    # out slots: genre==rock + answer.type==playlist
    'play_rock': {
        'input_dialog': [
            text('включи рок'),
        ],
    },

    # bass provider: yamusic
    # in slots: genre==jazz + mood==happy
    # out slots: genre==jazz + mood==happy + answer.type==playlist
    'play_happy_jazz': {
        'input_dialog': [
            text('включи веселый джаз'),
        ],
    },

    # bass provider: yaradio
    # in slots: mood==sad
    # out slots: mood==sad + answer.type==playlist
    'play_sad_music': {
        'input_dialog': [
            text('включи грустную музыку'),
        ],
    },

    # bass provider: yaradio
    # in slots: none
    # out slots: none
    'recommend_me_music': {
        'input_dialog': [
            text('порекомендуй мне музыку'),
        ],
    },

    # bass provider: yaradio
    # in slots: personality==is_personal
    # bass out slots: personality=is_personal
    'play_my_music': {
        'input_dialog': [
            text('включи мою музыку'),
        ],
    },

    'playlist_of_the_day': {
        'input_dialog': [
            text('включи плейлист дня'),
        ],
    },

    'playlist_with_alice_shots': {
        'input_dialog': [
            text('включи плейлист с твоими шотами'),
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

oauth_token_no_plus = create_oauth_token_fixture(auth.NoYaPlus)
