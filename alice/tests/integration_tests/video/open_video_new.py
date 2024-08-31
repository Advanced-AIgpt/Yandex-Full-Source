import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
class TestOpenNewVideo(object):

    owners = ('kolchanovs', 'abc:smarttv')

    @pytest.mark.parametrize('surface', [surface.smart_tv])
    def test_open_new_video(self, alice):
        response = alice('открой новинки')
        assert response.scenario == scenario.Video
        assert response.text
        assert response.has_voice_response()
        assert response.directive
        assert response.directive.name == directives.names.TvOpenSearchScreenDirective
