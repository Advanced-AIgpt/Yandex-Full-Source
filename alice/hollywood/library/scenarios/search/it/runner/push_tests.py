# -*- coding: utf-8 -*-
from alice.hollywood.library.scenarios.search.it.test_cases_push import TEST_RUN_PARAMS,\
    TESTS_DATA_PATH, SCENARIO_HANDLE, hollywood
from alice.hollywood.library.python.testing.integration.conftest import create_test_function
from alice.hollywood.library.scenarios.search.proto.search_pb2 import TSearchState

# To satisfy the linter
assert TSearchState
assert hollywood

test_push = create_test_function(
    TESTS_DATA_PATH,
    TEST_RUN_PARAMS,
    SCENARIO_HANDLE,
)
