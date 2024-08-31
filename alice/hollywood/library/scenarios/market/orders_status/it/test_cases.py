from alice.library.python.testing.megamind_request.input_dialog import text

from alice.hollywood.library.scenarios.market.testing.test_case import TestCase

from alice.hollywood.library.scenarios.market.orders_status.it.conftest import TESTS_DATA_PATH

SCENARIO_HANDLE = 'market_orders_status'


def get_test_case(tests_data, app_presets=None, oauth_token_fixture='logged_in_token_fixture'):
    return TestCase(
        tests_data,
        scenario_name='MarketOrdersStatus',
        scenario_handle=SCENARIO_HANDLE,
        data_path=TESTS_DATA_PATH,
        app_presets=app_presets or ['search_app'],
        fixtures=['srcrwr_params'],
        oauth_token_fixture=oauth_token_fixture,
    )

DISABLED_PLATFORMS = get_test_case(
    tests_data={
        'ask_orders_status': {
            'input_dialog': [
                text('что с моим заказом')
            ],
            'run_request_generator': {
                'skip': True,
            },
        },
    },
    app_presets=['quasar', 'navigator', 'elariwatch', 'auto'],
)

GUEST = get_test_case(
    tests_data={
        'ask_login': {
            'input_dialog': [
                text('что с моим заказом'),
                text('я залогинился'),
            ],
        },
    },
    app_presets=["search_app_prod"],
    oauth_token_fixture='guest_token_fixture',
)

USER_LOGGED_IN = get_test_case(
    tests_data={
        'ask_orders_status': {
            'input_dialog': [
                text('что с моим заказом')
            ],
        },
        'empty_frame': {
            'input_dialog': [
                text('привет'),
            ],
        },
    },
    app_presets=["search_app_prod"],
)
