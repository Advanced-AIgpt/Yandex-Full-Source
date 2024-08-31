import pytest

from alice.hollywood.library.scenarios.tv_channels.it.test_cases import TESTS_DATA_PATH
from alice.hollywood.library.python.testing.integration.conftest import create_stubber_fixture
from alice.hollywood.library.python.testing.stubber.stubber_server import StubberEndpoint

saas_stubber = create_stubber_fixture(
    TESTS_DATA_PATH,
    'saas-searchproxy-prestable.yandex.net',
    17000,
    [
        StubberEndpoint('/', ['GET']),
    ],
    stubs_subdir='saas_stubs'
)


@pytest.fixture(scope="function")
def srcrwr_params(saas_stubber):
    return {
        'TV_CHANNELS_PROXY': f'localhost:{saas_stubber.port}',
    }
