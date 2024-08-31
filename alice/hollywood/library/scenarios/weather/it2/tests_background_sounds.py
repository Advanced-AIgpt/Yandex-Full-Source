import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.mark.experiments('weather_enable_background_sounds')
@pytest.mark.scenario(name='Weather', handle='weather')
class TestsBackgroundSounds:

    @pytest.mark.parametrize('surface', [surface.station_pro])
    def test_smoke(self, alice):
        '''
        Перед переканонизацией надо найти город, где сейчас идет дождь или другие осадки
        на момент переканонизации
        '''
        r = alice(voice('погода в перми'))
        text = r.run_response.ResponseBody.Layout.Cards[0].Text
        output_speech = r.run_response.ResponseBody.Layout.OutputSpeech
        return f'Text: "{text}"\nOutputSpeech: "{output_speech}"'
