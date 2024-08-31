import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['fast_command']


analytics = {
    'product_scenario': 'smart_device_external_app',
    'purpose': 'youtube_mediasession_play',
    'origin': 'SmartSpeaker'
}

active_actions = {
    'semantic_frames': {
        'personal_assistant.scenarios.player.continue': {
            'typed_semantic_frame': {
                'media_session_play_semantic_frame': {
                    'media_session_id': {
                        'string_value': 'youtube.webview'
                    }
                }
            },
            'analytics': analytics
        },
        'personal_assistant.scenarios.player.pause': {
            'typed_semantic_frame': {
                'media_session_pause_semantic_frame': {
                    'media_session_id': {
                        'string_value': 'youtube.webview'
                    }
                }
            },
            'analytics': analytics
        }
    }
}


@pytest.mark.scenario(name='Commands', handle='fast_command')
@pytest.mark.device_state(active_actions=active_actions)
@pytest.mark.parametrize('surface', [surface.smart_display])
class TestMediaSession:

    def test_media_session_play(self, alice):
        r = alice(voice('продолжи'))
        assert r.scenario_stages() == {'run'}

        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('WebViewMediaSessionPlayDirective')

        return str(r)

    def test_media_session_pause(self, alice):
        r = alice(voice('пауза'))
        assert r.scenario_stages() == {'run'}

        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('WebViewMediaSessionPauseDirective')

        return str(r)
