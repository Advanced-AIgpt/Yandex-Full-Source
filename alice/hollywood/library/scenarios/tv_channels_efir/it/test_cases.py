from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from library.python import resource
from library.python import json

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/tv_channels_efir/it/data'

SCENARIO_NAME = 'ShowTvChannelsGallery'
SCENARIO_HANDLE = 'show_tv_channels_gallery'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['quasar']

DEFAULT_EXPERIMENTS = ["bg_fresh_granet", "enable_hollywood_scenario_play_channel_by_name"]

DEFAULT_DEVICE_STATE = json.loads(resource.find("device_state.json"))

TESTS_DATA = {
    'show_tv_channels_gallery': {
        'input_dialog': [
            text('покажи доступные телеканалы')
        ],
    }
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
