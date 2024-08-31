import pytest

from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.hollywood.library.python.testing.integration.conftest import create_bass_stubber_fixture, create_hollywood_fixture

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/tr_navi/add_point/it/data/'

SCENARIO_NAME = 'AddPointTr'
SCENARIO_HANDLE = 'add_point_tr'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['navigator']

DEFAULT_EXPERIMENTS = [
    'mm_enable_protocol_scenario=AddPointTr',
]

DEFAULT_DEVICE_STATE = {}

TESTS_DATA = {
    'accident_right_lane': {
        'input_dialog': [
            text('sağ şerit trafik kazası yaralı var'),  # ДТП на правой полосе, есть пострадавшие
        ],
    },
    'road_works_all_lanes': {
        'input_dialog': [
            text('çift şerit yol çalışması'),  # Дорожные работы на всех полосах
        ],
    },
    'geo_location': {
        'input_dialog': [
            text('boğaziçi köprüsü kapalı avrasya maratonu'),  # Евразийский марафон с Босфорского моста
        ],
    },
    'camera': {
        'input_dialog': [
            text('Yolda kamera'),  # На дороге камера
        ],
    },
    'empty_lane_slot': {
        'input_dialog': [
            text('burda asfalt tamiri var'),  # Ремонт асфальта
        ],
    }
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

bass_stubber = create_bass_stubber_fixture(TESTS_DATA_PATH)


@pytest.fixture(scope="function")
def srcrwr_params(bass_stubber):
    return {
        'ADD_POINT_TR_PROXY': f'localhost:{bass_stubber.port}',
    }
