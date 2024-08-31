import pytest
from alice.hollywood.library.python.testing.it2 import auth
from alice.hollywood.library.python.testing.it2.input import voice

import env_states
import predefined_contacts
from conftest import TestVideoCallBase


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.supported_features('phone_address_book')
@pytest.mark.contacts(predefined_contacts.CONTACTS)
@pytest.mark.experiments('bg_beggins_video_call_mute_mic', 'bg_beggins_video_call_unmute_mic')
class TestsCallControls(TestVideoCallBase):

    @pytest.mark.parametrize('', [
        pytest.param(id='current', marks=pytest.mark.environment_state(
            env_states.env_state_with_current_call(mic_muted = False, video_enabled = True))),
        pytest.param(id='outgoing', marks=pytest.mark.environment_state(
            env_states.env_state_with_outgoing_call(mic_muted = False, video_enabled = True)))
    ])
    def test_mute_mic(self, alice):
        response = alice(voice('выключи микрофон'))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        assert not layout.output_speech
        directives = layout.Directives
        assert len(directives) == 1
        assert layout.Directives[0].HasField('MuteMicDirective')

        return str(response)

    @pytest.mark.parametrize('', [
        pytest.param(id='current', marks=pytest.mark.environment_state(
            env_states.env_state_with_current_call(mic_muted = True, video_enabled = True))),
        pytest.param(id='outgoing', marks=pytest.mark.environment_state(
            env_states.env_state_with_outgoing_call(mic_muted = True, video_enabled = True)))
    ])
    def test_unmute_mic(self, alice):
        response = alice(voice('включи микрофон'))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        assert not layout.output_speech
        directives = layout.Directives
        assert len(directives) == 1
        assert layout.Directives[0].HasField('UnmuteMicDirective')

        return str(response)

    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    def test_mute_mic_not_in_call(self, alice):
        response = alice(voice('выключи микрофон'))
        assert response.scenario_stages() == {'run'}
        assert response.is_run_irrelevant()

    @pytest.mark.environment_state(env_states.ENV_STATE_WITH_LOGIN)
    def test_unmute_mic_not_in_call(self, alice):
        response = alice(voice('включи микрофон'))
        assert response.scenario_stages() == {'run'}
        assert response.is_run_irrelevant()
