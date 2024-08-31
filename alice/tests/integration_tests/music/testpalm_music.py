import re

import alice.library.restriction_level.protos.content_settings_pb2 as content_settings_pb2
import pytest

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface


def _assert_open_uri(directive, assert_uri_func):
    assert directive.name == directives.names.OpenUriDirective
    assert assert_uri_func(directive.payload.uri)


def _assert_musicsdk_uri(directive, assert_uri_func):
    _assert_open_uri(
        directive,
        lambda uri: uri.startswith('musicsdk://') and 'from=musicsdk-ru_yandex_' in uri and 'play=true' in uri and assert_uri_func(uri)
    )


def _assert_radio_uri(directive, radio_uri):
    _assert_open_uri(
        directive,
        lambda uri: uri == f'https://radio.yandex.ru/{radio_uri}?from=alice&mob=0&play=1'
    )


def _assert_music_play_directive(response):
    assert response.scenario == scenario.HollywoodMusic
    assert response.intent == intent.MusicPlay
    assert response.directive.name == directives.names.MusicPlayDirective


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.dexp])
class TestPalmMusicStation(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-460
    """

    owners = ('vitvlkv',)

    def test_alice_460(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert not response.directive
        assert response.text == 'Чтобы слушать музыку, вам нужно оформить подписку Яндекс.Плюс.'


@pytest.mark.parametrize('surface', [surface.watch])
class TestPalmMusicWatch(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-20
    https://testpalm.yandex-team.ru/testcase/alice-24
    """

    owners = ('vitvlkv',)

    @pytest.mark.parametrize('command', [
        'Поставь весёлую музыку',
        'Поставь группу queen',
        'Поставь музыку для бега',
    ])
    def test_alice_20(self, alice, command):
        response = alice(command)
        assert response.intent == intent.ProhibitionError
        assert not response.directive
        assert response.text in [
            'В часах такое провернуть сложновато.',
            'Я бы и рада, но здесь не могу. Эх.',
            'Здесь точно не получится.',
        ]

    def test_alice_24(self, alice):
        response = alice('Следующая песня')
        assert response.intent == intent.GeneralConversation
        assert not response.directive
        assert response.text


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.automotive])
class TestPalmMusicAutomotive(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1392
    """

    owners = ('vitvlkv',)

    def test_alice_1392(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        _assert_radio_uri(response.directive, 'user/onyourwave')

        response = alice('следующий')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.PlayNextTrack
        _assert_open_uri(response.directive, lambda uri: uri == 'yandexauto://media_control?action=next')

    @pytest.mark.parametrize('command, music_genre', [
        ('включи рок', 'rock'),
        pytest.param(
            'радио русский рок', 'rusrock',
            marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-7218'),
        ),
        ('вруби музыку из фильмов', 'films'),
    ])
    def test_alice_1392_genre(self, alice, command, music_genre):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        _assert_radio_uri(response.directive, f'genre/{music_genre}')


@pytest.mark.parametrize('surface', [surface.old_automotive])
class TestPalmMusicOldAutomotive(object):
    """
    Частично https://testpalm.yandex-team.ru/testcase/alice-1800
    """

    owners = ('vitvlkv',)

    def test_alice_1800(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.CarDirective
        assert response.directive.payload.intent == 'launch'
        assert response.directive.payload.params.app == 'yandexradio'

        assert response.text == 'Включаю вашу радиостанцию.'
        listen_on_music = response.button('Слушать на Яндекс.Музыке')
        assert listen_on_music, 'Expect "Слушать на Яндекс.Музыке" button in response.'
        _assert_open_uri(listen_on_music.directives[0], lambda uri: 'https://radio.yandex.ru/user/onyourwave?from=alice&mob=0&play=1' == uri)

        response = alice('следующий')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.PlayNextTrack
        assert response.directive.name == directives.names.CarDirective
        assert response.directive.payload.intent == 'media_control'
        assert response.directive.payload.params.action == 'next'
        assert not response.has_voice_response()


@pytest.mark.parametrize('surface', [surface.dexp])
class TestPalmMusicNextPreviousTracks(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-587
    """

    owners = ('sparkle',)

    @pytest.mark.oauth(auth.YandexPlus)
    def test_alice_587(self, alice):
        response = alice('включи rammstein - alter mann')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective

        response = alice('следующий трек')
        assert response.intent == intent.PlayNextTrack
        assert response.directive.name == directives.names.PlayerNextTrackDirective

        response = alice('предыдущий трек')
        assert response.intent == intent.PlayPreviousTrack
        assert response.directive.name == directives.names.PlayerPreviousTrackDirective

        response = alice('следующий трек')
        assert response.intent == intent.PlayNextTrack
        assert response.directive.name == directives.names.PlayerNextTrackDirective


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.dexp(device_config={'content_settings': content_settings_pb2.children}),
])
class TestPalmFamilyModeMusic(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1441
    """

    owners = ('vitvlkv',)

    @pytest.mark.parametrize('command', [
        'Включи кровосток',
        'Включи Ленинград песню Экспонат',
    ])
    def test_alice_1441(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert not response.directive
        assert response.text in [
            'Я не могу поставить эту музыку в детском режиме.',
            'Знаю такое, но не могу поставить в детском режиме.',
            'В детском режиме такое включить не получится.',
            'Не могу. Знаете почему? У вас включён детский режим.',
            'Я бы и рада, но у вас включён детский режим поиска.',
            'Не выйдет. У вас включён детский режим, а это не для детских ушей.',
        ]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.navi])
class TestPalmMusicInNavi(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1450
    """

    owners = ('abc:alice_scenarios_music',)

    @pytest.mark.parametrize('command, text_re, musicsdk_cgi', [
        (
            'включи мою музыку',
            re.compile(
                r'(^Послушаем ваше любимое.*|^Включаю ваши любимые песни.*|^Окей. Плейлист с вашей любимой музыкой.*|' +
                r'^Люблю песни, которые вы любите\.$|^Окей. Песни, которые вам понравились\.$)'),
            'kind=3&owner=1083955728',
        ),
        (
            'поставь judas priest',
            re.compile(r'^Включаю: Judas Priest\.$'),
            'artist=171309',
        ),
        (
            'включи рок',
            re.compile(r'^Включаю\.'),
            'tag=rock',
        ),
        (
            'поставь пугачеву',
            re.compile(r'^Включаю: Алла Пугачёва\.$'),
            'artist=167026',
        ),
        (
            'включи песню behind blue eyes',
            re.compile(r'^Включаю: .*Behind Blue Eyes'),
            'track=34608',
        ),
        (
            'включи музыку из фильмов',
            re.compile(r'^Включаю\.'),
            'tag=films',
        ),
    ])
    def test_alice_1450(self, alice, text_re, command, musicsdk_cgi):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        _assert_musicsdk_uri(response.directive, lambda uri: musicsdk_cgi in uri)
        assert re.match(text_re, response.text)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.device_state(device_config={'content_settings': content_settings_pb2.safe})
@pytest.mark.parametrize('surface', [surface.dexp])
class TestSafeModeMusicRadio(object):

    owners = ('abc:alice_scenarios_music', 'ardulat',)

    @pytest.mark.parametrize('command', [
        'Включи рок',
        'Включи музыку восьмидесятых',
        'Поставь веселую музыку',
        'Поставь музыку для бега',
    ])
    def test_prohibition(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text == 'Это лучше слушать вместе с родителями - попроси их включить. А пока для тебя - детская музыка.'

    def test_child_music(self, alice):
        response = alice('Включи детскую музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text in [
            'Поняла. Для вас - детская музыка.',
            'Легко. Для вас - детская музыка.',
            'Детская музыка - отличный выбор.',
        ]


@pytest.mark.skip('HOLLYWOOD-169')  # breaks when moving away from VINS, we decided it's okay
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPalmMusicAnaphora(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-1276
    '''

    owners = ('abc:alice_scenarios_music',)

    def test_anaphora(self, alice):
        response = alice('Кто такой Фредди Меркьюри?')
        assert response.intent in [intent.Search, intent.Factoid, intent.ObjectAnswer]

        response = alice('Включи его песни')
        assert response.text == 'Включаю исполнителя'
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.OpenUriDirective

        assert any(
            f'artist={artist}' in response.directive.payload.uri
            for artist in [
                3120,  # Freddie Mercury
                79215,  # Queen
            ]
        )


@pytest.mark.oauth(auth.YandexPlus)
class TestPalmFixedPlaylist(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-1107
    https://testpalm.yandex-team.ru/testcase/alice-1828
    https://testpalm.yandex-team.ru/testcase/alice-2165
    '''

    owners = ('abc:alice_scenarios_music',)

    @pytest.mark.voice
    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command, musicsdk_cgi', [
        ('Поставь плейлист вечные хиты', 'kind=1250&owner=105590476'),
        ('Включи подборку лёгкое электронное утро', 'kind=1459&owner=103372440'),
        ('Включи громкие новинки месяца', 'kind=1175&owner=103372440'),
    ])
    def test_fixed_playlist_searchapp(self, alice, command, musicsdk_cgi):
        response = alice(command)
        assert response.output_speech_text == 'Включаю'
        assert len(response.cards) == 1
        assert response.div_card
        assert response.cards[0].type == 'div2_card'
        assert not response.text_card
        assert response.intent == intent.MusicPlay
        _assert_musicsdk_uri(response.directive, lambda uri: musicsdk_cgi in uri)

    @pytest.mark.parametrize('surface', [surface.navi])
    @pytest.mark.parametrize('play_on_command, playlist, musicsdk_cgi', [
        ('Поставь плейлист', 'Вечные хиты', 'kind=1250&owner=105590476'),
        ('Включи подборку', 'Лёгкое электронное утро', 'kind=1459&owner=103372440'),
        ('Включи', 'Громкие новинки месяца', 'kind=1175&owner=103372440'),
    ])
    def test_fixed_playlist_navi(self, alice, play_on_command, playlist, musicsdk_cgi):
        response = alice(f'{play_on_command} {playlist}')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.text == f'Включаю подборку "{playlist}".'
        _assert_musicsdk_uri(response.directive, lambda uri: musicsdk_cgi in uri)

    @pytest.mark.parametrize('surface', [surface.yabro_win])
    @pytest.mark.parametrize('command, music_uri', [
        ('Поставь плейлист вечные хиты',
         'https://music.yandex.ru/users/ya.playlist/playlists/1250/?from=alice&mob=0&play=1',),
        ('Включи подборку лёгкое электронное утро',
         'https://music.yandex.ru/users/music-blog/playlists/1459/?from=alice&mob=0&play=1',),
        pytest.param('Включи громкие новинки месяца',
                     'https://music.yandex.ru/users/music-blog/playlists/1175/?from=alice&mob=0&play=1',
                     marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-9516')),
    ])
    def test_fixed_playlist_yabro_win(self, alice, command, music_uri):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.text == 'Включаю плейлист'
        _assert_open_uri(response.directive, lambda uri: music_uri in uri)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.yabro_win])
