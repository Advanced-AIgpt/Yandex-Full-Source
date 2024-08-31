from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/tv_channels/it/data/'

SCENARIO_NAME = 'TvChannels'
SCENARIO_HANDLE = 'tv_channels'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

DEFAULT_APP_PRESETS = ['tv']

DEFAULT_EXPERIMENTS = [
    'mm_enable_protocol_scenario=TvChannels',
]

DEFAULT_DEVICE_STATE = {
    'device_id': '0ff3e9b9174170315feaa4e2be18c883'
}

TESTS_DATA = {
    'switch_channel_zvezda': {
        'input_dialog': [
            text('переключи на канал звезда')
        ],
        'supported_features' : ['live_tv_scheme']
    },
    'switch_channel_mir24': {
        'input_dialog': [
            text('переключи на канал мир 24')
        ],
        'supported_features' : ['live_tv_scheme']
    },
    'switch_unknown_tv_channel': {
        'input_dialog': [
            text('переключи на нтв плюс')
        ],
        'supported_features' : ['live_tv_scheme']
    },
    'switch_live_tv_unavailable': {
        'input_dialog': [
            text('переключи на канал звезда')
        ],
        'supported_features' : []
    }
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
