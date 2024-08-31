import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestsNowForecastVoice:
    # a large bunch of cities to provide better coverage
    cities = ['москве', 'санкт-петербурге', 'екатеринбурге', 'сочи', 'краснодаре', 'барнауле',
              'лондоне', 'берлине', 'минске', 'таллине', 'хельсинки', 'пекине', 'пхеньяне',
              'киото', 'нурсултане', 'муроме', 'кургане', 'челябинске', 'иркутске', 'петрозаводске',
              'орске', 'вологда', 'казани', 'ростове на дону', 'нижневартовске', 'ставрополе',
              'сургуте', 'ульяновске', 'новороссийске', 'калининграде', 'кирове', 'архангельске']
    request = 'погода сейчас в '

    def test_experiment_off(self, alice):
        result = [str(alice(voice(self.request + city)).run_response.ResponseBody.Layout.OutputSpeech) for city in self.cities]
        return '\n'.join(result)

    @pytest.mark.experiments('weather_now_forecast_warning', 'weather_now_significance_threshold=80')
    def test_experiment_on_all(self, alice):
        result = [str(alice(voice(self.request + city)).run_response.ResponseBody.Layout.OutputSpeech) for city in self.cities]
        return '\n'.join(result)

    @pytest.mark.experiments(
        'weather_now_forecast_warning',
        'weather_now_significance_threshold=0.8',
        'weather_now_only_significant',
    )
    def test_experiment_on_threshold(self, alice):
        result = [str(alice(voice(self.request + city)).run_response.ResponseBody.Layout.OutputSpeech) for city in self.cities]
        return '\n'.join(result)
