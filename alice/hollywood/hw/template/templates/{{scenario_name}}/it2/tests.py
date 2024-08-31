import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.scenarios.{{scenario_name}}.proto.{{scenario_name}}_pb2 import T{{ScenarioName}}State # noqa


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['{{scenario_name}}']


@pytest.mark.experiments('bg_fresh_granet_form={{frame_name}}')
@pytest.mark.scenario(name='{{ScenarioName}}', handle='{{scenario_name}}')
@pytest.mark.parametrize('surface', surface.actual_surfaces)
class Tests:

    def test_hello_world(self, alice):
        r = alice(voice('Включи {{scenario_name}}'))
        assert r.scenario_stages() == {'run'}
        analytics_info = r.run_response.ResponseBody.AnalyticsInfo
        assert analytics_info.ProductScenarioName == '{{scenario_name}}'
        assert analytics_info.Intent == '{{frame_name}}'
        layout = r.run_response.ResponseBody.Layout
        assert len(layout.Directives) == 0
        assert layout.Cards[0].Text in ['Hello, Word']
        assert layout.OutputSpeech == layout.Cards[0].Text

