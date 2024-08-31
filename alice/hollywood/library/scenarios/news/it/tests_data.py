import pytest

from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.hollywood.library.python.testing.integration.conftest import create_bass_stubber_fixture

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/news/it/data/'

SCENARIO_NAME = 'News'
SCENARIO_HANDLE = 'news'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

DEFAULT_APP_PRESETS = ['quasar', 'search_app_prod']

DEFAULT_EXPERIMENTS = ['news_has_ticket_for_exp', 'alice_no_promo']

TESTS_DATA = {
    # Default.
    'get_news_default_index': {
        'input_dialog': [
            text('новости'),
        ],
    },
    # Topic.
    'get_news_topic_index': {
        'input_dialog': [
            text('расскажи главные новости'),
        ],
    },
    'get_news_topic_sport': {
        'input_dialog': [
            text('покажи новости спорта'),
        ],
    },
    'get_news_topic_personal': {
        'input_dialog': [
            text('покажи интересные новости'),
        ],
    },
    # Where.
    'get_news_where_moscow': {
        'input_dialog': [
            text('какие новости москва'),
        ],
    },
    'get_news_where_russia': {
        'input_dialog': [
            text('прочитай новости россии'),
        ],
    },
    'get_news_where_tyumen_region': {
        'input_dialog': [
            text('новости тюменской области'),
        ],
    },
    'get_news_where_ukraine': {
        'input_dialog': [
            text('новости украины'),
        ],
    },
    # Nonews.
    'get_news_nonews': {
        'input_dialog': [
            text('новости хиромантии'),
        ],
    },
    # Player pause directives.
    'get_news_when_music_is_playing': {
        'input_dialog': [
            text('новости', device_state={
                'music': {
                    'player': {
                        'pause': False
                    },
                    # Player paused in BASS check 'currently_playing' is nullable.
                    'currently_playing': {
                        'pause': False
                    }
                }
            }),
        ],
        'supported_features': ['tts_play_placeholder'],
    },
    # Experiments.
    'get_news_default_sport': {
        'input_dialog': [
            text('новости'),
        ],
        'experiments': ['alice_news_default_rubric=sport'],
    },
    'get_news_no_pause_sound': {
        'input_dialog': [
            text('новости'),
        ],
        'experiments': ['alice_news_no_pause_sound'],
    },
    # No rubrics
    'get_news_default_index_no_rubrics': {
        'input_dialog': [
            text('новости'),
        ],
        'experiments': ['news_disable_rubric_api'],
    },
    'get_news_topic_index_no_rubrics': {
        'input_dialog': [
            text('расскажи главные новости'),
        ],
        'experiments': ['news_disable_rubric_api'],
    },
    'get_news_topic_sport_no_rubrics': {
        'input_dialog': [
            text('покажи новости спорта'),
        ],
        'experiments': ['news_disable_rubric_api'],
    },
    'get_news_topic_personal_no_rubrics': {
        'input_dialog': [
            text('покажи интересные новости'),
        ],
        'experiments': ['news_disable_rubric_api'],
    },
    'get_news_where_moscow_no_rubrics': {
        'input_dialog': [
            text('какие новости москва'),
        ],
        'experiments': ['news_disable_rubric_api'],
    },
    'get_news_where_russia_no_rubrics': {
        'input_dialog': [
            text('прочитай новости россии'),
        ],
        'experiments': ['news_disable_rubric_api'],
    },
    'get_news_where_tyumen_region_no_rubrics': {
        'input_dialog': [
            text('новости тюменской области'),
        ],
        'experiments': ['news_disable_rubric_api'],
    },
    'get_news_where_ukraine_no_rubrics': {
        'input_dialog': [
            text('новости украины'),
        ],
        'experiments': ['news_disable_rubric_api'],
    },
    'get_news_nonews_no_rubrics': {
        'input_dialog': [
            text('новости хиромантии'),
        ],
        'experiments': ['news_disable_rubric_api'],
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
