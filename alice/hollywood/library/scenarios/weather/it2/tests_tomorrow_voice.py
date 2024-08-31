import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestsTomorrowForecastVoice:
    # a large bunch of cities to provide better coverage
    cities = ['москве', 'екатеринбурге', 'санкт-петербурге', 'великом новгороде',
              'сочи', 'калининграде', 'челябинске', 'архангельске', 'иркутске',
              'лондоне', 'берлине', 'хельсинки', 'таллине', 'минске', 'киеве',
              'риге', 'будапеште', 'тбилиси', 'киото', 'пекине', 'пхеньяне']
    request = 'погода завтра в '

    def test_experiment_off(self, alice):
        result = [str(alice(voice(self.request + city)).run_response.ResponseBody.Layout.OutputSpeech) for city in self.cities]
        return '\n'.join(result)

    @pytest.mark.experiments('weather_tomorrow_forecast_warning')
    def test_experiment_on(self, alice):
        result = [str(alice(voice(self.request + city)).run_response.ResponseBody.Layout.OutputSpeech) for city in self.cities]
        return '\n'.join(result)
