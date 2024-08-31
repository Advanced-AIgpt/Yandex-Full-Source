from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params, RunRequestFormat
from alice.library.python.testing.megamind_request.input_dialog import text

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/search/it/data/'

SCENARIO_NAME = 'Search'
SCENARIO_HANDLE = 'search'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['quasar', 'search_app_prod', 'launcher', 'auto', 'elariwatch']

DEFAULT_EXPERIMENTS = ['websearch_enable', 'read_factoid_source', 'enable_protocol_search_everywhere']

RUN_REQUEST_FORMAT = RunRequestFormat.PROTO_BINARY

TESTS_DATA = {
    'apple_calorific': {
        'input_dialog': [
            text('какая калорийность у яблока'),
        ],
    },
    'calculator_10': {
        'input_dialog': [
            text('сколько будет 10 плюс 10'),
        ],
    },
    'calculator_float': {
        'input_dialog': [
            text('сколько будет 10 поделить на 3'),
        ],
    },
    'suggest_fact': {
        'input_dialog': [
            text('кто такой джеф безос'),
        ],
    },
    'entity_fact': {
        'input_dialog': [
            text('сколько лет наполеону'),
        ],
    },
    'units_converter': {
        'input_dialog': [
            text('15 километров в мили'),
        ],
    },
    'time_difference': {
        'input_dialog': [
            text('разница во времени между москвой и чикаго'),
        ],
    },
    'distance_fact': {
        'input_dialog': [
            text('расстояние между москвой и владивостоком'),
        ],
    },
    'zip_code': {
        'input_dialog': [
            text('льва толстого 16 индекс'),
        ],
    },
    'sport_live_score': {
        'input_dialog': [
            text('счет в матче арсенала'),
        ],
    },
    'object_answer_as_fact': {
        'input_dialog': [
            text('москва'),
        ],
    },
    'instructions': {
        'input_dialog': [
            text('рецепт блинов'),
        ],
    },
    'show_serp': {
        'input_dialog': [
            text('как живут евреи'),
        ],
    },
    'serp_navi': {
        'input_dialog': [
            text('разнообразные столы'),
            text('открывай'),
        ],
        'app_presets': {
            'only': ['navigator'],
        },
    },
}

TEST_GEN_PARAMS = make_generator_params(TESTS_DATA, DEFAULT_APP_PRESETS)

TEST_RUN_PARAMS = make_runner_params(TESTS_DATA, DEFAULT_APP_PRESETS)
