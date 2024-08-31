import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import server_action


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['fast_command']


@pytest.mark.scenario(name='Commands', handle='fast_command')
@pytest.mark.parametrize('surface', [surface.station])
class TestMediaPlay:

    def test_stereo_pair_jingle(self, alice):
        payload = {
            'typed_semantic_frame': {
                'media_play_semantic_frame': {
                    'tune_id': {
                        'string_value': 'stereopair_ready',
                    },
                    'location_id': {
                        'string_value': 'stereopair',
                    },
                },
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'stereopair',
            },
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 2

        assert directives[0].HasField('StartMultiroomDirective')
        start_multiroom = directives[0].StartMultiroomDirective
        assert start_multiroom.RoomId == 'stereopair'

        assert directives[1].HasField('AudioPlayDirective')
        audio_play = directives[1].AudioPlayDirective
        assert audio_play.Stream.Url == 'https://storage.mds.yandex.net/get-music-shots/3472782/stereoready_1632489505.mp3'