class TestPalmMusicYabroWin(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2506
    """

    owners = ('abc:alice_scenarios_music',)

    def test_music_on(self, alice):
        response = alice('Включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.text == 'Включаю вашу радиостанцию.'
        _assert_open_uri(response.directive, lambda uri: 'https://radio.yandex.ru/user/onyourwave?from=alice&mob=0&play=1' == uri)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.dexp])
class TestPalmMusicRadio(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-475
    """

    owners = ('abc:alice_scenarios_music',)

    @pytest.mark.parametrize('command, genre', [
        ('включи джаз', 'джаз'),
        ('вруби рок', 'рок'),
        ('запусти индастриал', 'индастриал'),
        ('включи песню в жанре металл', 'метал'),
        ('запусти песню в жанре рэп', 'рэп и хип-хоп'),
        ('хочу песню в жанре кантри', 'кантри'),
    ])
    def test_alice_475_genre(self, alice, command, genre):
        response = alice(command)
        _assert_music_play_directive(response)
        assert response.text in [
            f'Поняла. Для вас - {genre}.',
            f'Легко. Для вас - {genre}.',
            f'{genre} - отличный выбор.'.capitalize(),
        ]

    @pytest.mark.parametrize('command, mood', [
        ('включи радостную музыку', 'вес[е|ё]л'),
        ('давай послушаем грустное', 'грустн'),
        ('поставь что-то повеселее', 'вес[е|ё]л'),
    ])
    def test_alice_475_mood(self, alice, command, mood):
        expected_response = [
            f'Это как раз подойдёт под {mood}ое настроение.',
            f'Вот, отлично подойдёт под {mood}ое настроение.',
            f'Есть отличная музыка для {mood}ого настроения.',
            f'Знаю подходящую музыку для {mood}ого настроения.',
            f'Вот, самое то для {mood}ого настроения.',
        ]
        response = alice(command)
        _assert_music_play_directive(response)
        assert any(re.match(text, response.text) for text in expected_response)

    def test_alice_475_eternal_hits(self, alice):
        response = alice('запусти вечные хиты')
        _assert_music_play_directive(response)
        assert response.text == 'Включаю вечные хиты.'

    @pytest.mark.parametrize('command, epoch', [
        ('включи музыку пятидесятых', '1950-х'),
        ('давай послушаем музыку шестидесятых', '1960-х'),
        ('поставь музыку восьмидесятых', '1980-х'),
    ])
    def test_alice_475_epoch(self, alice, command, epoch):
        response = alice(command)
        _assert_music_play_directive(response)
        assert response.text == f'Включаю музыку {epoch}.'

    @pytest.mark.parametrize('command, epoch', [
        ('включи кельсткую музыку', 'кельтская музыка'),
        ('запусти восточную музыку', 'восточная музыка'),
    ])
    def test_alice_475_nationality_region(self, alice, command, epoch):
        response = alice(command)
        _assert_music_play_directive(response)
        assert response.text in [
            f'Поняла. Для вас - {epoch}.',
            f'Легко. Для вас - {epoch}.',
            f'{epoch} - отличный выбор.'.capitalize(),
        ]

    def test_alice_475_background_music(self, alice):
        response = alice('включи фоновую музыку')
        _assert_music_play_directive(response)
        assert response.text == 'Окей. Включаю фоновую музыку.'

    @pytest.mark.parametrize('command, activity', [
        ('запусти музыку для вечеринки', 'вечеринки'),
        ('поставь музыку для тренировки', 'тренировки'),
    ])
    def test_alice_475_activity(self, alice, command, activity):
        response = alice(command)
        _assert_music_play_directive(response)
        assert response.text in [
            f'Вот, отлично подойдет для {activity}.',
            f'Вот, как раз для {activity}.',
            f'Включаю музыку для {activity}.',
            f'Хорошо, музыка для {activity}.',
            f'Окей. Музыка для {activity}.',
        ]

    @pytest.mark.parametrize('command', [
        'поставь музыку на русском языкe',
        'включи музыку на английском',
    ])
    def test_alice_475_language(self, alice, command):
        response = alice(command)
        _assert_music_play_directive(response)
        assert response.text == 'Включаю.'

    @pytest.mark.xfail(reason="https://st.yandex-team.ru/HOLLYWOOD-1022")
    def test_alice_475_combination(self, alice):
        expected_answers_rap = {
            'Есть кое-что для вас.',
            'Есть одна идея.',
            'Такое у меня есть.',
            'Есть музыка на этот случай.',
        }
        expected_answers_rock = [
            'Нашла что-то подходящее среди плейлистов других пользователей. Включаю ',
            'Вот что я нашла среди плейлистов других пользователей. Включаю ',
            'Включаю подборку "Вечный рок".',
        ]

        response = alice('включи грустный рэп')
        _assert_music_play_directive(response)
        assert response.text in expected_answers_rap

        response = alice('поставь музыку 60-ых на английском')
        _assert_music_play_directive(response)
        assert response.text == 'Включаю музыку 1960-х.'

        response = alice('вруби рок для тренировки')
        _assert_music_play_directive(response)
        assert any([text in response.text for text in expected_answers_rock]), \
               f'response.text={response.text} does not matches expected_answers_rock={expected_answers_rock}'

    @pytest.mark.parametrize('command', [
        'включи музыку с мужским вокалом',
        'запусти музыку с женским вокалом',
    ])
    def test_alice_475_vocal(self, alice, command):
        response = alice(command)
        _assert_music_play_directive(response)
        assert response.text == 'Включаю.'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.dexp])
