import pytest

from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.hollywood.library.python.testing.integration.conftest import create_stubber_fixture, create_hollywood_fixture
from alice.library.python.testing.megamind_request.input_dialog import text, image
from alice.hollywood.library.python.testing.stubber.stubber_server import StubberEndpoint


TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/transform_face/it/data/'

SCENARIO_NAME = 'TransformFace'
SCENARIO_HANDLE = 'transform_face'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

DEFAULT_APP_PRESETS = ['search_app_prod']

DEFAULT_EXPERIMENTS = []

DEFAULT_DEVICE_STATE = {}

TESTS_DATA = {
    'test_aging_text': {
        'input_dialog': [
            text('состарь меня'),
            image('https://avatars.mds.yandex.net/get-alice/4336950/test_ZQ9GHiqCbZo6OsN1TlPZDQ/big'),
        ],
        'app_presets': {
            'only': ['search_app_prod'],
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
        'TRANSFORM_FACE_PROXY': f'localhost:{cbir_features_stubber.port}'
    }
