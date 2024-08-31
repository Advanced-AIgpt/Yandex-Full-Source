import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice('voice_with_side_speech')
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('mm_side_speech_threshold=-1000')
class TestSideSpeech(object):

    owners = ('flimsywhimsy', )

    @pytest.mark.parametrize('command', [
        'пожалуйста не бегать я вас прошу',
        'а зачем ты его с чехла достала',
    ])
    def test_side_speech(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.SideSpeech
