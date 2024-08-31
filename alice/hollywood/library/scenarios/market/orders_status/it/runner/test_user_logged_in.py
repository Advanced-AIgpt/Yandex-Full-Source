from alice.hollywood.library.scenarios.market.orders_status.it.base import TESTS_DATA_PATH, SCENARIO_HANDLE
from alice.hollywood.library.scenarios.market.orders_status.it.test_user_logged_in_cases import \
    TEST_RUN_PARAMS
from alice.hollywood.library.python.testing.integration.conftest import \
    create_test_function
from alice.hollywood.library.scenarios.market.orders_status.it.conftest import srcrwr_params

# To satisfy the linter
assert srcrwr_params

# NOTE: We do not need to use any oauth_token_fixtures here, because this scenario doesn't use oauth

test_run = create_test_function(
    TESTS_DATA_PATH,
    TEST_RUN_PARAMS,
    usefixtures=["srcrwr_params"],
    scenario_handle=SCENARIO_HANDLE,
)
