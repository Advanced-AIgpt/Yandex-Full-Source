from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/taximeter/it/data/'

SCENARIO_NAME = 'Taximeter'
SCENARIO_HANDLE = 'taximeter'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['taximeter']

DEFAULT_EXPERIMENTS = []

TESTS_DATA = {
    'accept1': {
        'input_dialog': [
            text('да беру'),
        ],
    },
    'accept2': {
        'input_dialog': [
            text('давай'),
        ],
    },
    'deny1': {
        'input_dialog': [
            text('не надо'),
        ],
    },
    'deny2': {
        'input_dialog': [
            text('пропустить'),
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
