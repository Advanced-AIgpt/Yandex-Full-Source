# -*- coding: utf-8 -*-
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import create_run_request_generator_fun
from alice.hollywood.library.scenarios.search.it.apps_fixlist_tests_data import TEST_GEN_PARAMS, TESTS_DATA_PATH, TESTS_DATA, \
    DEFAULT_EXPERIMENTS, SCENARIO_NAME, SCENARIO_HANDLE, RUN_REQUEST_FORMAT

from alice.hollywood.library.scenarios.search.proto.search_pb2 import TSearchState

assert TSearchState  # to satisfy the linter

test_run_request_generator = create_run_request_generator_fun(
    SCENARIO_NAME,
    SCENARIO_HANDLE,
    TEST_GEN_PARAMS,
    TESTS_DATA_PATH,
    TESTS_DATA,
    DEFAULT_EXPERIMENTS,
    run_request_format=RUN_REQUEST_FORMAT)
