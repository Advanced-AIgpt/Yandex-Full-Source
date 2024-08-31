import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


def check_playlist_info(response, playlist_name, playlist_id):
    assert response.scenario == scenario.HollywoodMusic
    assert response.intent == intent.MusicPlay
    assert response.directive.name == directives.names.AudioPlayDirective
    assert response.text == f'Включаю подборку "{playlist_name}".'

    music_event = response.scenario_analytics_info.event('music_event')
    assert music_event is not None
    assert music_event.answer_type == 'Playlist'
    assert music_event.id.startswith(playlist_id)


@pytest.mark.xfail(reason='broken, will be fixed here DIALOG-8790')
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.loudspeaker, surface.station])
class TestPalmMusicPlaylistsPleasant(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-473
    '''

    owners = ('igor-darov',)

    @pytest.mark.parametrize('command, true_responses, answer_type', [
        pytest.param(
            'включи приятную музыку',
            ['Включаю.'],
            'Filters',
            id='playlist_pleasant_1'
        ),
    ])
    def test_alice_473_pleasant(self, alice, command, true_responses, answer_type):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text in true_responses

        music_event = response.scenario_analytics_info.event('music_event')
        assert music_event is not None
        assert music_event.answer_type == answer_type


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.loudspeaker, surface.station])
class TestPalmMusicPlaylists(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-473
    https://testpalm.yandex-team.ru/testcase/alice-570
    https://testpalm.yandex-team.ru/testcase/alice-2382
    '''

    owners = ('akhruslan',)

    @pytest.mark.parametrize('command, true_responses, answer_type', [
        pytest.param(
            'найди плейлист дня',
            ['Включаю подборку "Плейлист дня".'],
            'Playlist',
            id='playlist_of_the_day_1'
        ),
        pytest.param(
            'плейлист дня',
            ['Включаю подборку "Плейлист дня".'],
            'Playlist',
            id='playlist_of_the_day_2'
        ),
        pytest.param(
            'включи плейлист премьера',
            ['Включаю подборку "Премьера".'],
            'Playlist',
            id='playlist_premier_1'
        ),
        pytest.param(
            'плейлист премьера',
            ['Включаю подборку "Премьера".'],
            'Playlist',
            id='playlist_premier_2'
        ),
        pytest.param(
            'поставь плейлист дежавю',
            ['Включаю подборку "Дежавю".'],
            'Playlist',
            id='playlist_dejavu_1'
        ),
        pytest.param(
            'плейлист дежавю',
            ['Включаю подборку "Дежавю".'],
            'Playlist',
            id='playlist_dejavu_2'
        ),
        pytest.param(
            'включи мою любимую музыку',
            [
                'Послушаем ваше любимое.',
                'Включаю ваши любимые песни.',
                'Люблю песни, которые вы любите.',
                'Окей. Плейлист с вашей любимой музыкой.',
                'Окей. Песни, которые вам понравились.'
            ],
            'Playlist',
            id='playlist_favourite_1'
        ),
        pytest.param(
            'включи мой плейлист',
            [
                'Послушаем ваше любимое.',
                'Включаю ваши любимые песни.',
                'Люблю песни, которые вы любите.',
                'Окей. Плейлист с вашей любимой музыкой.',
                'Окей. Песни, которые вам понравились.'
            ],
            'Playlist',
            id='playlist_favourite_2'
        ),
        pytest.param(
            'включи плейлист мне нравится',
            [
                'Послушаем ваше любимое.',
                'Включаю ваши любимые песни.',
                'Люблю песни, которые вы любите.',
                'Окей. Плейлист с вашей любимой музыкой.',
                'Окей. Песни, которые вам понравились.'
            ],
            'Playlist',
            id='playlist_favourite_3'
        ),
    ])
    def test_alice_473(self, alice, command, true_responses, answer_type):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text in true_responses

        music_event = response.scenario_analytics_info.event('music_event')
        assert music_event is not None
        assert music_event.answer_type == answer_type

    @pytest.mark.parametrize('command', [
        'включи твой плейлист на яндекс.музыке',
        'включи плейлист с комментариями',
        'включи плейлист с шотами',
        'включи плейлист в котором ты комментируешь песни',
        'включи песни с твоими комментариями',
        'включи плейлист от Алисы',
        'включи плейлист Алисы',
        'включи плейлист с Алисой',
    ])
    def test_alice_2382(self, alice, command):
        response = alice(command)
        check_playlist_info(response, 'Плейлист с Алисой', '940441070:17850265')

    @pytest.mark.parametrize('command, playlist_name, playlist_id', [
        pytest.param(
            # https://music.yandex.ru/users/robot-alice-tests-plus/playlists/1001
            # FIXME(akhruslan): there is a discrepency in names which will be fixed after music DB update
            'Алиса, включи плейлист абацаба',
            'абацаба',
            '1083955728:',
            id='users_own_playlist'
        ),
    ])
    def test_alice_570(self, alice, command, playlist_name, playlist_id):
        response = alice(command)
        check_playlist_info(response, playlist_name, playlist_id)

    @pytest.mark.parametrize('command, playlist_name, playlist_id', [
        pytest.param(
            'Алиса, включи плейлист пилите шура',
            'Пилите Шура',  # Должно включиться что-то, но не этот плейлист, он приватный и принадлежит не нам
            '686859036:1001',
            id='another_users_playlist_negative'
        ),
        pytest.param(
            'Алиса, включи плейлист лесной переключатель',
            'Лесной переключатель',
            '687820164:1004',
            id='another_users_playlist_negative_2'
        ),
        pytest.param(
            'Алиса, включи плейлист помидор соус',
            'Помидор соус',
            '653600380:1001',
            id='another_users_playlist_negative_3'
        ),
    ])
    def test_alice_570_negative(self, alice, command, playlist_name, playlist_id):
        response = alice(command)

        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert playlist_name.lower() not in response.text.lower()

        music_event = response.scenario_analytics_info.event('music_event')
        assert music_event is not None
        assert music_event.answer_type == 'Playlist'
        assert not music_event.id.startswith(playlist_id)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.loudspeaker, surface.station])
