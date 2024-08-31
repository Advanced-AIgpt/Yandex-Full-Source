import pytest

from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


TEST_DEVICE_ID = 'device_id_1'

ENV_STATE_NOTIFICATION_ANIMATION = {
    'endpoints': [
        {
            'id': TEST_DEVICE_ID,
            'capabilities': [
                {
                    'state': {
                        'screen_states_map': {
                            'test_guid': {
                                'animation': {
                                    'animation_type': 'Notification'
                                }
                            }
                        }
                    },
                    '@type': 'type.googleapis.com/NAlice.TAnimationCapability',
                },
            ],
        },
    ],
}

ENV_STATE_ANOTHER_ANIMATION = {
    'endpoints': [
        {
            'id': TEST_DEVICE_ID,
            'capabilities': [
                {
                    'state': {
                        'screen_states_map': {
                            'test_guid': {
                                'animation': {
                                    'animation_type': 'UnknownAnimationType'
                                }
                            }
                        }
                    },
                    '@type': 'type.googleapis.com/NAlice.TAnimationCapability',
                },
            ],
        },
    ],
}


@pytest.fixture(scope="module")
def enabled_scenarios():
    return ['notifications_manager']


@pytest.mark.scenario(name='NotificationsManager', handle='notifications_manager')
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsAnimation:
    @pytest.mark.environment_state(ENV_STATE_NOTIFICATION_ANIMATION)
    @pytest.mark.device_state(device_id=TEST_DEVICE_ID)
    def test_with_notification(self, alice):
        r = alice(voice('есть обновления'))
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.Features.PlayerFeatures.SecondsSincePause == 0
        assert 'Это я даю вам понять, что у вас есть непрослушанные уведомления' in r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.environment_state(ENV_STATE_ANOTHER_ANIMATION)
    @pytest.mark.device_state(device_id=TEST_DEVICE_ID)
    def test_no_animation(self, alice):
        r = alice(voice('есть обновления'))
        assert r.run_response.Features.PlayerFeatures.RestorePlayer
        assert r.run_response.Features.PlayerFeatures.SecondsSincePause == 1
        assert 'Это я даю вам понять, что у вас есть непрослушанные уведомления' in r.run_response.ResponseBody.Layout.OutputSpeech

    def test_no_capability(self, alice):
        r = alice(voice('есть обновления'))
        assert not r.run_response.Features.PlayerFeatures.RestorePlayer
        assert 'Это я даю вам понять, что у вас есть непрослушанные уведомления' in r.run_response.ResponseBody.Layout.OutputSpeech
