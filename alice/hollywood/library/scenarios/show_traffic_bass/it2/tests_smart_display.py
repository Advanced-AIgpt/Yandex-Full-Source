import logging

import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import server_action
from alice.hollywood.library.python.testing.it2.stubber import create_localhost_bass_stubber_fixture


logger = logging.getLogger(__name__)

bass_stubber = create_localhost_bass_stubber_fixture()

SCENARIO_HANDLE = 'show_traffic_bass'
SCENARIO_NAME = 'ShowTrafficBass'


@pytest.fixture(scope='module')
def enabled_scenarios():
    return [SCENARIO_HANDLE]


@pytest.fixture(scope='function')
def srcrwr_params(bass_stubber):
    return {
        'HOLLYWOOD_COMMON_BASS': f'localhost:{bass_stubber.port}',
    }


EXPERIMENTS = [
    'mm_enable_protocol_scenario=ShowTrafficBass'
]


@pytest.mark.scenario(name=SCENARIO_NAME, handle=SCENARIO_HANDLE)
@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.parametrize('surface', [surface.smart_display])
class TestSmartDisplay:

    def test_get_main_screen(self, alice):
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'centaur_collect_main_screen': {}
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'main_screen'
            }
        }))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.HasField('ScenarioData')
        scenario_data = r.run_response.ResponseBody.ScenarioData
        assert scenario_data.HasField('TrafficData')
        return str(r)

    def test_get_main_screen_widget(self, alice):
        r = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'centaur_collect_widget_gallery_semantic_frame': {}
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'main_screen'
            }
        }))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.HasField('ScenarioData')
        scenario_data = r.run_response.ResponseBody.ScenarioData
        assert scenario_data.HasField('TrafficData')
        return str(r)
