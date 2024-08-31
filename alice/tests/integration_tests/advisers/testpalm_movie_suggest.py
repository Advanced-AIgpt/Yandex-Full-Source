import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(f'mm_enable_protocol_scenario={scenario.MovieSuggest}')
class TestMovieSuggest(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-2564
    '''

    owners = ('dan-anastasev',)

    @pytest.mark.parametrize('surface', [
        surface.station,
    ])
    def test_movie_suggest_scenario(self, alice):
        response = alice('Посоветуй мультфильм')
        assert response.scenario == scenario.MovieSuggest
        assert response.intent == intent.ShowCartoons
        assert response.directive.name == directives.names.ShowVideoDescriptionDirective

        response = alice('Посоветуй фильм')
        assert response.scenario == scenario.MovieSuggest
        assert response.intent == intent.ShowMovies
        assert response.directive.name == directives.names.ShowVideoDescriptionDirective

        response = alice('Нет')
        assert response.scenario == scenario.MovieSuggest
        assert response.intent == intent.ShowMovies
        assert response.directive.name == directives.names.ShowVideoDescriptionDirective

        response = alice('Да')
        assert response.scenario == scenario.Video
        assert response.intent == intent.OpenCurrentVideo
        assert response.directive.name in [directives.names.ShowPayPushScreenDirective, directives.names.VideoPlayDirective]

        response = alice('Порекомендуй фильм')
        assert response.scenario == scenario.MovieSuggest
        assert response.intent == intent.ShowMovies
        assert response.directive.name == directives.names.ShowVideoDescriptionDirective

    @pytest.mark.parametrize('surface', [
        surface.station(is_tv_plugged_in=False),
    ])
    def test_movie_suggest_without_screen(self, alice):
        response = alice('Посоветуй фильм')
        assert response.scenario == scenario.Video
        assert response.intent == intent.VideoPlay
        assert not response.directive
        assert response.text in [
            'Чтобы смотреть видеоролики, фильмы и сериалы, нужно подключить Станцию к экрану.',
            'Чтобы смотреть видеоролики, фильмы и сериалы, подключите Станцию к экрану.',
        ]
