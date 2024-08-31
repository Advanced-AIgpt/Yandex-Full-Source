import pytest

from alice.hollywood.library.python.testing.integration.conftest import\
    create_bass_stubber_fixture, create_oauth_token_fixture, create_hollywood_fixture

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/market/orders_status/it/data/'

bass_stubber = create_bass_stubber_fixture(TESTS_DATA_PATH)

SCENARIO_HANDLE = 'market_orders_status'
hollywood = create_hollywood_fixture([SCENARIO_HANDLE])


@pytest.fixture(scope="function")
def srcrwr_params(bass_stubber):
    return {
        'MARKET_ORDERS_STATUS_SCENARIO_PROXY': f'localhost:{bass_stubber.port}',
    }


guest_token_fixture = create_oauth_token_fixture('sec-01e2g093x7jpr4kz53q27d140m')
logged_in_token_fixture = create_oauth_token_fixture('sec-01e2fjghm8dh4xg41f6ecg04ty')
