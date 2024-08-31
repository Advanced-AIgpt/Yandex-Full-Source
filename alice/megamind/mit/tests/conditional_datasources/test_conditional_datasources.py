import pytest

from alice.megamind.mit.library.common.names.node_names import scenario_node_name, Stage
import alice.megamind.mit.library.common.names.scenario_names as scenario_names
import alice.megamind.mit.library.common.names.item_names as item_names
from alice.megamind.mit.library.request_builder import Voice


@pytest.mark.experiments('mm_subscribe_to_frame=alice.random_number:TestScenario')
@pytest.mark.experiments('mm_subscribe_to_frame=personal_assistant.scenarios.bluetooth_on:TestScenario')
@pytest.mark.experiments('mm_scenario=TestScenario')
class TestConditionalDatasources(object):
    def test_conditional_datasource_websearch(self, alice, apphost_stubber):
        def checker(ctx):
            assert ctx.get_protobuf_item(item_names.AH_ITEM_DATASOURCE_WEBSEARCH_RENDERRER) is not None

        apphost_stubber.assert_node_input(scenario_node_name(scenario_names.TEST_SCENARIO, Stage.RUN), checker)

        alice(Voice('включи блютуз'))

    def test_conditional_datasource_websearch_none(self, alice, apphost_stubber):
        def checker(ctx):
            has_item = False
            try:
                ctx.get_protobuf_item(item_names.AH_ITEM_DATASOURCE_WEBSEARCH_RENDERRER)
                has_item = True
            except:
                has_item = False
            assert not has_item

        apphost_stubber.assert_node_input(scenario_node_name(scenario_names.TEST_SCENARIO, Stage.RUN), checker)

        alice(Voice('рандомное число от 1 до 1000?'))
