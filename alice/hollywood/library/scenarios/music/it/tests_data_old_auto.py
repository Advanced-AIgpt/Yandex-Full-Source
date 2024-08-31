from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.hollywood.library.python.testing.integration.conftest import create_oauth_token_fixture, create_hollywood_fixture
from alice.library.python.testing.auth import auth


TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/music/it/data_auto_old/'

SCENARIO_NAME = 'HollywoodMusic'
SCENARIO_HANDLE = 'music'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['auto_old']

DEFAULT_EXPERIMENTS = [
    'mm_music_play_confidence_threshold=0',
]

TESTS_DATA = {
    # bass provider: yaradio
    'play_music': {
        'input_dialog': [
            text('включи музыку'),
        ],
    },

    # bass provider: yamusic
    # should be a soft error
    'play_queen_soft_error': {
        'input_dialog': [
            text('включи queen'),
        ],
    },

    # bass provider: yaradio
    # should be a soft error
    'play_rock_soft_error': {
        'input_dialog': [
            text('включи рок'),
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

oauth_token_plus = create_oauth_token_fixture(auth.YaPlusMusicLikes)
