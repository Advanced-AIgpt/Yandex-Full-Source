import pytest

from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.hollywood.library.python.testing.integration.conftest import create_bass_stubber_fixture

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/news/it/data_push/'

SCENARIO_NAME = 'News'
SCENARIO_HANDLE = 'news'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

DEFAULT_APP_PRESETS = ['quasar']

DEFAULT_EXPERIMENTS = [
    'alice_news_change_source_postroll',
    'alice_news_change_source_push',
    'alice_news_postroll_after_set_proba=1',
    'bg_enable_news_settings',
    'news_has_ticket_for_exp',
    'alice_news_radio_news_postroll_first_proba=0',
    'alice_news_radio_news_postroll_proba=0',
]

TESTS_DATA = {
    # Send push.
    'get_news_settings_direct_configure': {
        'input_dialog': [
            text('измени новости'),
        ],
    },
    'get_news_settings_default': {
        'input_dialog': [
            text('новости'),
            # TODO(flimsywhimsy): uncomment after promos rework
            # text('давай'),
        ],
        'experiments': ['alice_news_postroll_default_proba=1'],
    },
    # No push, postroll and direct topic changes.
    'get_news_settings_default_to_sport': {
        'input_dialog': [
            text('новости'),
            # TODO(flimsywhimsy): uncomment after promos rework
            # text('смени на спорт'),
        ],
        'experiments': ['alice_news_postroll_default_proba=1'],
    },
    'get_news_settings_rbk_agree': {
        'input_dialog': [
            text('новости РБК'),
            # TODO(flimsywhimsy): uncomment after promos rework
            # text('хочу'),
        ],
        'experiments': ['alice_news_postroll_new_topic_proba=1'],
    },
    # Decline.
    'get_news_settings_vc_decline': {
        'input_dialog': [
            text('новости виси'),
            # TODO(flimsywhimsy): uncomment after promos rework
            # text('не хочу'),
        ],
        'experiments': ['alice_news_postroll_new_topic_proba=1'],
    },
}


TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

bass_stubber = create_bass_stubber_fixture(TESTS_DATA_PATH)


@pytest.fixture(scope="function")
def srcrwr_params(bass_stubber):
    return {
        'NEWS_SCENARIO_PROXY': f'localhost:{bass_stubber.port}',
        'NEWS_SCENARIO_APPLY_PROXY': f'localhost:{bass_stubber.port}',
    }