class TestAdultMusic(object):

    owners = ('ardulat',)

    def test_adult_music(self, alice):
        response = alice('включи взрослую музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text in [
            'Послушаем ваше любимое.',
            'Включаю ваши любимые песни.',
            'Окей. Плейлист с вашей любимой музыкой.',
            'Люблю песни, которые вы любите.',
            'Окей. Песни, которые вам понравились.',
        ]

        assert 'personality' in response.slots
        assert response.slots['personality'].string == 'is_personal'


@pytest.mark.xfail(reason='flaky test, will be fixed in ALICE-12783')
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.searchapp, surface.station])
class TestRewindPlaylists(object):

    owners = ('mike88')

    @pytest.mark.parametrize('command, playlist_owner', [
        ('включи перемотку мой 2020', '1216955353'),
        ('включи большую перемотку', '1162487387'),
        ('летнюю перемотку', '924481842'),
        ('включи подборку детская перемотка', '1399951413'),
        ('включи подборку новогодняя перемотка девятнадцать', '986783119'),
    ])
    def test_rewind_playlists(self, alice, command, playlist_owner):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        if surface.is_smart_speaker(alice):
            assert 'Включаю' in response.text
            assert response.directive.name == directives.names.AudioPlayDirective
        else:
            assert response.directive.name == directives.names.OpenUriDirective
            assert f'owner={playlist_owner}' in response.directive.payload.uri
        music_event = response.scenario_analytics_info.event('music_event')
        assert music_event is not None
        assert playlist_owner in music_event.uri
        assert playlist_owner in music_event.id


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.loudspeaker, surface.station])
class TestUnverifiedPlaylist(object):

    owners = ('mike88',)

    def test_verified_playlist(self, alice):
        response = alice('включи подборку хроника электроника')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Включаю подборку "Хроника электроника".'

    def test_unverified_playlist(self, alice):
        response = alice('включи плейлист скк витязь')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert 'других пользователей' in response.text
