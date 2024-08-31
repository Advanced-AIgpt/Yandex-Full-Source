import pytest

from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.hollywood.library.python.testing.integration.conftest import create_bass_stubber_fixture, create_hollywood_fixture

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/tr_navi/get_weather/it/data/'

SCENARIO_NAME = 'GetWeatherTr'
SCENARIO_HANDLE = 'get_weather_tr'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['navigator']

DEFAULT_EXPERIMENTS = [
    'mm_enable_protocol_scenario=GetWeatherTr',
]

DEFAULT_DEVICE_STATE = {}

TESTS_DATA = {
    'weather_simple': {
        'input_dialog': [
            text('hava durumu nasıl')
        ],
    },
    'weather_tomorrow': {
        'input_dialog': [
            text('yarın için hava durumunu söyle')
        ],
    },
    'weather_weekend': {
        'input_dialog': [
            text('haftasonu hava nasıl olacak')
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

bass_stubber = create_bass_stubber_fixture(TESTS_DATA_PATH)


@pytest.fixture(scope="function")
def srcrwr_params(bass_stubber):
    return {
        'GET_WEATHER_TR_PROXY': f'localhost:{bass_stubber.port}',
    }
