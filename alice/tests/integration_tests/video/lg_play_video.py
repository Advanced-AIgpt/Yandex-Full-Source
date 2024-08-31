import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import json
import pytest
import re


from urllib.parse import parse_qs, unquote


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.legatus])
class TestPlayVideo(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('command, params', [
        pytest.param('включи бегущий по лезвию 2049',
                     '{\"pageId\":\"player\",\"pageParam\":\"4a9ade1c17c4f8c6bf5c1774c43d053e\"}',
                     id='movie'),
        pytest.param('включи рик и морти',
                     '{\"contentId\":\"4b7e75f953b028e5b856bbaf23d5459f\",\"pageId\":\"player\",\"pageParam\":\"46c5df252dc1a790b82d1a00fcf44812\"}',
                     id='serie'),
        pytest.param('включи рик и морти 2 сезон 4 серия',
                     '{\"contentId\":\"4ca8b4f4f017eb7a9b21576be8fd9677\",\"pageId\":\"player\",\"pageParam\":\"46c5df252dc1a790b82d1a00fcf44812\"}',
                     id='serie_with_specified_episode'),
    ])
    def test_play_video(self, alice, command, params):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.text in [
            'Включаю.',
            'Запускаю.',
            'Сейчас включу.',
            'Секунду.',
            'Секундочку.',
        ]
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == 'tv.kinopoisk.ru'
        assert response.directive.payload.params_json == params

    @pytest.mark.parametrize('command, params', [
        pytest.param('включи фильм Унесённые призраками',
                     '{\"pageId\":\"film\",\"pageParam\":\"4726854ee2be6d928a2d852281fa18f9\"}',
                     id='movie'),
        pytest.param('включи Во все тяжкие',
                     '{\"pageId\":\"film\",\"pageParam\":\"49c681d107948f2ebdf7dd6b46f6ebca\"}',
                     id='serie'),
    ])
    def test_play_video_with_payment(self, alice, command, params):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.text == 'Открываю экран оплаты.'
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == 'tv.kinopoisk.ru'
        assert response.directive.payload.params_json == params


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.legatus])
class TestPlayVideoWithoutPlus(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('command, params', [
        pytest.param('включи бегущий по лезвию 2049',
                     '{\"pageId\":\"film\",\"pageParam\":\"4a9ade1c17c4f8c6bf5c1774c43d053e\"}',
                     id='movie'),
        pytest.param('включи водоворот',
                     '{\"pageId\":\"film\",\"pageParam\":\"423a833b5eb8febcbfcc56c64a46ecda\"}',
                     id='serie'),
    ])
    def test_play_video(self, alice, command, params):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.text == 'Открываю экран оплаты.'
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == 'tv.kinopoisk.ru'
        assert response.directive.payload.params_json == params


@pytest.mark.version(videobass=88)
@pytest.mark.no_oauth
@pytest.mark.parametrize('surface', [surface.legatus])
class TestPlayVideoUnauthorized(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('command, params', [
        pytest.param('включи бегущий по лезвию 2049',
                     '{\"pageId\":\"film\",\"pageParam\":\"4a9ade1c17c4f8c6bf5c1774c43d053e\",\"pageQuery\":{\"auth-deferred\":true}}',
                     id='movie'),
        pytest.param('включи водоворот',
                     '{\"pageId\":\"film\",\"pageParam\":\"423a833b5eb8febcbfcc56c64a46ecda\",\"pageQuery\":{\"auth-deferred\":true}}',
                     id='serie'),
    ])
    def test_play_video(self, alice, command, params):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.text == 'Открываю экран оплаты.'
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == 'tv.kinopoisk.ru'
        assert response.directive.payload.params_json == params


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.legatus])
class TestShowDescription(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('command, params', [
        pytest.param('покажи описание бегущий по лезвию 2049',
                     '{\"pageId\":\"film\",\"pageParam\":\"4a9ade1c17c4f8c6bf5c1774c43d053e\"}',
                     id='movie'),
        pytest.param('покажи описание сериала рик и морти',
                     '{\"pageId\":\"film\",\"pageParam\":\"46c5df252dc1a790b82d1a00fcf44812\"}',
                     id='serie'),
    ])
    def test_show_description(self, alice, command, params):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.text in [
            'Вот описание.',
            'Открываю описание.',
            'Одну секунду.',
        ]
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == 'tv.kinopoisk.ru'
        assert response.directive.payload.params_json == params


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.legatus])
class TestShowDescriptionWithoutPlus(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('command, params', [
        pytest.param('покажи описание бегущий по лезвию 2049',
                     '{\"pageId\":\"film\",\"pageParam\":\"4a9ade1c17c4f8c6bf5c1774c43d053e\"}',
                     id='movie'),
    ])
    def test_show_description(self, alice, command, params):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.text in [
            'Вот описание.',
            'Открываю описание.',
            'Одну секунду.',
        ]
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == 'tv.kinopoisk.ru'
        assert response.directive.payload.params_json == params


@pytest.mark.version(videobass=88)
@pytest.mark.no_oauth
@pytest.mark.parametrize('surface', [surface.legatus])
class TestShowDescriptionUnauthorized(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('command, params', [
        pytest.param('покажи описание бегущий по лезвию 2049',
                     '{\"pageId\":\"film\",\"pageParam\":\"4a9ade1c17c4f8c6bf5c1774c43d053e\",\"pageQuery\":{\"auth-deferred\":true}}',
                     id='movie'),
    ])
    def test_show_description(self, alice, command, params):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.text in [
            'Вот описание.',
            'Открываю описание.',
            'Одну секунду.',
        ]
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == 'tv.kinopoisk.ru'
        assert response.directive.payload.params_json == params


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.legatus])
class TestShowGallery(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('command, text_re, params_re', [
        pytest.param('включи гарри поттер',
                     r'Видео-контент по запросу «гарри поттер»',
                     r'.*{\"appId\":\"tv.kinopoisk.ru\",\"params\":{\"pageId\":\"player\",\"pageParam\":\"4df0fe0d1c7bc66e88bb6848a1e926fd\"}}.*',
                     id='harry_potter'),
        pytest.param('покажи комедии',
                     r'Комедии',
                     r'.*{\"appId\":\"tv.kinopoisk.ru\",\"params\":{\"pageId\":\"player\",\"pageParam\":.*',
                     id='comedy'),
        pytest.param('покажи фильмы с Джонни Деппом',
                     r'Фильмы по запросу «с джонни деппом»',
                     r'.*{\"appId\":\"tv.kinopoisk.ru\",\"params\":{\"pageId\":\"player\",\"pageParam\":.*',
                     id='actor'),
        pytest.param('титаник',
                     r'Видео-контент по запросу «титаник»',
                     r'.*{\"appId\":\"tv.kinopoisk.ru\",\"params\":{\"contentId\":\"416e6e20ebd50a458a270d36447239d4\".*',
                     id='titanik'),
        pytest.param('кинпописк',
                     r'(Одну секунду|Сейчас поищем|Вот что удалось найти|Секундочку, сейчас найдем).',
                     r'.*{\"appId\":\"tv.kinopoisk.ru\",\"params\":{\"pageId\":\"player\",\"pageParam\":\"4.*',
                     id='kinopoisk'),
        pytest.param('Порекомендуй сериалы на вечер',
                     r'Сериалы по запросу «на вечер»',
                     r'.*{\"appId\":\"tv.kinopoisk.ru\",\"params\":{\"pageId\":\"player\",\"pageParam\":\"4.*',
                     id='recommend_series'),
    ])
    def test_show_gallery(self, alice, command, text_re, params_re):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert re.match(text_re, response.text)
        assert response.directive.name == directives.names.WebOSShowGalleryDirective
        assert re.match(params_re, response.directive.payload.items_json[0])

    @pytest.mark.parametrize('command', [
        pytest.param('найди видео с котиками', id='cats'),
        pytest.param('включи видео', id='general'),
    ])
    def test_show_gallery_nothing_found(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.text in [
            'Ничего не нашлось.',
            'Я ничего не нашла.',
            'Я ничего не смогла найти.',
        ]
        assert not response.directive

    @pytest.mark.parametrize('command, params', [
        pytest.param('рик и морти 3 сезон',
                     '{\"pageId\":\"series\",\"pageParam\":\"46c5df252dc1a790b82d1a00fcf44812\"}',
                     id='rick_and_morty'),
    ])
    def test_show_season_gallery(self, alice, command, params):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.text in [
            'Открываю.',
            'Секунду.',
            'Секундочку.',
            'Сейчас открою.',
        ]
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == 'tv.kinopoisk.ru'
        assert response.directive.payload.params_json == params


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.legatus])
class TestShowGalleryWithoutPlus(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('command, text, params', [
        pytest.param('включи гарри поттер',
                     'Видео-контент по запросу «гарри поттер»',
                     '{\"appId\":\"tv.kinopoisk.ru\",\"params\":{\"pageId\":\"player\",\"pageParam\":\"4df0fe0d1c7bc66e88bb6848a1e926fd\"}}',
                     id='harry_potter'),
    ])
    def test_show_gallery(self, alice, command, text, params):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.text == text
        assert response.directive.name == directives.names.WebOSShowGalleryDirective
        assert params in response.directive.payload.items_json[0]

    @pytest.mark.parametrize('command, params', [
        pytest.param('водоворот 1 сезон',
                     '{\"pageId\":\"series\",\"pageParam\":\"423a833b5eb8febcbfcc56c64a46ecda\"}',
                     id='vodovorot'),
    ])
    def test_show_season_gallery(self, alice, command, params):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.text in [
            'Открываю.',
            'Секунду.',
            'Секундочку.',
            'Сейчас открою.',
        ]
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == 'tv.kinopoisk.ru'
        assert response.directive.payload.params_json == params


@pytest.mark.version(videobass=88)
@pytest.mark.no_oauth
@pytest.mark.parametrize('surface', [surface.legatus])
class TestShowGalleryUnauthorized(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('command, text, params', [
        pytest.param('включи гарри поттер',
                     'Видео-контент по запросу «гарри поттер»',
                     '{\"appId\":\"tv.kinopoisk.ru\",\"params\":{\"pageId\":\"player\",\"pageParam\":\"4df0fe0d1c7bc66e88bb6848a1e926fd\",\"pageQuery\":{\"auth-deferred\":true}}}',
                     id='harry_potter'),
    ])
    def test_show_gallery(self, alice, command, text, params):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.text == text
        assert response.directive.name == directives.names.WebOSShowGalleryDirective
        assert params in response.directive.payload.items_json[0]

    @pytest.mark.parametrize('command, params', [
        pytest.param('водоворот 1 сезон',
                     '{\"pageId\":\"series\",\"pageParam\":\"423a833b5eb8febcbfcc56c64a46ecda\"}',
                     id='vodovorot'),
    ])
    def test_show_season_gallery(self, alice, command, params):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.text in [
            'Открываю.',
            'Секунду.',
            'Секундочку.',
            'Сейчас открою.',
        ]
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == 'tv.kinopoisk.ru'
        assert response.directive.payload.params_json == params


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.legatus])
@pytest.mark.environment_state({
    'endpoints': [
        {
            'capabilities': [
                {
                    'parameters': {
                        'available_apps': [
                            {
                                'app_id': 'com.685631.3411'
                            },
                            {
                                'app_id': 'youtube.leanback.v4'
                            }
                        ]
                    },
                    'meta': {
                        'supported_directives': [
                            'WebOSLaunchAppDirectiveType',
                            'WebOSShowGalleryDirectiveType'
                        ]
                    },
                    'state': {
                        'foreground_app_id': 'youtube.leanback.v4'
                    },
                    '@type': 'type.googleapis.com/NAlice.TWebOSCapability',
                }
            ],
            'meta': {
                'type': 'WebOsTvEndpointType'
            }
        }
    ]
})
class TestYoutubeInForeground(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('command', [
        # play video
        pytest.param('включи бегущий по лезвию 2049',
                     id='play_movie'),
        pytest.param('включи рик и морти',
                     id='play_serie'),
        pytest.param('включи рик и морти 2 сезон 4 серия',
                     id='play_serie_with_specified_episode'),
        # payment
        pytest.param('включи фильм начало',
                     id='payment_movie'),
        pytest.param('включи черное зеркало',
                     id='payment_serie'),
        # show description
        pytest.param('покажи описание бегущий по лезвию 2049',
                     id='show_description_movie'),
        pytest.param('покажи описание сериала рик и морти',
                     id='show_description_serie'),
        # show gallery
        pytest.param('включи гарри поттер',
                     id='show_gallery_harry_potter'),
        # show season gallery
        pytest.param('рик и морти 3 сезон',
                     id='show_season_gallery_rick_and_morty'),
    ])
    def test_video_queries(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.text == 'Секунду.'
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == 'youtube.leanback.v4'
        result_params = json.loads(response.directive.payload.params_json)
        params = parse_qs(result_params['contentTarget'])
        assert command == unquote(params['vq'][0])
