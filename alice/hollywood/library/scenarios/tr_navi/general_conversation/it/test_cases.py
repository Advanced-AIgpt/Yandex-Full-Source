import pytest

from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.hollywood.library.python.testing.integration.conftest import create_bass_stubber_fixture, create_hollywood_fixture

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/tr_navi/general_conversation/it/data/'

SCENARIO_NAME = 'GeneralConversationTr'
SCENARIO_HANDLE = 'general_conversation_tr'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['navigator']

DEFAULT_EXPERIMENTS = [
    'mm_enable_protocol_scenario=GeneralConversationTr',
]

DEFAULT_DEVICE_STATE = {}

TESTS_DATA = {
    'beg_your_pardon': {
        'input_dialog': [
            text('nasılsın'),
        ],
    },
    'general_conversation_dummy': {
        'input_dialog': [
            text('ahmet haluk koç'),
            text('abdul ahat andican'),
        ],
        'experiments': ['gc_not_banned'],
    },
    'general_conversation': {
        'input_dialog': [
            text('nasılsın'),
        ],
        'experiments': ['gc_not_banned'],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

bass_stubber = create_bass_stubber_fixture(TESTS_DATA_PATH)


@pytest.fixture(scope="function")
def srcrwr_params(bass_stubber):
    return {
        'GENERAL_CONVERSATION_TR_PROXY': f'localhost:{bass_stubber.port}',
    }
