from alice.hollywood.library.scenarios.news.it.tests_data_push import TEST_RUN_PARAMS, TESTS_DATA_PATH, SCENARIO_HANDLE, \
    bass_stubber, srcrwr_params, hollywood
from alice.hollywood.library.python.testing.integration.conftest import create_test_function
from alice.hollywood.library.scenarios.news.proto.news_pb2 import TNewsState

# To satisfy the linter
assert bass_stubber
assert srcrwr_params
assert TNewsState
assert hollywood

test_news_run = create_test_function(
    TESTS_DATA_PATH,
    TEST_RUN_PARAMS,
    SCENARIO_HANDLE,
    usefixtures=['srcrwr_params']
)
