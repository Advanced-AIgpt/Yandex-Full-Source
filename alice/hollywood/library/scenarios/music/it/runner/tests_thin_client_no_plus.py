from alice.hollywood.library.scenarios.music.it.srcrwr_thin import srcrwr_params
from alice.hollywood.library.scenarios.music.it.tests_thin_client_no_plus import TEST_RUN_PARAMS, TESTS_DATA_PATH,\
    TESTS_DATA, SCENARIO_HANDLE, music_back_stubber, music_back_dl_info_stubber, music_mds_stubber, hollywood
from alice.hollywood.library.python.testing.integration.conftest import create_test_function, create_bass_stubber_fixture
from alice.hollywood.library.scenarios.music.proto.music_context_pb2 import TMusicContext

# To satisfy the linter
assert music_back_stubber
assert music_back_dl_info_stubber
assert music_mds_stubber
assert srcrwr_params
assert TMusicContext
assert hollywood

bass_stubber = create_bass_stubber_fixture(TESTS_DATA_PATH, stubs_subdir='bass')

test_run = create_test_function(TESTS_DATA_PATH, TEST_RUN_PARAMS, SCENARIO_HANDLE, usefixtures=['srcrwr_params'],
                                tests_data=TESTS_DATA)
