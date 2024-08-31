import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
class TestStopMusic(object):

    owners = ('nkodosov',)

    @pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
    def test_thin_client(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('хватит')
        assert response.directives[-1].name == directives.names.ClearQueueDirective
        assert not response.has_voice_response()
        assert (response.text == 'ОК' or not response.text)

    @pytest.mark.parametrize('surface', [surface.dexp])
    def test_dexp(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.MusicPlayDirective

        response = alice('хватит')
        assert response.directive.name == directives.names.PlayerPauseDirective
        assert not response.has_voice_response()
        assert (response.text == 'ОК' or not response.text)

    @pytest.mark.parametrize('surface', [surface.navi, surface.searchapp])
    def test_searchapp(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.OpenUriDirective

        response = alice('стоп')
        assert response.directive.name == directives.names.PlayerPauseDirective
        assert not response.has_voice_response()
        assert response.text == 'Ставлю на паузу'

    @pytest.mark.parametrize('surface', [surface.automotive])
    def test_auto(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.OpenUriDirective

        response = alice('хватит')
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexauto://media_control?action=pause'
        assert not response.has_voice_response()

    @pytest.mark.parametrize('surface', [surface.old_automotive])
    def test_old_auto(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.CarDirective

        response = alice('хватит')
        assert response.directive.name == directives.names.CarDirective
        assert response.directive.payload.intent == 'media_control'
        assert response.directive.payload.params.action == 'pause'
        assert not response.has_voice_response()

    @pytest.mark.parametrize('surface', [surface.smart_tv])
    @pytest.mark.supported_features('music_quasar_client')
    def test_any_surface_with_music_quasar_client(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('хватит')
        assert response.directive.name == directives.names.ClearQueueDirective
        assert not response.has_voice_response()
        assert not response.text
