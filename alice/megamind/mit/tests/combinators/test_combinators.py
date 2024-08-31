import pytest

from alice.megamind.mit.library.common.names.node_names import (
    NODE_NAME_WALKER_FINALIZE,
    combinator_node_name,
    scenario_node_name,
    Stage,
)
import alice.megamind.mit.library.common.names.combinator_names as combinator_names
import alice.megamind.mit.library.common.names.scenario_names as scenario_names
from alice.megamind.mit.library.common.util.combinator.responses import (
    generate_handler_combinator_response,
    generate_handler_combinator_response_with_continue_args,
)
from alice.megamind.mit.library.common.util.scenario.responses import (
    generate_handler_run_response_with_continue_args,
    generate_handler_apply_response_with_output_speech,
)

from alice.megamind.mit.library.request_builder import ServerAction
from alice.megamind.mit.library.common.names.item_names import (
    AH_ITEM_COMBINATOR_MAIN_SCREEN_RESPONSE_RUN,
    AH_ITEM_COMBINATOR_MAIN_SCREEN_RESPONSE_CONTINUE,
    AH_ITEM_START_MUSIC_CONTINUE,
    AH_ITEM_SCENARIO_WEATHER_RUN_RESPONSE,
    AH_ITEM_SCENARIO_MUSIC_RUN_RESPONSE,
    AH_ITEM_SCENARIO_MUSIC_CONTINUE_RESPONSE,
)
from apphost.lib.proto_answers.http_pb2 import THttpResponse


CENTAUR_COLLECT_MAIN_SCREEN_FRAME = {
    'typed_semantic_frame': {
        'centaur_collect_main_screen': {},
    },
    'analytics': {
        'product_scenario': 'Centaur',
        'origin': 'SmartSpeaker',
        'purpose': 'centaur_collect_main_screen'
    }
}


def generate_blackbox_handler():
    def handler(ctx):
        response = THttpResponse()
        response.StatusCode = 200
        ctx.add_protobuf_item(b'http_response', response)


@pytest.mark.experiments('mm_enable_combinator=CentaurMainScreen')
class TestCombinators(object):
    def test_combinator_run(self, alice, apphost_stubber):
        return
        query = ServerAction(
            name='@@mm_semantic_frame',
            payload=CENTAUR_COLLECT_MAIN_SCREEN_FRAME
        )

        apphost_stubber.mock_node('BLACKBOX_HTTP', generate_blackbox_handler())
        apphost_stubber.mock_node(combinator_node_name(combinator_names.COMBINATOR_CENTAUR_MAIN_SCREEN, Stage.RUN), generate_handler_combinator_response())

        def checker(ctx):
            assert ctx.get_protobuf_item(AH_ITEM_COMBINATOR_MAIN_SCREEN_RESPONSE_RUN) is not None

        def check_not_visited(ctx):
            assert False

        def check_combinator_input_scenarios(ctx):
            assert ctx.get_protobuf_item(AH_ITEM_SCENARIO_WEATHER_RUN_RESPONSE) is not None

        apphost_stubber.assert_node_input(NODE_NAME_WALKER_FINALIZE, checker)
        apphost_stubber.assert_node_input(combinator_node_name(combinator_names.COMBINATOR_CENTAUR_MAIN_SCREEN, Stage.CONTINUE), check_not_visited, True)
        apphost_stubber.assert_node_input(combinator_node_name(combinator_names.COMBINATOR_CENTAUR_MAIN_SCREEN, Stage.RUN), check_combinator_input_scenarios)

        alice(query)

    def test_combinator_continue(self, alice, apphost_stubber):
        return
        query = ServerAction(
            name='@@mm_semantic_frame',
            payload=CENTAUR_COLLECT_MAIN_SCREEN_FRAME
        )

        apphost_stubber.mock_node('BLACKBOX_HTTP', generate_blackbox_handler())

        apphost_stubber.mock_node(combinator_node_name(combinator_names.COMBINATOR_CENTAUR_MAIN_SCREEN, Stage.RUN), generate_handler_combinator_response_with_continue_args())
        apphost_stubber.mock_node(combinator_node_name(combinator_names.COMBINATOR_CENTAUR_MAIN_SCREEN, Stage.CONTINUE), generate_handler_combinator_response())
        apphost_stubber.mock_node(scenario_node_name(scenario_names.HOLLYWOODMUSIC_SCENARIO, Stage.RUN), generate_handler_run_response_with_continue_args())
        apphost_stubber.mock_node(scenario_node_name(scenario_names.HOLLYWOODMUSIC_SCENARIO, Stage.CONTINUE), generate_handler_apply_response_with_output_speech('text'))

        def checker_finalize(ctx):
            assert ctx.get_protobuf_item(AH_ITEM_COMBINATOR_MAIN_SCREEN_RESPONSE_RUN) is not None
            assert ctx.get_protobuf_item(AH_ITEM_COMBINATOR_MAIN_SCREEN_RESPONSE_CONTINUE) is not None

        def checker_music_continue(ctx):
            assert ctx.get_protobuf_item(AH_ITEM_START_MUSIC_CONTINUE) is not None

        def check_combinator_input_scenarios_run(ctx):
            assert ctx.get_protobuf_item(AH_ITEM_SCENARIO_WEATHER_RUN_RESPONSE) is not None
            assert ctx.get_protobuf_item(AH_ITEM_SCENARIO_MUSIC_RUN_RESPONSE) is not None

        def check_combinator_input_scenarios_continue(ctx):
            assert ctx.get_protobuf_item(AH_ITEM_SCENARIO_WEATHER_RUN_RESPONSE) is not None
            assert ctx.get_protobuf_item(AH_ITEM_SCENARIO_MUSIC_RUN_RESPONSE) is not None
            assert ctx.get_protobuf_item(AH_ITEM_SCENARIO_MUSIC_CONTINUE_RESPONSE) is not None

        apphost_stubber.assert_node_input(NODE_NAME_WALKER_FINALIZE, checker_finalize)
        apphost_stubber.assert_node_input(scenario_node_name(scenario_names.HOLLYWOODMUSIC_SCENARIO, Stage.CONTINUE), checker_music_continue)
        apphost_stubber.assert_node_input(combinator_node_name(combinator_names.COMBINATOR_CENTAUR_MAIN_SCREEN, Stage.RUN), check_combinator_input_scenarios_run)
        apphost_stubber.assert_node_input(combinator_node_name(combinator_names.COMBINATOR_CENTAUR_MAIN_SCREEN, Stage.CONTINUE), check_combinator_input_scenarios_continue)

        alice(query)
