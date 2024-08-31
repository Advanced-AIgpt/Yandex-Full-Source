import alice.tests.library.directives as directives
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestBluetoothPlayer(object):

    owners = ('vitvlkv', )

    device_state = {
        'bluetooth': {
            'player': {
                'pause': False
            },
            'currently_playing': {
                'track_info': {
                    'title': 'Skyfall',
                    'artists': [{
                        'name': 'Our Last Night'
                    }],
                    'id': 'stub',
                    'albums': [{
                        'title': 'Age Of Ignorance'
                    }],
                }
            },
            'last_play_timestamp': 1639395944627,
        },
        'audio_player': {
            'last_play_timestamp': 1639332688075,
            'player_state': 'Stopped',
        },
    }

    def test_next_track(self, alice):
        response = alice('следующий трек')
        assert response.directive.name == directives.names.PlayerNextTrackDirective

    def test_what_is_playing(self, alice):
        response = alice('что играет')
        assert 'Skyfall' in response.text

    def test_shuffle(self, alice):
        response = alice('перемешай')
        assert not response.directive
        assert response.text == 'Пока я умею такое только в Яндекс.Музыке.'


@pytest.mark.experiments('enable_shuffle_in_hw_music')
class TestBluetoothPlayerExp(TestBluetoothPlayer):
    pass
