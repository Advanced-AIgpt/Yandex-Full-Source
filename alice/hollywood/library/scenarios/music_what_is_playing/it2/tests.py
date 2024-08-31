import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['music_what_is_playing']


@pytest.mark.scenario(name='MusicWhatIsPlaying', handle='music_what_is_playing')
@pytest.mark.experiments('mm_force_scenario=MusicWhatIsPlaying')
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class Tests:

    def test_smoke(self, alice):
        r = alice(voice('Что сейчас играет?'))
        assert r.scenario_stages() == {'run'}
        analytics_info = r.run_response.ResponseBody.AnalyticsInfo
        assert analytics_info.ProductScenarioName == 'music_what_is_playing'
        assert analytics_info.Intent == 'personal_assistant.scenarios.music_what_is_playing'
        layout = r.run_response.ResponseBody.Layout
        assert any(it for it in layout.Directives if it.HasField('StartMusicRecognizerDirective'))
        assert layout.OutputSpeech
        return layout.OutputSpeech
