from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/fast_command/it/data/'

SCENARIO_NAME = 'Commands'
SCENARIO_HANDLE = 'fast_command'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

DEFAULT_APP_PRESETS = [
    'quasar',
    'yandexmini',
    'small_smart_speakers'
]

DEFAULT_EXPERIMENTS = []

DEFAULT_DEVICE_STATE = {
    "alarm_state": {
        "currently_playing": True
    }
}

TESTS_DATA = {
    'alarm_stop': {
        'input_dialog': [
            text('хватит')
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
