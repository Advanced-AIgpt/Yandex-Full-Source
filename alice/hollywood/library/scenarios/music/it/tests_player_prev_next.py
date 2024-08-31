from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text


TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/music/it/data_player_prev_next/'

SCENARIO_NAME = 'HollywoodMusic'
SCENARIO_HANDLE = 'music'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

DEFAULT_APP_PRESETS = [
    'quasar',
    'yandexmini',
    'small_smart_speakers',
    'search_app_prod',
    'navigator',
    'auto',
    'elariwatch'
]

DEFAULT_EXPERIMENTS = [
    'internal_music_player',
    'enable_player_in_hw_music',
    'mm_enable_hollywood_music_for_searchapp',
    'mm_music_play_confidence_threshold=0',
    'enable_prev_next_in_hw_music'
]

DEFAULT_DEVICE_STATE = {
    'music': {
        'player': {
            'pause': False
        }
    }
}

TESTS_DATA = {
    'next_track': {
        'input_dialog': [
            text('следующая песня')
        ],
    },
    'prev_track': {
        'input_dialog': [
            text('предыдущая песня')
        ],
    },
    'next_nothing_playing': {
        'input_dialog': [
            text('дальше', device_state={
                'video': {
                    'current_screen': 'main',
                },
                'music': {
                    'player': {
                        'pause': True
                    },
                },
            }),
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
