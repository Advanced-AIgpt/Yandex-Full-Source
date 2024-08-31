from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import \
    create_run_request_generator_fun
from alice.hollywood.library.scenarios.suggesters.it import (
    test_cases_games, test_cases_movies
)

test_data_game_suggest = create_run_request_generator_fun(
    test_cases_games.SCENARIO_NAME,
    test_cases_games.SCENARIO_HANDLE,
    test_cases_games.TEST_GEN_PARAMS,
    test_cases_games.TESTS_DATA_PATH,
    test_cases_games.TESTS_DATA,
    test_cases_games.DEFAULT_EXPERIMENTS,
    test_cases_games.DEFAULT_DEVICE_STATE,
    mm_force_scenario=False)

test_data_movie_suggest = create_run_request_generator_fun(
    test_cases_movies.SCENARIO_NAME,
    test_cases_movies.SCENARIO_HANDLE,
    test_cases_movies.TEST_GEN_PARAMS,
    test_cases_movies.TESTS_DATA_PATH,
    test_cases_movies.TESTS_DATA,
    test_cases_movies.DEFAULT_EXPERIMENTS,
    test_cases_movies.DEFAULT_DEVICE_STATE,
    mm_force_scenario=False)
