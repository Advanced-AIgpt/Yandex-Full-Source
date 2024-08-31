import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['get_time']


@pytest.mark.app(lang='ar-SA')
@pytest.mark.experiments('mm_allow_lang_ar')
@pytest.mark.scenario(name='GetTime', handle='get_time')
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class Tests:

    def test_smoke(self, alice):
        r = alice(voice('كم الساعة؟'))  # Сколько сейчас времени
        assert r.scenario_stages() == {'run'}
        analytics_info = r.run_response.ResponseBody.AnalyticsInfo
        assert analytics_info.ProductScenarioName == 'get_time'
        assert analytics_info.Intent == 'personal_assistant.scenarios.get_time'
        layout = r.run_response.ResponseBody.Layout
        assert len(layout.Directives) == 0
        assert layout.OutputSpeech
        assert layout.SuggestButtons
        return '\n'.join([layout.OutputSpeech] + ['\n'] +
                          [it.Text for it in layout.Cards] + ['\n'] +
                          [it.ActionButton.Title for it in layout.SuggestButtons])