class TestPalmMusicSearchPluggedOutTv(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-486
    '''

    owners = ('abc:alice_scenarios_music',)

    @pytest.mark.parametrize('command, titles', [
        ('включи группу кино', ['КИНО']),
        ('поставь конец фильма', ['Конец фильма']),
        ('включи ice cube', ['Ice Cube']),
        ('поставь Waiting For The End', ['Linkin Park', 'A Thousand Suns', 'Waiting for the End']),
        ('включи Velvet Underground', ['The Velvet Underground']),
        ('поставь Beatles', ['The Beatles']),
        ('вруби David Bowie', ['David Bowie']),
        ('врубай Melanie Martinez', ['Melanie Martinez']),
        ('включи Oasis', ['Oasis']),
        ('включи Fall Out Boy', ['Fall Out Boy']),
        ('хочу Mylene Farmer', ['Mylène Farmer']),
        ('включи Белая Гвардия', ['Белая Гвардия']),
        ('давай послушаем Scorpions', ['Scorpions']),
        ('включи Gogol Bordello', ['Gogol Bordello']),
        ('поставь Титаник', ['Титаник', 'My Heart Will Go On']),  # Тут может быть Nautilus Pompilius Титаник, или OST из фильма Титаник...
    ])
    def test_alice_486(self, alice, command, titles):
        response = alice(command)
        _assert_music_play_directive(response)
        assert(any(title.lower() in response.text.lower() for title in titles))


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.dexp])
class TestPalmMusicSearchRequests(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-494
    '''

    owners = ('zhigan',)

    @pytest.mark.parametrize('command, title', [
        ('Включи понедельник 2 сорт', '2-й сорт, альбом "Личное дело", песня "Понедельник"'),
        ('Поставь 30 секунд до марса', 'Thirty Seconds to Mars'),
        ('Включи группу sum 41', 'Sum 41'),
        ('Включи пора домой', 'Сектор Газа, альбом "Вой на Луну. Лучшее и неизданное", песня "Пора домой"'),
        ('Включи музыку из лайн эйдж два', r'(?i)Lineage (II|2)'),
        pytest.param(
            'Поставь 2 сорт алкоголь', '2-й сорт, альбом "Личное дело", песня "Алкоголь"',
            marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-134')
        ),
    ])
    def test_alice_494_songs(self, alice, command, title):
        response = alice(command)
        _assert_music_play_directive(response)
        assert 'Включаю' in response.text
        assert re.search(title, response.text)

    def test_alice_494_videogames_soundtracks(self, alice):
        genre = 'музыка из видеоигр'
        response = alice('Влючи саундтреки к играм')
        _assert_music_play_directive(response)
        assert response.text in [
            f'Поняла. Для вас - {genre}.',
            f'Легко. Для вас - {genre}.',
            f'{genre} - отличный выбор.'.capitalize(),
        ]


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPalmMusicCards(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-2526
    '''

    owners = ('zhigan',)

    @pytest.mark.parametrize('command, text, musicsdk_cgi, title, author', [
        ('Включи песню шоу маст гоу он группы квин', 'Включаю', None, 'The Show Must Go On', 'Queen'),
        ('Включи гранатовый альбом', 'Включаю альбом', 'album=60058', 'Гранатовый альбом', 'Сплин'),
        ('Включи скорпионс', 'Включаю исполнителя', 'artist=90', 'Scorpions', None),
        ('Включи плейлист вечные хиты', 'Включаю плейлист', 'kind=1250&owner=105590476', 'Вечные хиты', None),
        ('Включи радостную музыку', 'Включаю плейлист', 'kind=1062&owner=837761439', 'Радостное', None),
    ])
    def test_alice_2526(self, alice, command, text, musicsdk_cgi, title, author):
        response = alice(command)
        assert response.output_speech_text == 'Включаю'
        assert len(response.cards) == 1
        assert response.div_card
        assert response.cards[0].type == 'div2_card'  # TODO: check content of the div2 card if possible
        assert not response.text_card
        _assert_musicsdk_uri(response.directive, lambda uri: musicsdk_cgi in uri if musicsdk_cgi else True)
