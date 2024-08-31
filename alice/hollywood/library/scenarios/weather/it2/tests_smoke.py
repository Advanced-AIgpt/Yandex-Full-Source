import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


def check_response(response):
    day_parts = ['утром', 'днём', 'вечером', 'ночью']
    for dp in day_parts:
        assert response.count(dp) + response.count(dp.capitalize()) < 2


@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.parametrize('surface', [surface.loudspeaker, surface.searchapp])
@pytest.mark.experiments(
    'weather_use_wind_scenario',
    'weather_use_pressure_scenario',
)
class TestsSmoke:
    cities = ['иркутске', 'москве', 'иркутске', 'казани', 'иркутске']
    regions_with_capitals = [('Пермском крае', 'Перми'), ('Казахстане', 'Нур-Султане'), ('Штате Гавайи', 'Гонолулу')]

    def test_basic(self, alice):
        r = alice(voice('погода в красноярске'))
        return str(r)

    def test_voice(self, alice):
        responses = [alice(voice('погода в ' + city)).run_response.ResponseBody.Layout.OutputSpeech for city in self.cities]
        for city, response in zip(self.cities, responses):
            assert city in response.lower()
            check_response(response)
        return '\n'.join(responses)

    def test_fallback_on_capital(self, alice):
        responses = []
        for region, capital in self.regions_with_capitals:
            response = alice(voice(f'погода в {region}')).run_response.ResponseBody.Layout.OutputSpeech
            assert region in response and capital in response
            responses.append(response)
        return '\n'.join(responses)

    def test_timezones(self, alice):
        responses = []
        for city in ['чикаго', 'москве', 'апиа', 'гонолулу', 'алофи']:
            response = alice(voice(f'погода завтра в {city}')).run_response.ResponseBody.Layout.OutputSpeech
            assert response.startswith('Завтра')
            responses.append(response)
        return '\n'.join(responses)

    @pytest.mark.experiments('weather_disable_ru_only_phrases')
    def test_experiment_weather_disable_ru_only_phrases(self, alice):
        r = alice(voice('погода в красноярске'))
        return str(r)


@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestsMisc:

    def test_rostov_simple(self, alice):
        '''
        "погода в ростове" - отвечаем про РОСТОВ-НА-ДОНУ
        '''
        r = alice(voice('погода в ростове'))
        output_speech = r.run_response.ResponseBody.Layout.OutputSpeech
        assert 'Сейчас в Ростове-на-Дону' in output_speech
        return output_speech

    def test_rostov_on_don(self, alice):
        '''
        "погода в ростове-на-дону" - отвечаем про РОСТОВ-НА-ДОНУ
        '''
        r = alice(voice('погода в ростове на дону'))
        output_speech = r.run_response.ResponseBody.Layout.OutputSpeech
        assert 'Сейчас в Ростове-на-Дону' in output_speech
        return output_speech

    def test_rostov_velikiy(self, alice):
        '''
        "погода в ростове великом" - отвечаем про РОСТОВ (он же РОСТОВ ВЕЛИКИЙ)
        '''
        r = alice(voice('погода в ростове великом'))
        output_speech = r.run_response.ResponseBody.Layout.OutputSpeech
        assert 'Сейчас в Ростове-на-Дону' not in output_speech
        assert 'Сейчас в Ростове' in output_speech
        return output_speech

    def test_novgorod_simple(self, alice):
        r = alice(voice('погода в новгороде'))
        output_speech = r.run_response.ResponseBody.Layout.OutputSpeech
        assert 'Сейчас в Великом Новгороде' in output_speech
        return output_speech

    def test_nizhny_novgorod(self, alice):
        r = alice(voice('погода в нижнем новгороде'))
        output_speech = r.run_response.ResponseBody.Layout.OutputSpeech
        assert 'Сейчас в Нижнем Новгороде' in output_speech
        return output_speech


@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.parametrize('surface', [surface.webtouch])
class TestsWebtouch:
    def test_simple(self, alice):
        r = alice(voice('погода в красноярске'))
        assert r.run_response.ResponseBody.Layout.Directives[0].OpenUriDirective.Uri


@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestsCityNoData:
    def test_simple(self, alice):
        r = alice(voice('погода в атлантиде на завтра'))
        assert r.run_response.ResponseBody.Layout.OutputSpeech == 'Извините, я не знаю, где это "в атлантиде".'
