import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(
    'mm_disable_protocol_scenario=Commands',  # We want Vins's 'stop' to handle requests
    'mm_disable_preclassifier_hint=personal_assistant.scenarios.fast_command.fast_pause',
    'vins_pause_commands_relevant_again',
)
class TestStop(object):
    owners = ('vitvlkv', 'nkodosov')

    @pytest.mark.parametrize('surface', [surface.station])
    def test_vins_stops_audio_player(self, alice):
        response = alice('включи queen')
        assert response.scenario == scenario.HollywoodMusic
        assert len(response.directives) == 2
        assert response.directives[0].name == directives.names.AudioPlayDirective
        assert response.directives[1].name == directives.names.MmStackEngineGetNextCallback

        response = alice('стоп')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.PlayerPause

        assert not response.text
        assert len(response.directives) == 2
        assert response.directives[0].name == directives.names.PlayerPauseDirective
        assert response.directives[1].name == directives.names.ClearQueueDirective

    @pytest.mark.parametrize('surface', [surface.station])
    def test_vins_stops_music_player(self, alice):
        response = alice('включи шум моря')
        assert response.scenario == scenario.Vins
        assert len(response.directives) == 1
        assert response.directives[0].name == directives.names.MusicPlayDirective

        response = alice('стоп')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.PlayerPause

        assert not response.text
        assert len(response.directives) == 2
        assert response.directives[0].name == directives.names.PlayerPauseDirective
        assert response.directives[1].name == directives.names.ClearQueueDirective

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_vins_stops_music_player_on_surface_without_directive_sequencer(self, alice):
        '''
        SearchApp (ПП) не имеет сапортед фичи directive_sequencer. Поэтому для нее директива
        clear_queue не возвращается.
        '''
        response = alice('включи шум моря')
        assert response.scenario == scenario.Vins
        assert len(response.directives) == 1
        assert response.directives[0].name == directives.names.OpenUriDirective

        response = alice('стоп')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.PlayerPause

        assert response.text == 'Ставлю на паузу'
        assert len(response.directives) == 1
        assert response.directives[0].name == directives.names.PlayerPauseDirective
