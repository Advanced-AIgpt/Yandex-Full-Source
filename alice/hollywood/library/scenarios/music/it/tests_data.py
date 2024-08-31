from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.hollywood.library.python.testing.integration.conftest import create_oauth_token_fixture, \
    create_hollywood_fixture
from alice.library.python.testing.auth import auth


TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/music/it/data/'

SCENARIO_NAME = 'HollywoodMusic'
SCENARIO_HANDLE = 'music'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['quasar']

DEFAULT_EXPERIMENTS = []

TESTS_DATA = {
    # TODO(vitvlkv): Move music_not_found test case to it2
    'music_not_found': {
        'input_dialog': [
            text('улица матросова'),
        ],
        'run_request_generator': {
            'skip': True  # Because this input text has started to find some music but we need "not found" result
        },
    },
    # TODO(vitvlkv): Move no_frames test case to it2
    'no_frames': {
        'input_dialog': [
            text('привет'),
        ],
        'run_request_generator': {
            'skip': True  # Because we deleted semantic frames from the request by hand
        },
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

oauth_token_plus = create_oauth_token_fixture(auth.YaPlusMusicLikes)
