from google.protobuf import text_format
import pytest

from alice.megamind.library.stack_engine.protos.stack_engine_pb2 import TStackEngineCore
import alice.megamind.mit.library.common.names.directives as directive_names
from alice.megamind.mit.library.common.names.node_names import scenario_node_name, Stage
import alice.megamind.mit.library.common.names.scenario_names as scenario_names
from alice.megamind.mit.library.common.util.scenario.responses import generate_handler_run_response_with_apply_args
from alice.megamind.mit.library.request_builder import (
    MegamindRequestBuilder,
    ServerAction,
)


STACK_ENGINE_CORE = '''
    Items {
        ScenarioName: 'TestScenario'
            Effect {
                ParsedUtterance {
                    TypedSemanticFrame {
                        TestSemanticFrame {
                            Dummy {
                                StringValue: 'test_warm_up'
                            }
                        }
                }
                Analytics {
                    Origin: Scenario
                    Purpose: 'test_scenario'
                }
            }
        }
    }
    SessionId: '28f803d9-dfe1-49c4-810f-cdd25bbc3ee2'
    ProductScenarioName: 'test_scenario'
    StackOwner: 'TestScenario'
'''

GET_NEXT_SERVER_ACTION_PAYLOAD = {
    '@request_id': 'e3ed70db-fd03-4b15-a74f-bb601db898b7',
    'stack_session_id': '28f803d9-dfe1-49c4-810f-cdd25bbc3ee2',
    '@scenario_name': 'TestScenario',
    'stack_product_scenario_name': 'test_scenario'
}


class TestWarmUp(object):
    @pytest.mark.parametrize('is_warm_up', [True, False])
    def test_warm_up_apply_with_tsf(self, alice, apphost_stubber, is_warm_up):
        query = ServerAction(directive_names.GET_NEXT, GET_NEXT_SERVER_ACTION_PAYLOAD, is_warm_up)
        stack_engine_proto = text_format.Parse(STACK_ENGINE_CORE, TStackEngineCore())
        request_builder = MegamindRequestBuilder(query).set_stack_engine(stack_engine_proto)

        apphost_stubber.mock_node(scenario_node_name(scenario_names.TEST_SCENARIO, Stage.RUN), generate_handler_run_response_with_apply_args())

        response = alice(request_builder)
        if is_warm_up:
            assert not response.get_directive(directive_names.DEFER_APPLY)
            assert response.get_directive(directive_names.GET_NEXT)
        else:
            assert response.get_directive(directive_names.DEFER_APPLY)
            assert not response.get_directive(directive_names.GET_NEXT)
