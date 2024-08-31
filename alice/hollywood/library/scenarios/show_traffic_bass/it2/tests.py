import logging
import json

import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import callback, voice
from alice.hollywood.library.python.testing.it2.stubber import create_localhost_bass_stubber_fixture
from google.protobuf import json_format


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
    'mm_enable_protocol_scenario=ShowTrafficBass',
    'traffic_cards',
    'traffic_cards_with_text_bubble'
]


@pytest.mark.scenario(name=SCENARIO_NAME, handle=SCENARIO_HANDLE)
@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.parametrize('surface', [surface.searchapp, surface.navi, surface.loudspeaker])
class TestShowTrafficBass:

    def test_usual(self, alice):
        response = alice(voice('покажи пробки'))
        assert response.scenario_stages() == {'run'}
        return str(response)

    def test_ellipsis(self, alice):
        response = alice(voice('пробки в Москве'))
        assert response.scenario_stages() == {'run'}

        response = alice(voice('а в Казани'))
        assert response.scenario_stages() == {'run'}
        return str(response)

    def test_details(self, alice):
        response = alice(voice('пробки в Уфе'))
        assert response.scenario_stages() == {'run'}

        response = alice(voice('на карте'))
        return str(response)

    def test_no_traffic_info(self, alice):
        response = alice(voice('пробки в Новой Зеландии'))
        assert response.scenario_stages() == {'run'}
        return str(response)

    def test_without_score(self, alice):
        response = alice(voice('пробки в Ишимбае'))
        assert response.scenario_stages() == {'run'}
        return str(response)


def _proto_to_dict(proto_message):
    json_str = json_format.MessageToJson(proto_message)
    return json.loads(json_str)


def _prepare_callback(callback):
    result = _proto_to_dict(callback)
    result.setdefault('payload', {}).update({
        '@scenario_name': 'ShowTrafficBass'
    })
    return result


@pytest.mark.scenario(name=SCENARIO_NAME, handle=SCENARIO_HANDLE)
@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestFeedback:

    def test_negative_feedback(self, alice):
        response = alice(voice('пробки'))
        feedback_callback = response.run_response.ResponseBody.FrameActions['suggest_feedback_negative_show_traffic'].Callback
        data = _prepare_callback(feedback_callback)
        response = alice(callback(feedback_callback.Name, data['payload']))
        return str(response)
