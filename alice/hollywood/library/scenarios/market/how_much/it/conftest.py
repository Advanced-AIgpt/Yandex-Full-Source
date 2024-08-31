import pytest

from alice.hollywood.library.python.testing.integration.conftest import create_stubber_fixture, create_hollywood_fixture
from alice.hollywood.library.python.testing.stubber.stubber_server import StubberEndpoint


TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/market/how_much/it/data/'
report_stubber = create_stubber_fixture(
    TESTS_DATA_PATH,
    'alice-report.tst.vs.market.yandex.net',
    port=17051,
    endpoints=[
        StubberEndpoint('/yandsearch', ['GET']),
    ],
)

SCENARIO_HANDLE = 'market_how_much'
hollywood = create_hollywood_fixture([SCENARIO_HANDLE])


@pytest.fixture(scope="function")
def report_srcrwr_params(report_stubber):
    return {
        'MARKET_HOW_MUCH_SCENARIO_REPORT_PROXY_0': f'localhost:{report_stubber.port}',
        'MARKET_HOW_MUCH_SCENARIO_REPORT_PROXY_1': f'localhost:{report_stubber.port}',
        'MARKET_HOW_MUCH_SCENARIO_REPORT_PROXY_2': f'localhost:{report_stubber.port}',
        'MARKET_HOW_MUCH_SCENARIO_REPORT_PROXY_3': f'localhost:{report_stubber.port}',
    }
