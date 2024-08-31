from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import create_run_request_generator_fun
from alice.hollywood.library.scenarios.alice_show.it.test_cases import TEST_GEN_PARAMS, TESTS_DATA_PATH, TESTS_DATA, \
    DEFAULT_EXPERIMENTS, SCENARIO_NAME, SCENARIO_HANDLE, oauth_token_plus

# To satisfy the linter
assert oauth_token_plus

test_run_request_generator = create_run_request_generator_fun(
    SCENARIO_NAME,
    SCENARIO_HANDLE,
    TEST_GEN_PARAMS,
    TESTS_DATA_PATH,
    TESTS_DATA,
    DEFAULT_EXPERIMENTS,
    mm_force_scenario=False,
    use_oauth_token_fixture='oauth_token_plus',
)
