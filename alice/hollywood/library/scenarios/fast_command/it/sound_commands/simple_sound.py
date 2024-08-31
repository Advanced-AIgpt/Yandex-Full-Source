from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/fast_command/it/data/'

SCENARIO_NAME = 'Commands'
SCENARIO_HANDLE = 'fast_command'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

DEFAULT_APP_PRESETS = [
    'search_app_prod',
    'navigator',
    'quasar',
    'auto',
    'elariwatch',
    'auto_old'
]

DEFAULT_EXPERIMENTS = [
    'enable_sound_in_hollywood_commands'
]

DEVICE_STATE_SOUND_5 = {
    "sound_level": 5
}

DEVICE_STATE_SOUND_0 = {
    "sound_level": 0
}

DEVICE_STATE_SOUND_10 = {
    "sound_level": 10
}

DEVICE_STATE_MUSIC = {
    "music": {
        "player": {
            "pause": False
        }
    }
}

TESTS_DATA = {
    'simple_louder': {
        'input_dialog': [
            text('громче')
        ],
    },
    'simple_quiter': {
        'input_dialog': [
            text('тише')
        ],
    },
    'simple_mute': {
        'input_dialog': [
            text('выключи звук')
        ],
    },
    'simple_unmute': {
        'input_dialog': [
            text('включи звук')
        ],
    },
    'simple_set': {
        'input_dialog': [
            text('поставь громкость 7')
        ],
    },
    'set_out_borders': {
        'input_dialog': [
            text('поставь громкость 2020')
        ],
    },
    'set_zero': {
        'input_dialog': [
            text('поставь громкость 0')
        ],
    },
    'set_settings': {
        'input_dialog': [
            text('поставь среднюю громкость')
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
