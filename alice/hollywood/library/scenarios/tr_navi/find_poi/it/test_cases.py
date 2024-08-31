import pytest

from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.hollywood.library.python.testing.integration.conftest import create_bass_stubber_fixture, create_hollywood_fixture

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/tr_navi/find_poi/it/data/'

SCENARIO_NAME = 'FindPoiTr'
SCENARIO_HANDLE = 'find_poi_tr'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['navigator']

DEFAULT_EXPERIMENTS = [
    'mm_enable_protocol_scenario=FindPoiTr',
]

DEFAULT_DEVICE_STATE = {}

TESTS_DATA = {
    'find_poi': {
        'input_dialog': [
            text('tercüman sitesi zeytinburnu')
        ],
    },
    'find_poi_with_building': {
        'input_dialog': [
            text('uhuvvet sokak no 10')
        ],
    },
    'find_poi_with_building_unnormalized': {
        'input_dialog': [
            text('uhuvvet sokak no on')
        ],
    },
    'ara_queries': {
        'input_dialog': [
            text('19 mayıs caddesini ara'),
            text('bir şey ara')
        ],
    },
    'where_slot': {
        'input_dialog': [
            text('en yakın metro istasyonu bulabilir misin')
        ]
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

bass_stubber = create_bass_stubber_fixture(TESTS_DATA_PATH)


@pytest.fixture(scope="function")
def srcrwr_params(bass_stubber):
    return {
        'FIND_POI_TR_PROXY': f'localhost:{bass_stubber.port}',
    }
