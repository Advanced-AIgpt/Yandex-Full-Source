import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['metronome']


METRONOME_TRACK_IDS = {
    40: '30657961',
    65: '30657986',
    100: '30688707',
    120: '30688727',
    144: '30688751',
}


@pytest.mark.scenario(name='Metronome', handle='metronome')
@pytest.mark.experiments(*[
    'bg_fresh_granet_prefix=alice.metronome',
    'bg_exp_enable_metronome',
])
@pytest.mark.parametrize('surface', [surface.station])
class Tests:

    @pytest.mark.parametrize('command, expected_answer, expected_bpm', [
        pytest.param('запусти метроном', 'Включаю. 120 bpm.', 120, id='start_default'),
        pytest.param('метроном на 65 ударов', 'Включаю. 65 bpm.', 65, id='start_65'),
        pytest.param('включи метроном на 1800', 'Максимум 144 bpm. Включаю.', 144, id='start_1800'),
        pytest.param('алиса метроном на 10', 'Минимум 40 bpm. Включаю.', 40, id='start_10'),
        pytest.param('метроном на 40 bpm', 'Включаю. 40 bpm.', 40, id='start_40'),
        pytest.param('метроном на 144 bpm', 'Включаю. 144 bpm.', 144, id='start_144'),
    ])
    def test_start_metronome(self, alice, command, expected_answer, expected_bpm):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech == expected_answer
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'metronome'
        music_play_frame = r.run_response.ResponseBody.StackEngine.Actions[1].ResetAdd.Effects[0].ParsedUtterance.TypedSemanticFrame.MusicPlaySemanticFrame
        assert music_play_frame.ObjectId.StringValue == METRONOME_TRACK_IDS[expected_bpm]
        assert music_play_frame.DisableNlg.BoolValue is True
        assert music_play_frame.Repeat.RepeatValue == 'One'

    def test_start_metronome_saved_state(self, alice):
        alice(voice('метроном на 65 bpm'))
        r = alice(voice('запусти метроном'))
        assert r.run_response.ResponseBody.Layout.OutputSpeech == 'Включаю. 65 bpm.'

    @pytest.mark.device_state(audio_player={
        'current_stream': {
            'stream_id': METRONOME_TRACK_IDS[100]
        },
        'player_state': 'Playing'
    })
    @pytest.mark.parametrize('command, expected_bpm', [
        pytest.param('быстрее', 105, id='update_faster'),
        pytest.param('сильно быстрее', 120, id='update_sig_faster'),
        pytest.param('немного быстрее', 102, id='update_sli_faster'),
        pytest.param('медленнее', 95, id='update_slower'),
        pytest.param('сильно медленнее', 80, id='update_sig_slower'),
        pytest.param('немного медленнее', 98, id='update_sli_slower'),
        pytest.param('быстрее до 140', 140, id='update_faster_to_140'),
        pytest.param('медленнее до 60', 60, id='update_slower_to_60'),
    ])
    def test_update_metronome(self, alice, command, expected_bpm):
        r = alice(voice('метроном на 100'))
        r = alice(voice(command))
        assert r.run_response.ResponseBody.Layout.OutputSpeech == f'{expected_bpm} bpm.'

    def test_update_no_metronome(self, alice):
        r = alice(voice('быстрее'))
        assert r.run_response.ResponseBody.Layout.OutputSpeech == 'Извините, я не поняла, какой метроном нужно включить.'
        assert r.run_response.Features.IsIrrelevant is True
