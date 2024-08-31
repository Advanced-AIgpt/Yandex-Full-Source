import pytest
from alice.hollywood.library.python.testing.it2 import auth
from alice.hollywood.library.python.testing.it2.input import server_action

import endpoint_state_updates
import env_states
from conftest import TestVideoCallBase


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.supported_features('phone_address_book')
class TestsEndpointUpdate(TestVideoCallBase):

    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    def test_endpoint_state_update_with_video_call(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload=endpoint_state_updates.VIDEO_CALL_STATE_UPDATE_FRAME))
        assert response.scenario_stages() == {'run'}
        assert response.is_run_relevant()
        assert not response.run_response.ResponseBody.Layout.OutputSpeech
        return str(response)

    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    def test_endpoint_state_update_without_video_call(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload=endpoint_state_updates.IOT_STATE_UPDATE_FRAME))
        assert response.scenario_stages() == {'run'}
        assert response.is_run_irrelevant()
        return str(response)
