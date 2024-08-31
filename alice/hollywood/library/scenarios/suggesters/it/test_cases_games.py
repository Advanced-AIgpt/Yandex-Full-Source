from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/suggesters/it/data_games/'

SCENARIO_NAME = 'GameSuggest'
SCENARIO_HANDLE = 'game_suggest'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['quasar']

DEFAULT_EXPERIMENTS = []

DEFAULT_DEVICE_STATE = {}

TESTS_DATA = {
    'accept_forth_game': {
        'input_dialog': [
            text('посоветуй игру'),
            text('не хочу такую'),
            text('давай другие'),
            text('следующая'),
            text('давай'),
        ],
    },
    'ignore_onboarding_frame_without_flag': {
        'input_dialog': [
            text('давай поиграем'),
        ]
    },
    'process_onboarding_frame_with_flag': {
        'input_dialog': [
            text('давай поиграем'),
        ],
        'experiments': ['hw_game_suggest_use_onboarding_frame'],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
