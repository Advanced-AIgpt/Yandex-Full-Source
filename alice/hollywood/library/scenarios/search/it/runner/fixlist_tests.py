# -*- coding: utf-8 -*-
from alice.hollywood.library.scenarios.search.it.apps_fixlist_tests_data import TEST_RUN_PARAMS,\
    TESTS_DATA_PATH, SCENARIO_HANDLE, hollywood
from alice.hollywood.library.python.testing.integration.conftest import create_test_function
from alice.hollywood.library.scenarios.search.proto.search_pb2 import TSearchState

assert TSearchState  # to satisfy the linter
assert hollywood  # to satisfy the linter

test_open_apps_fixlist = create_test_function(TESTS_DATA_PATH, TEST_RUN_PARAMS, SCENARIO_HANDLE)
