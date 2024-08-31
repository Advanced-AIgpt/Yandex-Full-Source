from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/suggesters/it/data_movies/'

SCENARIO_NAME = 'MovieSuggest'
SCENARIO_HANDLE = 'movie_suggest'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['quasar']

DEFAULT_EXPERIMENTS = ['mm_enable_protocol_scenario=Video',
                       'mm_proactivity_movie_suggest']

DEFAULT_DEVICE_STATE = {"is_tv_plugged_in": "true"}

TESTS_DATA = {
    'accept_third_movie': {
        'input_dialog': [
            text('порекомендуй мне фильм'),
            text('уже смотрел'),
            text('следующий'),  # 'дальше' не работает
            text('давай'),  # 'давай этот' не работает
        ],
    },
    'show_cartoons': {
        'input_dialog': [
            text('посоветуй мультики')
        ],
    },
    'show_everything': {
        'input_dialog': [
            text('что посмотреть')
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
