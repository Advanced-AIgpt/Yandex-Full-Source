from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from library.python import resource
from library.python import json

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/mordovia_video_selection/it/data'

SCENARIO_NAME = 'MordoviaVideoSelection'
SCENARIO_HANDLE = 'mordovia_video_selection'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['quasar']

DEFAULT_EXPERIMENTS = ["bg_fresh_granet", "allow_subtitles_and_audio_in_mordovia"]

DEFAULT_DEVICE_STATE = json.loads(resource.find("device_state.json"))

TESTS_DATA = {
    'play_film_by_number': {
        'input_dialog': [
            text('включи номер 2')
        ],
    },
    'play_film_by_name': {
        'input_dialog': [
            text('включи фильм достать ножи')
        ],
    },
    'open_film_description': {
        'input_dialog': [
            text('открой описание фильма достать ножи')
        ],
    },
    'what_to_watch': {
        'input_dialog': [
            text('что посмотреть')
        ],
    },
    'open_tv_channels_tab': {
        'input_dialog': [
            text('перейди на вкладку ТВ')
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
