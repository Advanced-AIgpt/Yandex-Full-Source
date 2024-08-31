import pytest

from alice.megamind.mit.library.common.names.item_names import (
    AH_ITEM_SCENARIO_REQUEST,
    AH_ITEM_SCENARIO_RESPONSE,
)
from alice.megamind.mit.library.common.names.node_names import scenario_node_name, Stage
import alice.megamind.mit.library.common.names.scenario_names as scenario_names
from alice.megamind.mit.library.request_builder import (
    MegamindRequestBuilder,
    Voice,
)
from alice.megamind.mit.library.util import get_dummy_proto
from alice.megamind.mit.tests.scenario_state.proto.test_state_pb2 import TTestState
from alice.megamind.protos.scenarios.request_pb2 import TScenarioApplyRequest, TScenarioRunRequest
from alice.megamind.protos.scenarios.response_pb2 import (
    TScenarioApplyResponse,
    TScenarioRunResponse,
)


class TestScenarioState(object):
    @pytest.mark.experiments('mm_subscribe_to_frame=alice.random_number:TestScenario')
    @pytest.mark.experiments('mm_scenario=TestScenario')
    @pytest.mark.parametrize('stage', [Stage.CONTINUE, Stage.COMMIT, Stage.APPLY])
    def test_scenario_state(self, alice, apphost_stubber, stage):
        builder = MegamindRequestBuilder(Voice('рандомное число от 1 до 1000?'))
        expected_answer = 'state is safe'

        expected_state = TTestState()
        expected_state.CheckPhrase = 'do not break scenario state'
        builder.set_scenario_state(scenario_names.TEST_SCENARIO, expected_state)

        def check_state(ctx, response_cls):
            request = response_cls()
            request.ParseFromString(ctx.get_protobuf_item(AH_ITEM_SCENARIO_REQUEST))

            state = TTestState()
            request.BaseRequest.State.Unpack(state)
            assert state.CheckPhrase == expected_state.CheckPhrase

        def run_handler(ctx):
            check_state(ctx, TScenarioRunRequest)

            response = TScenarioRunResponse()
            if stage == Stage.APPLY:
                response.ApplyArguments.Pack(get_dummy_proto())
            elif stage == Stage.COMMIT:
                response.CommitCandidate.ResponseBody.Layout.OutputSpeech = expected_answer
                response.CommitCandidate.Arguments.Pack(get_dummy_proto())
            elif stage == Stage.CONTINUE:
                response.ContinueArguments.Pack(get_dummy_proto())
            ctx.add_protobuf_item(AH_ITEM_SCENARIO_RESPONSE, response)

        apphost_stubber.mock_node(scenario_node_name(scenario_names.TEST_SCENARIO, Stage.RUN), run_handler)

        def apply_handler(ctx):
            check_state(ctx, TScenarioApplyRequest)

            response = TScenarioApplyResponse()
            response.ResponseBody.Layout.OutputSpeech = expected_answer
            ctx.add_protobuf_item(AH_ITEM_SCENARIO_RESPONSE, response)

        apphost_stubber.mock_node(scenario_node_name(scenario_names.TEST_SCENARIO, stage), apply_handler)

        response = alice(builder)
        if stage != Stage.CONTINUE:
            response = alice(builder.apply(response))
        assert response.output_speech == expected_answer
