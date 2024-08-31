import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import alice.tests.library.intent as intent
import pytest


METRONOME_TRACK_IDS = {
    40: '30657961',
    55: '30657976',
    60: '30657981',
    65: '30657986',
    75: '30657996',
    78: '30657999',
    82: '30688689',
    85: '30688692',
    100: '30688707',
    115: '30688722',
    120: '30688727',
    138: '30688745',
    144: '30688751',
}


def _check_metronome_response(response, expected_answer, expected_intent):
    assert response.scenario == scenario.Metronome
    assert response.text == expected_answer
    assert len(response.directives) == 1
    assert response.directives[0].name == directives.names.MmStackEngineGetNextCallback
    assert response.intent == expected_intent


def _check_music_response(response, expected_bpm):
    assert response.scenario == scenario.HollywoodMusic
    assert len(response.directives) == 2
    assert response.directives[0].name == directives.names.AudioPlayDirective
    assert response.directives[0].payload.stream.id == METRONOME_TRACK_IDS[expected_bpm]
    assert response.directives[0].payload.metadata.glagol_metadata.music_metadata.repeat_mode == 'One'
    assert response.directives[1].name == directives.names.MmStackEngineGetNextCallback


@pytest.mark.version(hollywood=176)
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(
    f'mm_enable_protocol_scenario={scenario.Metronome}',
    'bg_exp_enable_metronome'
)
class TestMetronome(object):
    owners = ('nkodosov', 'abc:megamind')

    @pytest.mark.parametrize('command, expected_bpm, expected_answer', [
        ('запусти метроном', 120, 'Включаю. 120 bpm.'),
        ('метроном на 65 ударов', 65, 'Включаю. 65 bpm.'),
        ('включи метроном на 1800', 144, 'Максимум 144 bpm. Включаю.'),
        ('алиса метроном на 10', 40, 'Минимум 40 bpm. Включаю.'),
        ('метроном на 40 bpm', 40, 'Включаю. 40 bpm.'),
        ('метроном на 144 bpm', 144, 'Включаю. 144 bpm.'),
    ])
    def test_start_metronome(self, alice, command, expected_bpm, expected_answer):
        response = alice(command)
        _check_metronome_response(response, expected_answer, intent.MetronomeStart)
        response.next()
        _check_music_response(response, expected_bpm)

    def test_start_metronome_saved_state(self, alice):
        response = alice('метроном на 65 bpm')
        response.next()
        alice('домой')
        response = alice('запусти метроном')
        _check_metronome_response(response, 'Включаю. 65 bpm.', intent.MetronomeStart)
        response.next()
        _check_music_response(response, 65)

    @pytest.mark.parametrize('command, start_bpm, expected_bpm, expected_answer, expected_intent', [
        ('быстрее', 80, 85, '85 bpm.', intent.MetronomeFaster),
        ('медленнее', 80, 75, '75 bpm.', intent.MetronomeSlower),
        ('быстрее', 140, 144, 'Максимум 144 bpm. Включаю.', intent.MetronomeFaster),
        ('медленнее', 41, 40, 'Минимум 40 bpm. Включаю.', intent.MetronomeSlower),
        ('немного быстрее', 80, 82, '82 bpm.', intent.MetronomeFaster),
        ('немного медленнее', 80, 78, '78 bpm.', intent.MetronomeSlower),
        ('немного быстрее', 143, 144, 'Максимум 144 bpm. Включаю.', intent.MetronomeFaster),
        ('немного медленнее', 41, 40, 'Минимум 40 bpm. Включаю.', intent.MetronomeSlower),
        ('сильно быстрее', 80, 100, '100 bpm.', intent.MetronomeFaster),
        ('сильно медленнее', 80, 60, '60 bpm.', intent.MetronomeSlower),
        ('сильно быстрее', 130, 144, 'Максимум 144 bpm. Включаю.', intent.MetronomeFaster),
        ('сильно медленнее', 50, 40, 'Минимум 40 bpm. Включаю.', intent.MetronomeSlower),
        ('быстрее на 35', 80, 115, '115 bpm.', intent.MetronomeFaster),
        ('медленнее на 15', 80, 65, '65 bpm.', intent.MetronomeSlower),
        ('быстрее на 100', 90, 144, 'Максимум 144 bpm. Включаю.', intent.MetronomeFaster),
        ('медленнее на 50', 70, 40, 'Минимум 40 bpm. Включаю.', intent.MetronomeSlower),
        ('быстрее до 138', 80, 138, '138 bpm.', intent.MetronomeFaster),
        ('медленнее до 55', 80, 55, '55 bpm.', intent.MetronomeSlower),
        ('быстрее до 1000', 90, 144, 'Максимум 144 bpm. Включаю.', intent.MetronomeFaster),
        ('медленнее до 20', 70, 40, 'Минимум 40 bpm. Включаю.', intent.MetronomeSlower),
    ])
    def test_update_metronome(self, alice, command, start_bpm, expected_bpm, expected_answer, expected_intent):
        response = alice(f'метроном на {start_bpm} bpm')
        response.next()
        response = alice(command)
        _check_metronome_response(response, expected_answer, expected_intent)
        response.next()
        _check_music_response(response, expected_bpm)

    @pytest.mark.version(hollywood=203)
    def test_update_no_metronome(self, alice):
        response = alice('быстрее')
        assert response.scenario != scenario.Metronome
