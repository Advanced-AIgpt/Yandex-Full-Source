import pytest

from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.hollywood.library.python.testing.integration.conftest import create_stubber_fixture, create_hollywood_fixture
from alice.hollywood.library.python.testing.stubber.stubber_server import StubberEndpoint

from alice.tests.integration_tests.ml.draw_picture import TestDrawPicture


TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/draw_picture/it/data/'

SCENARIO_NAME = 'DrawPicture'
SCENARIO_HANDLE = 'draw_picture'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['search_app_prod']

DEFAULT_EXPERIMENTS = []

DEFAULT_DEVICE_STATE = {}  # NOTE: you may pass some device_state to the `create_run_request_generator_fun`

tests = TestDrawPicture()

TESTS_DATA = {
    'test_draw_picture': {
        'input_dialog': [
            text('нарисуй картину'),
        ],
        'app_presets': {
            'only': ['launcher', 'search_app_prod'],
        },
    },
    'test_draw_picture_yabro': {
        'input_dialog': [
            text('нарисуй картину'),
        ],
        'app_presets': {
            'only': ['yabro_prod'],
        },
    },
    'test_station_surface': {
        'input_dialog': [
            text('нарисуй картину'),
        ],
        'app_presets': {
            'only': ['quasar', 'yandexmini', 'tv'],
        },
    },
    'test_unsupported_surface': {
        'input_dialog': [
            text('нарисуй картину'),
        ],
        'app_presets': {
            'only': ['navigator', 'elariwatch', 'auto'],
        },
    },
    'test_route': {
        'input_dialog': [
            text('нарисуй маршрут до казани'),
        ],
        'app_presets': {
            'only': ['navigator', 'auto', 'search_app_prod'],
        },
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

cbir_features_stubber = create_stubber_fixture(
    TESTS_DATA_PATH,
    'yandex.ru',
    80,
    [
        StubberEndpoint('/images-apphost/cbir-features', ['GET']),
    ],
)


@pytest.fixture(scope="function")
def srcrwr_params(cbir_features_stubber):
    return {
        'DRAW_PICTURE_PROXY': f'localhost:{cbir_features_stubber.port}'
    }
