import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.smart_tv])
@pytest.mark.experiments('video_pure_hw_base_info', 'video_pure_hw_search')
class TestTvSearch(object):

    owners = ('dandex',)

    @pytest.mark.parametrize('command', ['найди', 'поищи', 'включи'])
    @pytest.mark.parametrize('genre', ['мультики', 'фильмы', 'сериалы'])
    def test_genre_search(self, alice, command, genre):
        response = alice(f'{command} {genre}')
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.TvOpenSearchScreenDirective
        assert response.directive.payload.search_query == genre
