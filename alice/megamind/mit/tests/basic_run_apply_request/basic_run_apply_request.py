import pytest

from alice.megamind.mit.library.common.names.node_names import scenario_node_name, Stage
import alice.megamind.mit.library.common.names.scenario_names as scenario_names
from alice.megamind.mit.library.common.util.scenario.responses import (
    generate_handler_run_response_with_apply_args,
    generate_handler_apply_response_with_output_speech,
)
from alice.megamind.mit.library.request_builder import (
    MegamindRequestBuilder,
    Voice,
)
from alice.megamind.protos.scenarios.request_pb2 import (
    TScenarioApplyRequest,
)


RANDOM_NUMBER_FRAME_NAME = 'alice.random_number'


class TestBasicRunApplyRequest(object):
    @pytest.mark.experiments(f'mm_subscribe_to_frame={RANDOM_NUMBER_FRAME_NAME}:TestScenario')
    @pytest.mark.experiments('mm_scenario=TestScenario')
    @pytest.mark.parametrize('drop_init_in_post_classify', [True, False])
    def test_basic_run_apply_request(self, alice, apphost_stubber, drop_init_in_post_classify):
        request_builder = MegamindRequestBuilder(Voice('рандомное число от 1 до 1000?'))
        if drop_init_in_post_classify:
            request_builder.add_experiments({'mm_drop_init_scenarios_in_post_classify': '1'})
        apphost_stubber.mock_node(scenario_node_name(scenario_names.TEST_SCENARIO, Stage.RUN),
                                  generate_handler_run_response_with_apply_args())
        response = alice(request_builder)
        expected_utterance = '4 8 15 16 23 42'

        def check_semantic_frames(apply_request):
            have_random_number_sf = False
            for semantic_frame in apply_request.Input.SemanticFrames:
                have_random_number_sf = have_random_number_sf or semantic_frame.Name == RANDOM_NUMBER_FRAME_NAME
            assert have_random_number_sf, 'No random number frames in TInput::SemanticFrames of apply request to scenario'

        def check_arguments(apply_request):
            assert apply_request.Arguments, 'No Arguments in apply request to scenario'

        def check_client_info(apply_request):
            assert apply_request.BaseRequest.ClientInfo.AppId, 'No BaseRequest::ClientInfo::AppId in apply request to scenario'

        def check_apply_request(ctx):
            request = TScenarioApplyRequest()
            request.ParseFromString(ctx.get_protobuf_item(b'mm_scenario_request'))
            check_semantic_frames(request)
            check_arguments(request)
            check_client_info(request)

        apphost_stubber.assert_node_input(scenario_node_name(scenario_names.TEST_SCENARIO, Stage.APPLY),
                                          check_apply_request)
        apphost_stubber.mock_node(scenario_node_name(scenario_names.TEST_SCENARIO, Stage.APPLY),
                                  generate_handler_apply_response_with_output_speech(expected_utterance))
        response = alice(request_builder.apply(response))
        assert response.output_speech == expected_utterance
