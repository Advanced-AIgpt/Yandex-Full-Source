import pytest

from alice.megamind.mit.library.common.names.node_names import scenario_node_name, Stage
import alice.megamind.mit.library.common.names.scenario_names as scenario_names
import alice.megamind.mit.library.common.names.item_names as item_names
from alice.megamind.mit.library.common.util.scenario.responses import (
    generate_handler_run_response_with_continue_args
)
from alice.megamind.mit.library.request_builder import Voice


@pytest.mark.experiments('mm_enable_parallel_continue')
class TestSimpleParallelContinue(object):
    @pytest.mark.experiments('mm_subscribe_to_frame=alice.random_number:ImageWhatIsThis')
    @pytest.mark.experiments('mm_subscribe_to_frame=alice.random_number:GeneralConversation')
    @pytest.mark.experiments('mm_subscribe_to_frame=alice.random_number:Afisha')
    @pytest.mark.experiments('mm_enable_protocol_scenario=Afisha')
    def test_simple_parallel_continue(self, alice, apphost_stubber):
        apphost_stubber.mock_node(scenario_node_name(scenario_names.IMAGEWHATISTHIS_SCENARIO, Stage.RUN),
                                  generate_handler_run_response_with_continue_args())
        apphost_stubber.mock_node(scenario_node_name(scenario_names.GENERALCONVERSATION_SCENARIO, Stage.RUN),
                                  generate_handler_run_response_with_continue_args())
        # apphost_stubber.mock_node(scenario_node_name(scenario_names.AFISHA_SCENARIO, Stage.RUN),
        #                           generate_handler_run_response_with_continue_args(is_http_proxy=True))

        def check_continue_request(ctx):
            assert ctx.get_protobuf_item(item_names.AH_ITEM_SCENARIO_REQUEST) is not None

        apphost_stubber.assert_node_input(scenario_node_name(scenario_names.IMAGEWHATISTHIS_SCENARIO, Stage.CONTINUE), check_continue_request)
        apphost_stubber.assert_node_input(scenario_node_name(scenario_names.GENERALCONVERSATION_SCENARIO, Stage.CONTINUE), check_continue_request)
        # apphost_stubber.assert_node_input(scenario_node_name(scenario_names.AFISHA_SCENARIO, Stage.CONTINUE), check_continue_request)

        alice(Voice('рандомное число от 1 до 1000?'))

    @pytest.mark.experiments('mm_subscribe_to_frame=alice.random_number:HollywoodMusic')
    @pytest.mark.experiments('mm_subscribe_to_frame=alice.random_number:GeneralConversation')
    def test_block_music_parallel_continue(self, alice, apphost_stubber):
        apphost_stubber.mock_node(scenario_node_name(scenario_names.HOLLYWOODMUSIC_SCENARIO, Stage.RUN),
                                  generate_handler_run_response_with_continue_args())
        apphost_stubber.mock_node(scenario_node_name(scenario_names.GENERALCONVERSATION_SCENARIO, Stage.RUN),
                                  generate_handler_run_response_with_continue_args())

        def check_has_continue_request(ctx):
            assert ctx.get_protobuf_item(item_names.AH_ITEM_SCENARIO_REQUEST) is not None

        def check_not_visited(ctx):
            assert False

        apphost_stubber.assert_node_input(scenario_node_name(scenario_names.HOLLYWOODMUSIC_SCENARIO, Stage.CONTINUE), check_not_visited, True)
        apphost_stubber.assert_node_input(scenario_node_name(scenario_names.GENERALCONVERSATION_SCENARIO, Stage.CONTINUE), check_has_continue_request)

        alice(Voice('рандомное число'))

    @pytest.mark.experiments('mm_subscribe_to_frame=personal_assistant.scenarios.music_play:GeneralConversation')
    def test_run_music_parallel_continue(self, alice, apphost_stubber):
        apphost_stubber.mock_node(scenario_node_name(scenario_names.HOLLYWOODMUSIC_SCENARIO, Stage.RUN),
                                  generate_handler_run_response_with_continue_args())
        apphost_stubber.mock_node(scenario_node_name(scenario_names.GENERALCONVERSATION_SCENARIO, Stage.RUN),
                                  generate_handler_run_response_with_continue_args())

        def check_has_continue_request(ctx):
            assert ctx.get_protobuf_item(item_names.AH_ITEM_SCENARIO_REQUEST) is not None

        apphost_stubber.assert_node_input(scenario_node_name(scenario_names.HOLLYWOODMUSIC_SCENARIO, Stage.CONTINUE), check_has_continue_request)
        apphost_stubber.assert_node_input(scenario_node_name(scenario_names.GENERALCONVERSATION_SCENARIO, Stage.CONTINUE), check_has_continue_request)

        alice(Voice('включи музыку'))
