from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/tr_navi/handcrafted/it/data/'

SCENARIO_NAME = 'HandcraftedTr'
SCENARIO_HANDLE = 'handcrafted_tr'

hollywood =create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['navigator']

DEFAULT_EXPERIMENTS = [
    'mm_enable_protocol_scenario=HandcraftedTr',
]

DEFAULT_DEVICE_STATE = {}

TESTS_DATA = {
    'do_you_believe_in_god': {
        'input_dialog': [
            text('Sence Yaradan gerçek mi?'),
        ],
    },
    'send_me_your_photo': {
        'input_dialog': [
            text('Selfie\'ni yollasana.'),
        ],
    },
    'who_am_i': {
        'input_dialog': [
            text('Ben kimim?'),
        ],
    },
    'lets_have_a_smoke': {
        'input_dialog': [
            text('Sigara mı içsek?'),
        ],
    },
    'do_you_have_children': {
        'input_dialog': [
            text('Hiç çocuğun var mı?'),
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
