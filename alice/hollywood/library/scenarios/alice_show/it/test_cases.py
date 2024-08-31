from alice.hollywood.library.python.testing.integration.conftest import create_oauth_token_fixture, create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.library.python.testing.auth import auth


TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/alice_show/it/data/'

SCENARIO_NAME = 'AliceShow'
SCENARIO_HANDLE = 'alice_show'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

DEFAULT_APP_PRESETS = ['quasar', 'search_app_prod']

DEFAULT_EXPERIMENTS = [
    f'mm_enable_protocol_scenario={SCENARIO_NAME}',
    'hw_alice_show_enable_push',
]

DEFAULT_DEVICE_STATE = {}

DEFAULT_MEMENTO = {
    "UserConfigs": {
        "MorningShowNewsConfig": {
            "Default": True,
            "NewsProviders": [{
                "NewsSource": "6e24a5bb-yandeks-novost",
                "Rubric": "__mixed_news__"
            }]
        },
        "MorningShowTopicsConfig": {
            "Default": True
        },
        "MorningShowSkillsConfig": {
            "Default": True
        }
    }
}

TESTS_DATA = {
    'morning_show': {
        'input_dialog': [
            text('утреннее шоу', memento=DEFAULT_MEMENTO),
        ],
    },
    'evening_show': {
        'input_dialog': [
            text('вечернее шоу', memento=DEFAULT_MEMENTO),
        ],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

oauth_token_plus = create_oauth_token_fixture(auth.YaPlusNoMusicLikes)
oauth_token_no_plus = create_oauth_token_fixture(auth.NoYaPlus)
