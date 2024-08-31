import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.parametrize('surface', [surface.navi])
class TestsTTSExperiments:
    city = 'в москве'  # запрос в место, время начала и/или конца осадков описывается через часы и минуты
    # пример: Сегодня в Москве слабый дождь закончится около [12:20], температура от +5 до +7°, ветер 7м/с

    @pytest.mark.experiments('weather_today_forecast_warning')
    def test_experiment_tts_today(self, alice):
        r = alice(voice('погода сегодня ' + self.city))
        text = r.run_response.ResponseBody.Layout.Cards[0].Text
        vc = r.run_response.ResponseBody.Layout.OutputSpeech

        assert text.replace(':', ' ') == vc
        return "\n".join(["Text:  " + text, "Voice: " + vc])

    @pytest.mark.experiments('weather_now_forecast_warning')
    def test_experiment_tts_now(self, alice):
        r = alice(voice('погода сейчас ' + self.city))
        text = r.run_response.ResponseBody.Layout.Cards[0].Text
        vc = r.run_response.ResponseBody.Layout.OutputSpeech

        assert text.replace(':', ' ') == vc
        return "\n".join(["Text:  " + text, "Voice: " + vc])

    @pytest.mark.experiments('weather_for_range_forecast_warning')
    def test_experiment_tts_weekend(self, alice):
        r = alice(voice('погода на выходных ' + self.city))
        text = r.run_response.ResponseBody.Layout.Cards[0].Text
        vc = r.run_response.ResponseBody.Layout.OutputSpeech

        assert text.replace(':', '') == vc
        return "\n".join(["Text:  " + text, "Voice: " + vc])

    @pytest.mark.experiments('weather_for_range_forecast_warning')
    def test_experiment_tts_this_week(self, alice):
        r = alice(voice('погода на эту неделю ' + self.city))
        text = r.run_response.ResponseBody.Layout.Cards[0].Text
        vc = r.run_response.ResponseBody.Layout.OutputSpeech

        assert text.replace(':', '') == vc
        return "\n".join(["Text:  " + text, "Voice: " + vc])

    @pytest.mark.experiments('weather_for_range_forecast_warning')
    def test_experiment_tts_next_week(self, alice):
        r = alice(voice('погода на следующую неделю ' + self.city))
        text = r.run_response.ResponseBody.Layout.Cards[0].Text
        vc = r.run_response.ResponseBody.Layout.OutputSpeech

        assert text.replace(':', '') == vc
        return "\n".join(["Text:  " + text, "Voice: " + vc])
