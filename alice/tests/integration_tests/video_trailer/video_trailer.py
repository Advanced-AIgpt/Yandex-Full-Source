import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


current_trailer_video_state = {
    'current_screen': 'mordovia_webview',
    'screen_state': {
        'scenario': 'MordoviaVideoSelection:VideoStationSPA:main',
        'scenario_name': 'MordoviaVideoSelection',
        'view_key': 'VideoStationSPA:main'
    },
    'view_state': {
        'currentScreen': 'videoEntity',
        'sections': [
            {
                'current_item': {
                    'description': 'История любви на фоне оксфордской академической жизни. Историк Диана Бишоп специализируется на старинных рукописях и однажды обнаруживает в архивах некий...',
                    'entref': '0oCgpydXc3NTgwMTMxGAKtuO_l',
                    'main_trailer_uuid': '4b46cc515699506498310a1bf977f007',
                    'metaforlog': {
                        'genres': 'фэнтези, драма',
                        'onto_category': 'film',
                        'onto_id': 'ruw7580131',
                        'onto_otype': 'Series@on',
                        'rating_kp': 7.4,
                        'release_year': 2018,
                        'restriction_age': 16
                    },
                    'provider_info': [
                        {
                            'available': 1,
                            'provider_item_id': '43db3b86e72348d483f88d64f665d997',
                            'provider_name': 'kinopoisk',
                            'type': 'tv_show'
                        }
                    ],
                    'provider_item_id': '43db3b86e72348d483f88d64f665d997',
                    'provider_name': 'kinopoisk',
                    'title': 'Открытие ведьм'
                },
                'items': []
            }
        ]
    }
}


empty_trailer_video_state = {
    'current_screen': 'mordovia_webview',
    'screen_state': {
        'scenario': 'MordoviaVideoSelection:VideoStationSPA:main',
        'scenario_name': 'MordoviaVideoSelection',
        'view_key': 'VideoStationSPA:main'
    },
    'view_state': {
        'currentScreen': 'videoEntity',
        'sections': [
            {
                'current_item': {
                    'description': 'Жена узнает о том, что муж постоянно ей изменяет. Их общий друг и знакомый Фальк решает подшутить над бедной семейной парой. Своей служанке, мечтающей ст...',
                    'entref': '0oCglydXcxNjUwNDkYAs6MlUc',
                    'metaforlog': {
                        'genres': 'комедия',
                        'onto_category': 'film',
                        'onto_id': 'ruw165049',
                        'onto_otype': 'Film',
                        'rating_kp': 8.1,
                        'release_year': 1978,
                        'restriction_age': 6
                    },
                    'provider_info': [
                        {
                            'available': 1,
                            'provider_item_id': '4a467b9602147ecea6c983cf39e61446',
                            'provider_name': 'kinopoisk',
                            'type': 'movie'
                        }
                    ],
                    'provider_item_id': '4a467b9602147ecea6c983cf39e61446',
                    'provider_name': 'kinopoisk',
                    'title': 'Летучая мышь'
                },
                'items': []
            }
        ]
    }
}


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('video_webview_video_entity')
@pytest.mark.parametrize('surface', [surface.station])
class TestOpenCurrentTrailer(object):

    owners = ('doggywolf')

    @pytest.mark.device_state(video=current_trailer_video_state)
    def test_open_current_trailer(self, alice):
        response = alice('трейлер')
        assert response.scenario == scenario.VideoTrailer
        assert response.directive.name == directives.names.VideoPlayDirective

    @pytest.mark.device_state(video=empty_trailer_video_state)
    def test_open_empty_trailer(self, alice):
        response = alice('покажи трейлер')
        assert response.scenario == scenario.VideoTrailer
        assert response.text in ['Извините, ничего не нашлось.', 'Кажется, трейлера здесь нет.', 'Не получилось найти трейлер.']
