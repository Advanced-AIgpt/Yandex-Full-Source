import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.mark.experiments(
    'bg_fresh_granet_prefix=alice.scenarios.get_weather',
    'weather_use_pressure_scenario',
)
@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestsPressure:

    basic_when = ['сейчас', 'сегодня', 'завтра', 'послезавтра', 'сегодня вечером', 'завтра утром']

    def test_today(self, alice):
        r = alice(voice('какое давление в красноярске'))
        return r.run_response.ResponseBody.Layout.OutputSpeech

    def test_now(self, alice):
        r = alice(voice('какое сейчас давление в барселоне'))
        return r.run_response.ResponseBody.Layout.OutputSpeech

    def test_day_part(self, alice):
        r = alice(voice('завтра вечером в брюгге какое атмосферное давление'))
        return r.run_response.ResponseBody.Layout.OutputSpeech

    def test_day(self, alice):
        r = alice(voice('прогноз давления на завтра в берлине'))
        return r.run_response.ResponseBody.Layout.OutputSpeech

    def test_days_range(self, alice):
        r = alice(voice('давление в париже завтра и послезавтра'))
        return r.run_response.ResponseBody.Layout.OutputSpeech

    def test_ellipsis_time(self, alice):
        alice(voice('давление в питере'))
        r = alice(voice('а завтра'))
        response = r.run_response.ResponseBody.Layout.OutputSpeech
        assert response.startswith('Завтра в Санкт-Петербурге')
        assert 'давление' in response
        return response

    def test_ellipsis_place(self, alice):
        alice(voice('давление завтра в питере'))
        r = alice(voice('а в москве'))
        response = r.run_response.ResponseBody.Layout.OutputSpeech
        assert response.startswith('Завтра в Москве')
        assert 'давление' in response
        return response

    def test_suggests(self, alice):
        suggests_list = [alice(voice('давление ' + when)).run_response.ResponseBody.Layout.SuggestButtons for when in self.basic_when]
        assert all([suggests for suggests in suggests_list])
        return '\n\n'.join([str(suggests) for suggests in suggests_list])
