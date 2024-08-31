import json
from urllib.parse import parse_qs, unquote

import pytest

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.legatus])
class TestPlayMusic(object):

    owners = ('amullanurov',)

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
    def test_play_music(self, alice, command, text, params):
        response = alice(command)
        assert response.intent == intent.MusicPlay
        assert response.text == text
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == 'tv.kinopoisk.ru'
        assert response.directive.payload.params_json == params

    @pytest.mark.version(megamind=231)
    def test_play_music_ambient_sound(self, alice):
        response = alice('Включи звуки природы')
        assert response.intent == intent.MusicAmbientSound
        assert response.text in [
            'Простите, но я не могу включить звуки природы здесь. Попросите меня об этом в умной колонке или в приложении Яндекса.',
            'Упс, звуки природы на телик пока не залила. Но могу с радостью включить их в приложении или через колонку, если попросите.',
        ]
        assert response.scenario == scenario.Vins
        assert not response.directive

    @pytest.mark.version(megamind=231)
    def test_play_music_fairy_tale(self, alice):
        response = alice('Включи сказку про колобка')
        assert response.intent == intent.MusicFairyTale
        assert response.text in [
            'Простите, но я не могу включить сказку здесь. Попросите меня об этом в умной колонке или в приложении Яндекса.',
            'Упс, сказки на телик пока не залила. Но могу с радостью рассказать их в приложении или через колонку, если попросите.',
        ]
        assert response.scenario == scenario.Vins
        assert not response.directive

    @pytest.mark.version(megamind=231)
    def test_play_vins_music(self, alice):
        response = alice('Включи радио')
        assert response.intent == intent.MusicPlay
        assert response.text in [
            'Извините, я пока не умею искать музыку.',
        ]
        assert response.scenario == scenario.Vins
        assert not response.directive


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.legatus])
@pytest.mark.skip(reason='https://st.yandex-team.ru/MEGAMIND-3482')
class TestYoutubeInForeground(object):

    owners = ('amullanurov',)

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
    def test_play_music(self, alice, command):
        response = alice(command)
        assert response.intent == intent.MusicPlay
        assert response.text == 'Секунду.'
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.WebOSLaunchAppDirective
        assert response.directive.payload.app_id == 'youtube.leanback.v4'
        result_params = json.loads(response.directive.payload.params_json)
        params = parse_qs(result_params['contentTarget'])
        assert command == unquote(params['vq'][0])


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.legatus])
class TestPlayerCommands(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('command', [
        pytest.param('перемотай на 3 минуты вперед', id='rewind'),
        pytest.param('следующий трек', id='next_track'),
        pytest.param('предыдущий трек', id='prev_track'),
        pytest.param('продолжи играть', id='continue'),
        pytest.param('лайк', id='like'),
        pytest.param('дизлайк', id='dislike', marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/MEDIAALICE-280')),
        pytest.param('включи сначала', id='replay'),
        pytest.param('поставь на повтор', id='repeat'),
        pytest.param('перемешай', id='shuffle'),
    ])
    def test_player_command(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.text in [
            'Я еще не научилась этому. Давно собираюсь, но все времени нет.',
            'Я пока это не умею.',
            'Я еще не умею это.',
            'Я не могу пока, но скоро научусь.',
            'Меня пока не научили этому.',
            'Когда-нибудь я смогу это сделать, но не сейчас.',
            'Надеюсь, я скоро смогу это делать. Но пока нет.',
            'Я не знаю, как это сделать. Извините.',
            'Так делать я еще не умею.',
            'Программист Алексей обещал это вскоре запрограммировать. Но он мне много чего обещал.',
            'К сожалению, этого я пока не умею. Но я быстро учусь.',
        ]
