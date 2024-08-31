from alice.hollywood.library.scenarios.market.orders_status.it.conftest import srcrwr_params
from alice.hollywood.library.scenarios.market.orders_status.it import test_cases

# To satisfy the linter
assert srcrwr_params

# NOTE: We do not need to use any oauth_token_fixtures here, because this scenario doesn't use oauth

test_disabled_platforms_run = test_cases.DISABLED_PLATFORMS.create_test_function()
test_guest_run = test_cases.GUEST.create_test_function()
test_user_logged_in_run = test_cases.USER_LOGGED_IN.create_test_function()
