import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.version(megamind=204)
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('redirect_to_thin_player')
class TestsRedirect(object):
    owners = ('abc:alice_scenarios_music',)

    def test_ambient_sound(self, alice):
        response = alice('включи звуки природы')
        assert response.voice_response.should_listen is False
        response.next()
        assert response.text is None
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.directive.payload.metadata.glagol_metadata.music_metadata.type == 'Playlist'
        assert response.directive.payload.metadata.glagol_metadata.music_metadata.id == '103372440:1919'
        assert response.directive.payload.metadata.glagol_metadata.music_metadata.shuffled

        response = alice('включи шум костра')
        assert response.voice_response.should_listen is False
        response.next()
        assert response.text is None
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.directive.payload.metadata.glagol_metadata.music_metadata.type == 'Playlist'
        assert response.directive.payload.metadata.glagol_metadata.music_metadata.id == '103372440:1904'
        assert response.directive.payload.metadata.glagol_metadata.music_metadata.shuffled

    def test_promo(self, alice):
        response = alice('спой песню')
        assert response.voice_response.should_listen is False
        response.next()
        assert response.text is None
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.directive.payload.metadata.glagol_metadata.music_metadata.type == 'Album'
        assert response.directive.payload.metadata.glagol_metadata.music_metadata.id == '4924870'
        assert response.directive.payload.metadata.glagol_metadata.music_metadata.shuffled
