import re

import pytest
from alice.hollywood.library.python.testing.it2 import auth
from alice.hollywood.library.python.testing.it2.input import server_action

import env_states
from conftest import TestVideoCallBase


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.supported_features('phone_address_book')
class TestsLogin(TestVideoCallBase):

    @pytest.mark.environment_state(env_states.ENV_STATE_WITHOUT_LOGIN)
    def test_login_failed_callback(self, alice):
        response = alice(server_action(name='@@mm_semantic_frame', payload={
            'typed_semantic_frame': {
                'video_call_login_failed_semantic_frame': {
                    'provider': {
                        'enum_value': 'Telegram'
                    }
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'video_call_login_failed'
            }
        }))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        output_speech = 'Не удалось зайти в аккаунт'
        assert re.match(output_speech, layout.OutputSpeech)

        return str(response)
