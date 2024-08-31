from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import text
from alice.hollywood.library.python.testing.integration.conftest import create_oauth_token_fixture, create_hollywood_fixture
from alice.library.python.testing.auth import auth

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/music/it/data_hardcoded_music/'

SCENARIO_NAME = 'HollywoodHardcodedMusic'
SCENARIO_HANDLE = 'hardcoded_music'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['quasar', 'search_app_prod']

DEFAULT_EXPERIMENTS = []

DEFAULT_MEMENTO = '''
{
    "UserConfigs": {
        "MorningShowNewsConfig": {
            "Default": true,
            "NewsProviders": [{
                "NewsSource": "6e24a5bb-yandeks-novost",
                "Rubric": "__mixed_news__"
            }]
        },
        "MorningShowTopicsConfig": {
            "Default": true
        },
        "MorningShowSkillsConfig": {
            "Default": true
        }
    }
}
'''


TESTS_DATA = {
    'music_hardcoded_meditation': {
        'input_dialog': [
            text('запусти медитацию'),
        ],
    },
    'music_hardcoded_meditation_relax': {
        'input_dialog': [
            text('запусти медитацию для расслабления'),
        ],
    },
    'music_hardcoded_alice_show': {
        'input_dialog': [
            text('алиса включи утреннее шоу', memento=DEFAULT_MEMENTO)
        ],
    },
    'music_hardcoded_alice_children_show': {
        'input_dialog': [
            text('алиса включи детское шоу', memento=DEFAULT_MEMENTO),
        ],
    },
    'music_hardcoded_alice_show_skill': {
        'input_dialog': [
            text('алиса включи утреннее шоу', memento=DEFAULT_MEMENTO),
        ],
        'experiments': ['hw_morning_show_skills_before_topics', 'hw_morning_show_skill_count=-1'],
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)

oauth_token_plus = create_oauth_token_fixture(auth.YaPlusMusicLikes)
