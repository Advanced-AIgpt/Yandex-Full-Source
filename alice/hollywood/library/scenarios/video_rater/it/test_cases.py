from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/video_rater/it/data/'

SCENARIO_NAME = 'VideoRater'
SCENARIO_HANDLE = 'video_rater'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = [
    'quasar',
]

DEFAULT_EXPERIMENTS = [
    'mm_enable_protocol_scenario=VideoRater',
]

TESTS_DATA = {
    'cleanup': {
        'input_dialog': [
            text('хочу оценить фильм'),
        ],
        'experiments': ['hw_video_rater_clear_history'],
    },
    'rate_movie': {
        'input_dialog': [
            text('хочу оценить фильм'),
            text('Отличный'),
            text('Скучный'),
            text('Хватит'),
        ],
    },
    'check_memory': {
        'input_dialog': [
            text('хочу оценить фильм'),
            text('нравится'),
            text('хватит'),
        ],
    },
}


TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
