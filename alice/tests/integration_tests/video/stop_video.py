import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
class TestStopVideo(object):

    owners = ('nkodosov',)

    @pytest.mark.parametrize('surface', [surface.station])
    def test_video_stop_station(self, alice):
        alice('найди видео с котиками')
        alice('включи номер 1')
        response = alice('хватит')
        assert response.scenario == scenario.Commands
        assert response.directive.name == directives.names.ClearQueueDirective
        assert not response.has_voice_response()

    @pytest.mark.parametrize('surface', [surface.smart_tv])
    @pytest.mark.device_state(video={'player_capabilities': ['pause']})
    def test_video_stop_tv(self, alice):
        response = alice('включи первый канал')
        assert response.scenario == scenario.TvChannels
        assert response.directive.name == directives.names.OpenUriDirective
        response = alice('хватит')
        assert response.scenario == scenario.Commands
        assert response.directive.name == directives.names.PlayerPauseDirective
        assert not response.has_voice_response()

    @pytest.mark.parametrize('surface', [surface.smart_tv])
    def test_video_stop_tv_unsupported(self, alice):
        response = alice('включи первый канал')
        assert response.scenario == scenario.TvChannels
        assert response.directive.name == directives.names.OpenUriDirective
        response = alice('хватит')
        assert response.scenario == scenario.Commands
        assert response.text == 'Извините, такая команда не поддерживается этим плеером.'
