import logging
import json
from urllib.parse import parse_qs, unquote

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.scenarios.music.it2_music_client.music_sdk_helpers import (
    get_response,
)


logger = logging.getLogger(__name__)


@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.legatus])
@pytest.mark.oauth(auth.YandexPlus)
class TestsLegatus:

    @pytest.mark.parametrize('command, text, params', [
        pytest.param('включи песню краш',
                     'Включаю: Клава Кока, NILETTO, альбом \"Краш\", песня \"Краш\".',
                     '{\"id\":\"66869588\",\"pageId\":\"musicPlayer\",\"type\":\"track\"}',
                     id='track'),
        pytest.param('включи альбом the dark side of the moon',
                     'Включаю: Pink Floyd, альбом \"The Dark Side Of The Moon\".',
                     '{\"id\":\"297567\",\"pageId\":\"musicPlayer\",\"type\":\"album\"}',
                     id='album'),
        pytest.param('включи альбом the dark side of the moon вперемешку',
                     'Включаю: Pink Floyd, альбом \"The Dark Side Of The Moon\".',
                     '{\"id\":\"297567\",\"pageId\":\"musicPlayer\",\"shuffle\":true,\"type\":\"album\"}',
                     id='album_shuffle'),
        pytest.param('включи альбом the dark side of the moon на повторе',
                     'Включаю: Pink Floyd, альбом \"The Dark Side Of The Moon\".',
                     '{\"id\":\"297567\",\"pageId\":\"musicPlayer\",\"repeat\":true,\"type\":\"album\"}',
                     id='album_repeat'),
        pytest.param('включи queen',
                     'Включаю: Queen.',
                     '{\"id\":\"79215\",\"pageId\":\"musicPlayer\",\"type\":\"artist\"}',
                     id='artist'),
        pytest.param('включи плейлист дня',
                     'Включаю подборку \"Плейлист дня\".',
                     '{\"kind\":\"127167070\",\"owner\":\"503646255\",\"pageId\":\"musicPlayer\",\"type\":\"playlist\"}',
                     id='special_playlist'),
        pytest.param('включи плейлист вечные хиты',
                     'Включаю подборку \"Вечные хиты\".',
                     '{\"kind\":\"1250\",\"owner\":\"105590476\",\"pageId\":\"musicPlayer\",\"type\":\"playlist\"}',
                     id='usual_playlist'),
        pytest.param('включи музыку',
                     'Включаю.',
                     '{\"pageId\":\"musicPlayer\",\"tag\":\"user:onyourwave\",\"type\":\"radio\"}',
                     id='radio'),
    ])
    def test_web_os_params(self, alice, command, text, params):
        r = alice(voice(command))
        response = get_response(r)
        layout = response.Layout
        assert layout.Cards[0].Text == text

        directive = layout.Directives[0].WebOSLaunchAppDirective
        assert directive.AppId == 'tv.kinopoisk.ru'
        assert directive.ParamsJson.decode('utf-8') == params

        return str(r)  # for debug purposes


@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.legatus])
@pytest.mark.oauth(auth.Yandex)
class TestsLegatusWithoutPlus:

    @pytest.mark.parametrize('command', [
        pytest.param('включи песню краш',
                     id='track'),
        pytest.param('включи альбом the dark side of the moon',
                     id='album'),
        pytest.param('включи queen',
                     id='artist'),
        pytest.param('включи плейлист дня',
                     id='special_playlist'),
        pytest.param('включи плейлист вечные хиты',
                     id='usual_playlist'),
        pytest.param('включи музыку',
                     id='radio'),
    ])
    def test_web_os_params(self, alice, command):
        r = alice(voice(command))
        response = get_response(r)
        layout = response.Layout
        assert layout.Cards[0].Text in [
            'Чтобы слушать музыку, вам нужно оформить подписку Яндекс.Плюс.',
            'Извините, но я пока не могу включить то, что вы просите. Для этого необходимо оформить подписку на Плюс.',
            'Простите, я бы с радостью, но у вас нет подписки на Плюс.',
        ]
        params = '{\"pageId\":\"music\"}'
        directive = layout.Directives[0].WebOSLaunchAppDirective
        assert directive.AppId == 'tv.kinopoisk.ru'
        assert directive.ParamsJson.decode('utf-8') == params

        return str(r)  # for debug purposes


@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.legatus])
@pytest.mark.no_oauth
class TestsLegatusUnauthorized:

    @pytest.mark.parametrize('command', [
        pytest.param('включи песню краш',
                     id='track'),
        pytest.param('включи альбом the dark side of the moon',
                     id='album'),
        pytest.param('включи queen',
                     id='artist'),
        pytest.param('включи плейлист дня',
                     id='special_playlist'),
        pytest.param('включи плейлист вечные хиты',
                     id='usual_playlist'),
        pytest.param('включи музыку',
                     id='radio'),
    ])
    def test_web_os_params(self, alice, command):
        r = alice(voice(command))
        response = get_response(r)
        layout = response.Layout
        assert layout.Cards[0].Text in [
            'Если хотите послушать музыку, авторизуйтесь, пожалуйста.',
            'Авторизуйтесь, пожалуйста, и сразу включу вам все, что попросите!',
        ]
        params = '{\"pageId\":\"music\"}'
        directive = layout.Directives[0].WebOSLaunchAppDirective
        assert directive.AppId == 'tv.kinopoisk.ru'
        assert directive.ParamsJson.decode('utf-8') == params

        return str(r)  # for debug purposes


@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.legatus])
@pytest.mark.oauth(auth.YandexPlus)
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
class TestsLegatusYoutubeInForeground:

    @pytest.mark.parametrize('command', [
        pytest.param('включи песню краш',
                     id='track'),
        pytest.param('включи альбом the dark side of the moon',
                     id='album'),
        pytest.param('включи queen',
                     id='artist'),
        pytest.param('включи плейлист дня',
                     id='special_playlist'),
        pytest.param('включи плейлист вечные хиты',
                     id='usual_playlist'),
        pytest.param('включи музыку',
                     id='radio'),
    ])
    def test_web_os_params(self, alice, command):
        r = alice(voice(command))
        response = get_response(r)
        layout = response.Layout
        assert layout.Cards[0].Text == 'Секунду.'
        directive = layout.Directives[0].WebOSLaunchAppDirective
        assert directive.AppId == 'youtube.leanback.v4'
        result_params = json.loads(directive.ParamsJson.decode('utf-8'))
        params = parse_qs(result_params['contentTarget'])
        assert command == unquote(params['vq'][0])


@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.legatus])
@pytest.mark.no_oauth
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
class TestsLegatusYoutubeInForegroundUnauthorized:

    @pytest.mark.parametrize('command', [
        pytest.param('включи песню краш',
                     id='track'),
        pytest.param('включи альбом the dark side of the moon',
                     id='album'),
        pytest.param('включи queen',
                     id='artist'),
        pytest.param('включи плейлист дня',
                     id='special_playlist'),
        pytest.param('включи плейлист вечные хиты',
                     id='usual_playlist'),
        pytest.param('включи музыку',
                     id='radio'),
    ])
    def test_web_os_params(self, alice, command):
        r = alice(voice(command))
        response = get_response(r)
        layout = response.Layout
        assert layout.Cards[0].Text == 'Секунду.'
        directive = layout.Directives[0].WebOSLaunchAppDirective
        assert directive.AppId == 'youtube.leanback.v4'
        result_params = json.loads(directive.ParamsJson.decode('utf-8'))
        params = parse_qs(result_params['contentTarget'])
        assert command == unquote(params['vq'][0])
