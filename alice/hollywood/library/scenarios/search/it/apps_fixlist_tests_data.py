from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params, RunRequestFormat
from alice.library.python.testing.megamind_request.input_dialog import text

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/search/it/data_fixlist/'

SCENARIO_NAME = 'OpenAppsFixlist'
SCENARIO_HANDLE = 'open_apps_fixlist'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['quasar', 'search_app_prod', 'launcher', 'auto', 'elariwatch']

DEFAULT_EXPERIMENTS = [
    'enable_ya_eda_fixlist',
    'enable_whocalls_fixlist',
    'bg_muzpult_granet',
    'bg_muzpult_dssm',
    'bg_fresh_alice_prefix=alice.apps_fixlist',
]

RUN_REQUEST_FORMAT = RunRequestFormat.PROTO_BINARY

TESTS_DATA = {
    'hungry': {
        'input_dialog': [
            text('я хочу есть'),
        ],
    },
    'order': {
        'input_dialog': [
            text('купи бургеры'),
        ],
    },
    'deliver': {
        'input_dialog': [
            text('доставь пиццу'),
        ],
    },
    'aon': {
        'input_dialog': [
            text('настрой аон'),
        ],
    },
    'order_navi': {
        'input_dialog': [
            text('купи бургеры'),
            text('открывай'),
        ],
        'app_presets': {
            'only': ['navigator'],
        },
    },
    'muzpult': {
        'input_dialog': [
            text('включи музыкальный пульт'),
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
