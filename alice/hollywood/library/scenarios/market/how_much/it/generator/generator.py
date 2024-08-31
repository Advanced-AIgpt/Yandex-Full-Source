from alice.hollywood.library.scenarios.market.how_much.it import test_cases
from alice.hollywood.library.scenarios.market.how_much.it.conftest \
    import report_srcrwr_params as srcrwr_params

# To satisfy the linter
assert srcrwr_params

test_quasar_generator = test_cases.QUASAR.create_generator_function()
test_common_generator = test_cases.COMMON.create_generator_function()
