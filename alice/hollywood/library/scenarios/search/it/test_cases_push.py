from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/search/it/data_push/'

SCENARIO_NAME = 'Search'
SCENARIO_HANDLE = 'search'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

DEFAULT_APP_PRESETS = ['quasar']

DEFAULT_EXPERIMENTS = ['push_do_nothing_on_decline', 'handoff_promo_proba_1.0']

TESTS_DATA = {
    'sent_push': {
        'input_dialog': [
            text('расскажи какая калорийность у яблока'),
            text('отправь пуш')
        ],
        'experiments': DEFAULT_EXPERIMENTS
    },
    'promo_agree': {
        'input_dialog': [
            text('расскажи какая калорийность у яблока'),
            text('да')
        ],
        'experiments': DEFAULT_EXPERIMENTS
    },
    'promo_disagree': {
        'input_dialog': [
            text('расскажи какая калорийность у яблока'),
            text('нет')
        ],
        'experiments': DEFAULT_EXPERIMENTS
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)
TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
