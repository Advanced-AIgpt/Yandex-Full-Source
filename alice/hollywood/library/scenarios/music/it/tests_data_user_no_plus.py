from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.hollywood.library.python.testing.integration.conftest import create_oauth_token_fixture, create_hollywood_fixture
from alice.library.python.testing.auth import auth


TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/music/it/data_user_no_plus/'

SCENARIO_NAME = 'HollywoodMusic'
SCENARIO_HANDLE = 'music'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = []

DEFAULT_EXPERIMENTS = []

TESTS_DATA = {
    'play_queen_internal_player': {
        'input_dialog': [
            text('включи queen'),
        ],
        'app_presets': {
            'only': ['quasar']
        },
        'experiments': ['internal_music_player', 'mm_enable_hollywood_music_for_searchapp']
    },

    'play_queen': {
        'input_dialog': [
            text('включи queen'),
        ],
        'app_presets': {
            'only': ['browser_prod']
        },
    },

    'recommend_me_music_internal_player': {
        'input_dialog': [
            text('порекомендуй мне музыку'),
        ],
        'app_presets': {
            'only': ['quasar']
        },
        'experiments': ['internal_music_player', 'mm_enable_hollywood_music_for_searchapp']
    },

    'recommend_me_music': {
        'input_dialog': [
            text('порекомендуй мне музыку'),
        ],
        'app_presets': {
            'only': ['browser_prod']
        },
    },

    'play_my_music_internal_player': {
        'input_dialog': [
            text('включи мою музыку'),
        ],
        'app_presets': {
            'only': ['quasar']
        },
        'experiments': ['internal_music_player', 'mm_enable_hollywood_music_for_searchapp']
    },

    'play_my_music': {
        'input_dialog': [
            text('включи мою музыку'),
        ],
        'app_presets': {
            'only': ['browser_prod']
        },
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

oauth_token_no_plus = create_oauth_token_fixture(auth.NoYaPlus)
