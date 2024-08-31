from alice.library.python.testing.megamind_request.input_dialog import text

from alice.hollywood.library.scenarios.market.testing.test_case import TestCase

from alice.hollywood.library.scenarios.market.how_much.it.conftest import TESTS_DATA_PATH

SCENARIO_HANDLE = 'market_how_much'


def get_test_case(tests_data, app_presets=None, experiments=None):
    return TestCase(
        tests_data,
        scenario_name='MarketHowMuch',
        scenario_handle=SCENARIO_HANDLE,
        data_path=TESTS_DATA_PATH,
        app_presets=app_presets or ['search_app_prod'],
        experiments=experiments or [],
        fixtures=['srcrwr_params'],
    )

QUASAR = get_test_case(
    tests_data={
        "ask_request_slot": {
            "input_dialog": [
                text('сколько стоит'),
            ],
        },
        'found_goods': {
            'input_dialog': [
                text('сколько стоит утюг'),
            ],
        },
        'empty_serp': {
            'input_dialog': [
                text('сколько стоит феррата сет'),
            ],
        },
    },
    app_presets=['quasar'],
)

COMMON = get_test_case(
    tests_data={
        "ask_request_slot": {
            "input_dialog": [
                text('сколько стоит'),
            ],
        },
        "empty_serp": {
            "input_dialog": [
                text('сколько стоит феррата сет'),
            ],
        },
        "no_redirect": {
            "input_dialog": [
                text('сколько стоит нечто'),
            ],
        },
        "category_redirect": {
            "input_dialog": [
                text('сколько стоит телефон'),
            ],
        },
        "region_redirect": {
            "input_dialog": [
                text('сколько стоит самсунг в хабаровске'),
            ],
        },
        "region_and_category_redirect": {
            "input_dialog": [
                text('сколько стоит айфон в хабаровске'),
            ],
        },
        "parametric_redirect": {
            "input_dialog": [
                text('сколько стоит телевизоры самсунг'),
            ],
        },
        "color_redirect": {
            "input_dialog": [
                text('сколько стоит красная помада'),
            ],
        },
        "gallery_with_model": {
            "input_dialog": [
                text('сколько стоит электронная книга'),
            ],
        },
        "gallery_with_offer": {
            "input_dialog": [
                text('цены на потолочные светильники'),
            ],
        },
        "vulgar_query": {
            "input_dialog": [
                text("сколько стоит боевой пистолет"),
            ],
        },
        # TODO(bas1330) test skipping docs from forbidden categories
        "forbidden_category": {
            "input_dialog": [
                text('сколько стоит книга'),
            ],
        },
        # TODO(bas1330) test request not from KUBR
        "complex_request": {
            "input_dialog": [
                text('утюг сколько стоит красный'),
            ],
        },
        "normalize_request": {
            "input_dialog": [
                text('сколько стоит плейстейшн четыре'),
            ],
        },
    },
)
