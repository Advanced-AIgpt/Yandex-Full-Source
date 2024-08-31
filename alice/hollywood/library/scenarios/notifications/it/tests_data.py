from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params, RunRequestFormat
from alice.library.python.testing.megamind_request.input_dialog import text

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/notifications/it/data/'

SCENARIO_NAME = 'NotificationsManager'
SCENARIO_HANDLE = 'notifications_manager'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['quasar']
DEFAULT_EXPERIMENTS = []
RUN_REQUEST_FORMAT = RunRequestFormat.PROTO_TEXT

TESTS_DATA = {
    'notifications_subscribe_accept': {
        'input_dialog': [
            text('хочу получать уведомления про новые функции'),
            text('да')
        ],
    },
    'notifications_subscribe_refuse': {
        'input_dialog': [
            text('хочу получать уведомления про новые функции'),
            text('нет')
        ],
    },
    'notifications_subscribe_twice': {
        'input_dialog': [
            text('хочу получать уведомления про новые функции'),
        ],
    },
    'notifications_unsubscribe_empty_accept': {
        'input_dialog': [
            text('отписаться от уведомления'),
            text('да')
        ],
    },
    'notifications_unsubscribe_empty_refuse': {
        'input_dialog': [
            text('отписаться от уведомления'),
            text('нет')
        ],
    },
    'notifications_unsubscribe_accept': {
        'input_dialog': [
            text('отписаться от уведомлений про новые функции'),
            text('да')
        ],
    },
    'notifications_unsubscribe_refuse': {
        'input_dialog': [
            text('отписаться от уведомлений про новые функции'),
            text('нет')
        ],
    },
    'notifications_unsubscribe_all_accept': {
        'input_dialog': [
            text('отписаться от уведомлений'),
        ],
    },
    'notifications_onboarding_accept': {
        'input_dialog': [
            text('почему ты мигаешь'),
            text('да'),
        ],
    },
    'notifications_onboarding_refuse': {
        'input_dialog': [
            text('почему ты мигаешь'),
            text('нет'),
        ],
    },
    'notifications_read_empty': {
        'input_dialog': [
            text('покажи мои уведомления'),
        ],
    },
    'notifications_read': {
        'input_dialog': [
            text('покажи мои уведомления'),
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
