from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text


TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/happy_new_year/it/data/'

SCENARIO_NAME = 'HappyNewYear'
SCENARIO_HANDLE = 'happy_new_year'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['search_app_prod']

DEFAULT_EXPERIMENTS = []

DEFAULT_DEVICE_STATE = {}  # NOTE: you may pass some device_state to the `create_run_request_generator_fun`

TESTS_DATA = {
    'test_happy_new_year': {
        'input_dialog': [
            text('сделай для меня открытку'),
        ],
        'app_presets': {
            'only': ['launcher', 'search_app_prod'],
        },
    },
    'test_happy_new_year_yabro': {
        'input_dialog': [
            text('сделай для меня открытку'),
        ],
        'app_presets': {
            'only': ['yabro_prod'],
        },
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
