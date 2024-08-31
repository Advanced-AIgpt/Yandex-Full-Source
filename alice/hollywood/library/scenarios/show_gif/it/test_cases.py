from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/show_gif/it/data/'

SCENARIO_NAME = 'ShowGif'
SCENARIO_HANDLE = 'show_gif'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['quasar', 'search_app_prod', 'search_app_beta', 'search_app_ios']

DEFAULT_EXPERIMENTS = []

DEFAULT_DEVICE_STATE = {}

TESTS_DATA = {
    'show_gif': {
        'input_dialog': [
            text('покажи гифку')
        ],
    },
    'show_gif_text': {
        'input_dialog': [
            text('покажи гифку')
        ],
        'app_presets': {
            'only': ['search_app_prod']
        },
        'experiments': ['hw_debug_gif_to_text'],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
