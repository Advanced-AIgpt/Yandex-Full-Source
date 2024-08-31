import pytest

from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.hollywood.library.python.testing.integration.conftest import create_bass_stubber_fixture, create_hollywood_fixture

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/tr_navi/switch_layer/it/data/'

SCENARIO_NAME = 'SwitchLayerTr'
SCENARIO_HANDLE = 'switch_layer_tr'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['navigator']

DEFAULT_EXPERIMENTS = [
    'mm_enable_protocol_scenario=SwitchLayerTr',
    'bg_fresh_granet_form=alice.navi.switch_layer',
]

DEFAULT_DEVICE_STATE = {}

TESTS_DATA = {
    'show_traffic': {
        'input_dialog': [
            text('alisa sıkışıklığı'),  # Показать слой с дорожным трафиком
        ],
    },
    'hide_traffic': {
        'input_dialog': [
            text('alisa yoğunluğunu kapatır mısın'),  # Скрыть слой с дорожным трафиком
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

bass_stubber = create_bass_stubber_fixture(TESTS_DATA_PATH)


@pytest.fixture(scope="function")
def srcrwr_params(bass_stubber):
    return {
        'SWITCH_LAYER_TR_PROXY': f'localhost:{bass_stubber.port}',
    }
