from alice.hollywood.library.scenarios.market.orders_status.it import test_cases

from alice.hollywood.library.scenarios.market.orders_status.it.conftest import (
    srcrwr_params,
    guest_token_fixture,
    logged_in_token_fixture,
)

# To satisfy the linter
assert srcrwr_params
assert guest_token_fixture
assert logged_in_token_fixture

test_disabled_platforms = test_cases.DISABLED_PLATFORMS.create_generator_function()
test_guest = test_cases.GUEST.create_generator_function()
test_user_logged_in = test_cases.USER_LOGGED_IN.create_generator_function()
