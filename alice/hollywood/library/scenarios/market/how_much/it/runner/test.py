from alice.hollywood.library.scenarios.market.how_much.it.conftest import \
    report_srcrwr_params as srcrwr_params
from alice.hollywood.library.scenarios.market.how_much.it.conftest import hollywood
from alice.hollywood.library.scenarios.market.how_much.it import test_cases

# To satisfy the linter
assert srcrwr_params
assert hollywood

# NOTE: We do not need to use any oauth_token_fixtures here, because this scenario doesn't use oauth

test_quasar_run = test_cases.QUASAR.create_test_function()
test_common_run = test_cases.COMMON.create_test_function()
