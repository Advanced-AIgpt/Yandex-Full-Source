import re
import itertools

import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.mark.experiments(
    'bg_fresh_granet_prefix=alice.scenarios.get_weather',
    'weather_use_wind_scenario',
)
@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestsWind:

    number = r'\d+'  # целое положительное число
    day_parts = ['утром', 'днём', 'вечером', 'ночью', 'Утром', 'Днём', 'Вечером', 'Ночью']
    expect_words = ['возможны', 'ожидайте', 'будут']
    now_phrases = ['Сейчас', 'В настоящее время', 'В настоящий момент', 'В данный момент', 'В данную минуту']
    wind_dirs = ['северный', 'северо-восточный', 'восточный', 'юго-восточный', 'южный', 'юго-западный', 'западный', 'северо-западный']
    wind_strengths = ['штиль', 'слабый', '', 'сильный', 'ураганный']
    wind_gusts = [fr' с порывами до {number} м/с', '']
    winds = [f'{strength} {dir} ветер' for (strength, dir) in itertools.product(wind_strengths, wind_dirs)] + ['штиль']

    basic_when = ['сейчас', 'сегодня', 'завтра', 'послезавтра', 'сегодня вечером', 'завтра утром']

    def test_today(self, alice):
        r = alice(voice('какой ветер в москве'))
        response = r.run_response.ResponseBody.Layout.OutputSpeech
        assert any(re.match(fr'{now} в Москве {wind}, {self.number} м/с{gust}\. {dp} {expect} порывы до {self.number} м/с.*', response)
                   for now in self.now_phrases for wind in self.winds for gust in self.wind_gusts for dp in self.day_parts for expect in self.expect_words)

        return response

    def test_now(self, alice):
        r = alice(voice('какой ветер в казани сейчас'))
        return r.run_response.ResponseBody.Layout.OutputSpeech

    def test_day_part(self, alice):
        r = alice(voice('какой ветер в красноярске завтра вечером'))
        return r.run_response.ResponseBody.Layout.OutputSpeech

    def test_day(self, alice):
        r = alice(voice('какой ветер завтра в токио'))
        return r.run_response.ResponseBody.Layout.OutputSpeech

    def test_days_range(self, alice):
        r = alice(voice('ветер в бостоне завтра и послезавтра'))
        return r.run_response.ResponseBody.Layout.OutputSpeech

    def test_ellipsis_time(self, alice):
        alice(voice('какой сегодня ветер в красноярске'))
        r = alice(voice('а завтра'))
        response = r.run_response.ResponseBody.Layout.OutputSpeech
        assert response.startswith('Завтра в Красноярске')
        assert 'ветер' in response or 'штиль' in response
        return response

    def test_ellipsis_place(self, alice):
        alice(voice('какой завтра ветер в казани'))
        r = alice(voice('а в москве'))
        response = r.run_response.ResponseBody.Layout.OutputSpeech
        assert response.startswith('Завтра в Москве')
        assert 'ветер' in response or 'штиль' in response
        return response

    def test_suggests(self, alice):
        suggests_list = [alice(voice('ветер ' + when)).run_response.ResponseBody.Layout.SuggestButtons for when in self.basic_when]
        assert all([suggests for suggests in suggests_list])
        return '\n\n'.join([str(suggests) for suggests in suggests_list])

    def test_long_range(self, alice):
        r = alice(voice('прогноз ветра на десять дней'))
        speech = r.run_response.ResponseBody.Layout.OutputSpeech

        # there should be info about 10 days
        assert len(speech.split('\n')) == 10

        return speech
