import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import server_action, voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['tv_controls']


@pytest.mark.scenario(name='TvControls', handle='tv_controls')
@pytest.mark.parametrize('surface', [surface.smart_tv])
class Tests:
    @pytest.skip(reason="Not working begemot in megamind!")
    @pytest.mark.experiments('enable_screensaver', 'ignore_screensaver_capability', 'bg_fresh_granet=alice.controls.open_screensaver')
    def test_open_screensaver(self, alice):
        r = alice(voice('Открой заставку'))
        assert r.scenario_stages() == {'run'}
        analytics_info = r.run_response.ResponseBody.AnalyticsInfo
        assert analytics_info.ProductScenarioName == 'tv_controls'
        assert analytics_info.Intent == 'alice.controls.open_screensaver'
        layout = r.run_response.ResponseBody.Layout
        assert len(layout.Directives) == 1
        return str(r)

    def test_longtap_tutorial(self, alice):
        payload = {
            "typed_semantic_frame": {"tv_long_tap_tutorial_semantic_frame": {}},
            "analytics": {"origin": "SmartTv", "purpose": "long_tap_tutorial", "product_scenario": "TVPultPromo"},
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run'}
        assert (r.run_response.ResponseBody.Layout.OutputSpeech== 'Простите, я вас не поняла. Чтобы я вас услышала, нажмите и удерживайте кнопку на пульте, пока говорите.')
        return str(r)
