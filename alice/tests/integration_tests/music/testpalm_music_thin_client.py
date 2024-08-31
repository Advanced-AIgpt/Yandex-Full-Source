import re

import alice.library.restriction_level.protos.content_settings_pb2 as content_settings_pb2
import pytest

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface

NEED_YA_PLUS_RESPONSE_TEXTS = [
    'Чтобы слушать музыку, вам нужно оформить подписку Яндекс.Плюс.',
    'Извините, но я пока не могу включить то, что вы просите. Для этого необходимо оформить подписку на Плюс.',
    'Простите, я бы с радостью, но у вас нет подписки на Плюс.',
]

PLAYER_LIKE_RESPONSE_TEXTS = [
    'Буду включать такое чаще.',
    'Запомню, что вам такое по душе.',
    'Рада, что вы оценили.',
    'Поставила лайк.',
    'Круто! Ставлю лайк.',
    'Уже поставила лайк.',
    'Поставила лайк за вас.',
]

PLAYER_LIKE_BUT_UNKNOWN_MUSIC_RESPONSE_TEXTS = [
    'Не могу понять какую песню лайкать.',
    'Я бы с радостью, но не знаю какую песню лайкать.',
]

PLAYER_DISLIKE_RESPONSE_TEXTS = [
    'Дизлайк принят.',
    'Хорошо, ставлю дизлайк.',
    'Окей, не буду такое ставить.',
    'Поняла. Больше не включу.',
    'Нет проблем, поставила дизлайк.',
    'Поставила дизлайк.',
]

PLAYER_SHOT_WHAT_IS_PLAYING_RESPONSE_TEXTS = [
    'Сейчас в эфире мой шот.',
    'Сейчас звучит мой мудрый шот.',
    'А это мой шот - специально для вас.',
    'Это мой шот. Я рада, что вы заинтересовались.',
]


def _assert_audio_play_directive(response):
    assert response.scenario == scenario.HollywoodMusic
    assert response.intent == intent.MusicPlay
    assert response.directive.name == directives.names.AudioPlayDirective


def _assert_has_audio_play_directive(response, intent=None):
    if intent is not None:
        assert response.intent == intent
    directive = response.get_directive(directives.names.AudioPlayDirective)
    assert directive.name == directives.names.AudioPlayDirective


def _assert_has_player_pause_directive(response):
    assert response.scenario == scenario.Commands
    assert response.intent == intent.PlayerPause
    directive = response.get_directive(directives.names.ClearQueueDirective)
    assert directive.name == directives.names.ClearQueueDirective


def _assert_player_successful_like(response):
    assert response.scenario == scenario.HollywoodMusic
    assert response.intent == intent.PlayerLike
    assert response.text in PLAYER_LIKE_RESPONSE_TEXTS


def _assert_player_successful_dislike(response, should_change_to_next=True):
    assert response.scenario == scenario.HollywoodMusic
    assert response.intent == intent.PlayerDislike
    assert response.text in PLAYER_DISLIKE_RESPONSE_TEXTS
    if should_change_to_next:
        _assert_has_audio_play_directive(response)


class _ResponseAudioTrack(object):
    def __init__(self, audio_play_response):
        directive = audio_play_response.get_directive(directives.names.AudioPlayDirective)
        assert directive.name == directives.names.AudioPlayDirective
        self.stream_id = directive.payload.stream.id
        self.title = directive.payload.metadata.title
        self.subtitle = directive.payload.metadata.subtitle

    def get_track_string_for_request(self):
        return f'{self.subtitle} - {self.title}'


def _is_shot_playing(response):
    directive = response.get_directive(directives.names.AudioPlayDirective)
    return directive.payload.stream.type == 'Shot'


@pytest.mark.experiments('music_force_show_first_track')
@pytest.mark.parametrize('surface', [surface.station])
class TestPalmMusicStation(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-440
    https://testpalm.yandex-team.ru/testcase/alice-460
    """

    owners = ('vitvlkv',)

    @pytest.mark.oauth(auth.YandexPlus)
    def test_alice_440(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Включаю.'

        response = alice('домой')
        assert response.directives[0].name == directives.names.MordoviaShowDirective
        assert response.directives[-1].name == directives.names.ClearQueueDirective

        response = alice('продолжить слушать')
        assert response.intent == intent.PlayerContinue
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('домой')
        assert response.directives[0].name == directives.names.MordoviaShowDirective
        assert response.directives[-1].name == directives.names.ClearQueueDirective

        response = alice('включи')
        assert response.intent == intent.PlayerContinue
        assert response.directive.name == directives.names.AudioPlayDirective

    @pytest.mark.oauth(auth.Yandex)
    def test_alice_460(self, alice):
        response = alice('включи музыку')
        assert response.text in NEED_YA_PLUS_RESPONSE_TEXTS
        # TODO: Uncomment this after fix https://st.yandex-team.ru/ALICE-13948
        # assert response.intent == intent.MusicPlay
        assert not response.directive


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
class TestPalmMusicNextPreviousTracks(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-587
    """

    owners = ('sparkle',)

    def test_alice_587(self, alice):
        response = alice('включи rammstein - alter mann')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        first_track_title = response.directive.payload.metadata.title

        alice.skip(seconds=5)

        response = alice('следующий трек')
        assert response.intent == intent.PlayNextTrack
        assert response.directive.name == directives.names.AudioPlayDirective, 'Включается радио autoflow'
        second_track_title = response.directive.payload.metadata.title

        alice.skip(seconds=5)

        response = alice('предыдущий трек')
        assert response.intent == intent.PlayPreviousTrack
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.directive.payload.metadata.title == first_track_title

        alice.skip(seconds=5)

        response = alice('следующий трек')
        assert response.intent == intent.PlayNextTrack
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.directive.payload.metadata.title == second_track_title


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.device_state(device_config={'content_settings': content_settings_pb2.children})
@pytest.mark.parametrize('surface', [surface.loudspeaker])
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
        # Fix bug https://st.yandex-team.ru/ALICE-13948 and uncomment:
        # assert response.intent == intent.MusicPlay
        assert response.text in [
            'Я не могу поставить эту музыку в детском режиме.',
            'Знаю такое, но не могу поставить в детском режиме.',
            'В детском режиме такое включить не получится.',
            'Не могу. Знаете почему? У вас включён детский режим.',
            'Я бы и рада, но у вас включён детский режим поиска.',
            'Не выйдет. У вас включён детский режим, а это не для детских ушей.',
        ]
        assert not response.directive


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.device_state(device_config={'content_settings': content_settings_pb2.safe})
@pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
@pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-14827')
class TestSafeModeMusicRadio(object):

    owners = ('ardulat', 'vitvlkv', 'abc:alice_scenarios_music')

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
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Это лучше слушать вместе с родителями - попроси их включить. А пока для тебя - детская музыка.'

    def test_child_music(self, alice):
        response = alice('Включи детскую музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text in [
            'Поняла. Для вас - детская музыка.',
            'Легко. Для вас - детская музыка.',
            'Детская музыка - отличный выбор.',
        ]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.loudspeaker, surface.station])
class TestPalmMusicRadio(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-475
    """

    owners = ('zhigan',)

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
        _assert_audio_play_directive(response)
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
        expected_response = {
            f'Это как раз подойдёт под {mood}ое настроение.',
            f'Вот, отлично подойдёт под {mood}ое настроение.',
            f'Есть отличная музыка для {mood}ого настроения.',
            f'Знаю подходящую музыку для {mood}ого настроения.',
            f'Вот, самое то для {mood}ого настроения.',
        }
        response = alice(command)
        _assert_audio_play_directive(response)
        assert any(re.match(text, response.text) for text in expected_response)

    def test_alice_475_eternal_hits(self, alice):
        response = alice('запусти вечные хиты')
        _assert_audio_play_directive(response)
        assert response.text == 'Включаю вечные хиты.'

    @pytest.mark.parametrize('command, epoch', [
        ('включи музыку пятидесятых', '1950-х'),
        ('давай послушаем музыку шестидесятых', '1960-х'),
        ('поставь музыку восьмидесятых', '1980-х'),
    ])
    def test_alice_475_epoch(self, alice, command, epoch):
        response = alice(command)
        _assert_audio_play_directive(response)
        assert response.text == f'Включаю музыку {epoch}.'

    @pytest.mark.parametrize('command, epoch', [
        ('включи кельсткую музыку', 'кельтская музыка'),
        ('запусти восточную музыку', 'восточная музыка'),
    ])
    def test_alice_475_nationality_region(self, alice, command, epoch):
        response = alice(command)
        _assert_audio_play_directive(response)
        assert response.text in [
            f'Поняла. Для вас - {epoch}.',
            f'Легко. Для вас - {epoch}.',
            f'{epoch} - отличный выбор.'.capitalize(),
        ]

    def test_alice_475_background_music(self, alice):
        response = alice('включи фоновую музыку')
        _assert_audio_play_directive(response)
        assert response.text == 'Окей. Включаю фоновую музыку.'

    @pytest.mark.parametrize('command, activity', [
        ('запусти музыку для вечеринки', 'вечеринки'),
        ('поставь музыку для тренировки', 'тренировки'),
    ])
    def test_alice_475_activity(self, alice, command, activity):
        response = alice(command)
        _assert_audio_play_directive(response)
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
        _assert_audio_play_directive(response)
        assert response.text == 'Включаю.'

    @pytest.mark.xfail(reason="https://st.yandex-team.ru/HOLLYWOOD-1025")
    def test_alice_475_combination(self, alice):
        expected_answers_rap = {
            'Есть кое-что для вас.',
            'Есть одна идея.',
            'Такое у меня есть.',
            'Есть музыка на этот случай.',
        }
        expected_answers_rock = {
            'Нашла что-то подходящее среди плейлистов других пользователей. Включаю ',
            'Вот что я нашла среди плейлистов других пользователей. Включаю ',
            'Включаю подборку "Вечный рок".',
        }

        response = alice('включи грустный рэп')
        _assert_audio_play_directive(response)
        assert response.text in expected_answers_rap

        response = alice('поставь музыку 60-ых на английском')
        _assert_audio_play_directive(response)
        assert response.text == 'Включаю музыку 1960-х.'

        response = alice('вруби рок для тренировки')
        _assert_audio_play_directive(response)
        assert any([answer in response.text for answer in expected_answers_rock]), \
               f'response.text={response.text} does not matches expected_answers_rock={expected_answers_rock}'

    @pytest.mark.parametrize('command', [
        'включи музыку с мужским вокалом',
        'запусти музыку с женским вокалом',
    ])
    def test_alice_475_vocal(self, alice, command):
        response = alice(command)
        _assert_audio_play_directive(response)
        assert response.text == 'Включаю.'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station(is_tv_plugged_in=False),
    surface.station_pro(is_tv_plugged_in=False),
])
class TestPalmMusicSearchPluggedOutTv(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-486
    '''

    owners = ('abc:alice_scenarios_music',)

    @pytest.mark.parametrize('command, titles', [
        pytest.param(
            'включи кино',
            ['КИНО'],
            marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-7438'),
        ),
        ('поставь конец фильма', ['Конец фильма']),
        ('включи ice cube', ['Ice Cube']),
        ('поставь Waiting For The End', ['Linkin Park', 'Waiting for the End']),
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
        _assert_audio_play_directive(response)
        assert(any(title.lower() in response.text.lower() for title in titles))


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station,
])
class TestPalmMusicSearchRequests(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-494
    '''

    owners = ('zhigan',)

    @pytest.mark.parametrize('command, title', [
        ('Включи понедельник 2 сорт', '2-й сорт, песня "Понедельник"'),
        ('Поставь 30 секунд до марса', 'Thirty Seconds to Mars'),
        ('Включи группу sum 41', 'Sum 41'),
        ('Включи пора домой', 'Сектор Газа, песня "Пора домой"'),
        ('Включи музыку из лайн эйдж два', r'(?i)Lineage (II|2)'),
        pytest.param(
            'Поставь 2 сорт алкоголь', '2-й сорт, альбом "Личное дело", песня "Алкоголь"',
            marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-134')
        ),
    ])
    def test_alice_494_songs(self, alice, command, title):
        response = alice(command)
        _assert_audio_play_directive(response)
        assert 'Включаю' in response.text
        assert re.search(title, response.text)

    def test_alice_494_videogames_soundtracks(self, alice):
        genre = 'музыка из видеоигр'
        response = alice('Влючи саундтреки к играм')
        _assert_audio_play_directive(response)
        assert response.text in [
            f'Поняла. Для вас - {genre}.',
            f'Легко. Для вас - {genre}.',
            f'{genre} - отличный выбор.'.capitalize(),
        ]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.smart_tv])
class TestMusicTv(object):
    owners = ('ardulat', 'olegator')

    @pytest.mark.parametrize('command', [
        'Включи AC DC',
        'Включи rammstein',
        'Включи леди гага покер фейс',
        'Включи группу сплин',
        'Включи linkin park',
        'Включи queen',
        'Включи pink floyd',
        'Включи майкл джексон',
        'Включи эминэм',
        'Включи любэ конь',
        'Включи музыку',
        'Включи рок',
    ])
    def test_search_text(self, alice, command):
        response = alice(command)
        # На тв музыкальные клипы тоже релевантны
        assert response.scenario in [scenario.HollywoodMusic, scenario.Video]
        if response.scenario == scenario.HollywoodMusic:
            assert response.intent == intent.MusicPlay
            assert response.directive.name == directives.names.AudioPlayDirective


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
class TestPalmMusicLikeRequests(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-434
    """

    owners = ('klim-roma', 'abc:alice_scenarios_music',)

    @pytest.mark.xfail(reason="https://st.yandex-team.ru/HOLLYWOOD-1030")
    def test_alice_434(self, alice):
        liked_tracks_count = 3
        liked_tracks_sequence = []

        alice.plug_tv_in()

        response = alice('включи альбом deuce - invincible')
        assert response.scenario == scenario.HollywoodMusic
        _assert_has_audio_play_directive(response, intent=intent.MusicPlay)

        for _ in range(liked_tracks_count):
            alice('мне не нравится')

        try:
            response = alice('включи альбом deuce - invincible')
            assert response.scenario == scenario.HollywoodMusic
            _assert_has_audio_play_directive(response, intent=intent.MusicPlay)
            current_track = _ResponseAudioTrack(response)

            response = alice('поставь лайк')
            _assert_player_successful_like(response)
            liked_tracks_sequence.append(current_track)

            alice.skip(seconds=5)

            response = alice('следующий трек')
            _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)
            current_track = _ResponseAudioTrack(response)

            response = alice('мне нравится эта песня')
            _assert_player_successful_like(response)
            liked_tracks_sequence.append(current_track)

            alice.skip(seconds=5)

            response = alice('следующий трек')
            _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)
            paused_track = _ResponseAudioTrack(response)

            response = alice('пауза')
            _assert_has_player_pause_directive(response)

            response = alice('поставь лайк')
            _assert_player_successful_like(response)
            liked_tracks_sequence.append(paused_track)

            alice.skip(seconds=5)

            response = alice('включи scorpions - raised on rock')
            assert response.scenario == scenario.HollywoodMusic
            _assert_has_audio_play_directive(response, intent=intent.MusicPlay)

            response = alice('пауза')
            _assert_has_player_pause_directive(response)

            alice.plug_tv_out()
            alice.skip(seconds=90)

            response = alice('поставь лайк')
            assert response.scenario == scenario.HollywoodMusic
            assert not response.intent
            assert response.text in PLAYER_LIKE_BUT_UNKNOWN_MUSIC_RESPONSE_TEXTS

            alice.skip(seconds=5)

            response = alice('включи мои любимые песни')
            assert response.scenario == scenario.HollywoodMusic
            _assert_has_audio_play_directive(response, intent=intent.MusicPlay)
            for track in reversed(liked_tracks_sequence):
                audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
                assert audio_play_directive.name == directives.names.AudioPlayDirective
                assert audio_play_directive.payload.stream.id == track.stream_id
                response = alice('следующий трек')
                _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)
        finally:
            for track in liked_tracks_sequence:
                response = alice(f'включи {track.get_track_string_for_request()}')
                audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
                if audio_play_directive is not None and audio_play_directive.payload.stream.id == track.stream_id:
                    alice('мне не нравится')


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.voice
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station,
])
class TestPalmMusicDislikeRequests(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-435
    """

    owners = ('klim-roma', 'abc:alice_scenarios_music',)

    @pytest.mark.parametrize('dislike_command', [
        'дурацкая песня',
        'дебильная песня',
        'хреновая песня',
        'песня ужас',
    ])
    def test_alice_435(self, alice, dislike_command):
        try:
            response = alice('поставь мою музыку')
            assert response.scenario == scenario.HollywoodMusic
            _assert_has_audio_play_directive(response, intent=intent.MusicPlay)
            disliked_track_from_my_music = _ResponseAudioTrack(response)

            alice.skip(seconds=5)

            response = alice('больше не включай')
            _assert_player_successful_dislike(response)

            response = alice('поставь мою музыку')
            _assert_has_audio_play_directive(response, intent=intent.MusicPlay)
            audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
            assert audio_play_directive.payload.stream.id != disliked_track_from_my_music.stream_id

            alice.skip(seconds=5)

            response = alice('включи трэп')
            assert response.scenario == scenario.HollywoodMusic
            _assert_has_audio_play_directive(response, intent=intent.MusicPlay)

            alice.skip(seconds=5)

            response = alice('мне не нравится эта песня')
            _assert_player_successful_dislike(response)

            alice.skip(seconds=5)

            response = alice(dislike_command)
            _assert_player_successful_dislike(response)

            response = alice('пауза')
            _assert_has_player_pause_directive(response)

            alice.skip(seconds=5)

            response = alice('дизлайк')
            _assert_player_successful_dislike(response, should_change_to_next=False)

            alice.skip(seconds=5)

            response = alice('включи песню синий трактор - едет трактор')
            assert response.scenario == scenario.HollywoodMusic
            _assert_has_audio_play_directive(response, intent=intent.MusicPlay)
            disliked_track = _ResponseAudioTrack(response)

            alice.skip(seconds=5)

            response = alice('песня отстой')

            # TODO: Uncomment this and delete further checks after https://st.yandex-team.ru/HOLLYWOOD-691 is implemented
            # _assert_has_player_pause_directive(response)

            _assert_player_successful_dislike(response, should_change_to_next=False)
            directive = response.get_directive(directives.names.MmStackEngineGetNextCallback)
            assert directive.name == directives.names.MmStackEngineGetNextCallback

            response.next()
            _assert_has_audio_play_directive(response)
            audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
            assert audio_play_directive.payload.stream.id != disliked_track.stream_id
        finally:
            response = alice(f'включи {disliked_track_from_my_music.get_track_string_for_request()}')
            alice('поставь лайк')


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station(is_tv_plugged_in=False),
])
class TestPalmMusicWhatIsPlaying(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1169
    """

    owners = ('klim-roma', 'abc:alice_scenarios_music',)

    def test_alice_1169(self, alice):
        track_author = 'jefferson airplane'
        track_name = 'somebody to love'
        response = alice(f'включи {track_author} - {track_name}')
        _assert_has_audio_play_directive(response, intent=intent.MusicPlay)

        expected_track_response = r'(Это|Сейчас играет) (.+), песня "(.+)"'

        response = alice('какая это песня?')
        match = re.match(expected_track_response, response.text)
        assert match
        assert match.group(2).lower() == track_author and match.group(3).lower() == track_name

        response = alice('следующий трек')
        _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)

        response = alice('что сейчас играет?')
        match = re.match(expected_track_response, response.text)
        assert match, f'No match in response "{response.text}"'
        assert match.group(2).lower() != track_author or match.group(3).lower() != track_name


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.station,
    surface.station_pro,
])
@pytest.mark.device_state(is_tv_plugged_in=True)
class TestPalmGoBackwardFromMusicPlayer(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-345
    '''

    owners = ('klim-roma', 'abc:alice_scenarios_music',)

    def test_alice_345(self, alice):
        response = alice('включи музыку')
        _assert_audio_play_directive(response)
        assert alice.device_state.Video.CurrentScreen == 'music_player'

        response = alice('назад')
        assert response.get_directive(directives.names.GoBackwardDirective)


@pytest.mark.oauth(auth.RobotMultiroom)
@pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
class TestUGCTracks(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-1720
    '''

    owners = ('klim-roma', 'abc:alice_scenarios_music',)

    def test_alice_1720(self, alice):
        response = alice('включи плейлист ugc')
        _assert_audio_play_directive(response)

        expected_track_response = r'(Это|Сейчас играет) песня "(.+)"'

        response = alice('что играет?')
        match = re.match(expected_track_response, response.text)
        assert match, f'No match in response "{response.text}"'
        assert match.group(2).lower() == 'Первый трек.mp3'.lower()

        response = alice('дальше')
        _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)

        response = alice('что играет?')
        match = re.match(expected_track_response, response.text)
        assert match, f'No match in response "{response.text}"'
        assert match.group(2).lower() == 'Второй трек.mp3'.lower()

        response = alice('дальше')
        _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
class TestPalmMusicPlayerRepeat(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1604
    """

    owners = ('klim-roma', 'abc:alice_scenarios_music',)

    def test_alice_1604_check_non_repeating_behaviour(self, alice):
        response = alice('включи песню napalm death you suffer')
        _assert_has_audio_play_directive(response, intent=intent.MusicPlay)

        audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
        non_repeating_track_stream_id = audio_play_directive.payload.stream.id

        alice.skip(milliseconds=alice.device_state.AudioPlayer.DurationMs + 300)

        audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
        assert audio_play_directive.payload.stream.id != non_repeating_track_stream_id

    def test_alice_1604(self, alice):
        response = alice('включи на повторе песню napalm death you suffer')
        _assert_has_audio_play_directive(response, intent=intent.MusicPlay)

        audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
        repeating_track_stream_id = audio_play_directive.payload.stream.id

        for _ in range(10):
            alice.skip(milliseconds=alice.device_state.AudioPlayer.DurationMs)
            _assert_has_audio_play_directive(response)
            audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
            assert audio_play_directive.payload.stream.id == repeating_track_stream_id

        response = alice('включи на репите calvin harris - vault character')
        _assert_has_audio_play_directive(response, intent=intent.MusicPlay)

        audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
        repeating_track_stream_id = audio_play_directive.payload.stream.id

        alice.skip(milliseconds=alice.device_state.AudioPlayer.DurationMs)

        _assert_has_audio_play_directive(response)
        audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
        assert audio_play_directive.payload.stream.id == repeating_track_stream_id


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
class TestPalmMusicShotsNextPrev(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2383
    """

    owners = ('klim-roma', 'abc:alice_scenarios_music',)

    def test_alice_2383(self, alice):
        response = alice('включи плейлист с алисой')
        _assert_has_audio_play_directive(response, intent=intent.MusicPlay)

        is_next_shot = not _is_shot_playing(response)
        next_track_requests_count = 3
        if not is_next_shot:
            next_track_requests_count += 1

        for _ in range(next_track_requests_count):
            response = alice('следующая')
            _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)

            assert is_next_shot == _is_shot_playing(response), 'Tracks and shots are expected to alternate'
            is_next_shot = not is_next_shot

        response = alice('предыдущая')
        _assert_has_audio_play_directive(response, intent=intent.PlayPreviousTrack)
        assert not _is_shot_playing(response), '"Previous track" command should cause a track (not shot) to start playing if it is a shot playing right now'

        response = alice('предыдущая')
        _assert_has_audio_play_directive(response, intent=intent.PlayPreviousTrack)
        assert not _is_shot_playing(response), '"Previous track" command should cause a track (not shot) to start playing even if it is another track playing right now'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
class TestPalmMusicShotsAutoChanging(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2384
    """

    owners = ('klim-roma', 'abc:alice_scenarios_music',)

    def test_alice_2384(self, alice):
        response = alice('включи плейлист с алисой')
        _assert_has_audio_play_directive(response, intent=intent.MusicPlay)

        is_next_shot = not _is_shot_playing(response)

        for _ in range(2):
            alice.skip(milliseconds=alice.device_state.AudioPlayer.DurationMs)
            _assert_has_audio_play_directive(response)

            assert is_next_shot == _is_shot_playing(response), 'Tracks and shots are expected to alternate'
            is_next_shot = not is_next_shot


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
])
class TestPalmMusicShotsDislikes(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2386
    """

    owners = ('klim-roma', 'abc:alice_scenarios_music',)

    @pytest.mark.skip(reason='https://st.yandex-team.ru/HOLLYWOOD-813')
    def test_alice_2386_dislike_track(self, alice):
        response = alice('включи плейлист с алисой')
        _assert_has_audio_play_directive(response, intent=intent.MusicPlay)

        if _is_shot_playing(response):
            response = alice('следующая')
            _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)
            assert not _is_shot_playing(response), 'Tracks and shots are expected to alternate'

        audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
        disliked_track_stream_id = audio_play_directive.payload.stream.id

        response = alice('дизлайк')
        _assert_player_successful_dislike(response)

        response = alice('включи плейлист с алисой')
        _assert_has_audio_play_directive(response, intent=intent.MusicPlay)

        if _is_shot_playing(response):
            response = alice('следующая')
            _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)
            assert not _is_shot_playing(response), 'Tracks and shots are expected to alternate'

        audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
        assert audio_play_directive.payload.stream.id != disliked_track_stream_id, 'Disliked track is expected to be removed from shots playlist'

    def test_alice_2386_dislike_shot(self, alice):
        response = alice('включи плейлист с алисой')
        _assert_has_audio_play_directive(response, intent=intent.MusicPlay)
        is_first_shot = _is_shot_playing(response)

        # skip one track to ensure that we will work with the second shot in playlist
        response = alice('следующая')
        _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)
        assert is_first_shot != _is_shot_playing(response), 'Tracks and shots are expected to alternate'

        if not _is_shot_playing(response):
            response = alice('следующая')
            _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)
            assert _is_shot_playing(response), 'Tracks and shots are expected to alternate'

        audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
        disliked_shot_stream_id = audio_play_directive.payload.stream.id

        response = alice('дизлайк')
        _assert_player_successful_dislike(response)

        audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
        next_track_stream_id = audio_play_directive.payload.stream.id

        response = alice('включи плейлист с алисой')
        _assert_has_audio_play_directive(response, intent=intent.MusicPlay)
        is_first_shot = _is_shot_playing(response)

        # skip one track to ensure that we will work with the second shot in playlist
        response = alice('следующая')
        _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)
        assert is_first_shot != _is_shot_playing(response), 'Tracks and shots are expected to alternate'

        if not _is_shot_playing(response):
            response = alice('следующая')
            _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)
            assert _is_shot_playing(response), 'Tracks and shots are expected to alternate'

        audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
        assert audio_play_directive.payload.stream.id == disliked_shot_stream_id, 'Disliked shot is expected to be kept in shots playlist'

        response = alice('следующая')
        _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)
        assert not _is_shot_playing(response), 'Tracks and shots are expected to alternate'

        audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
        assert audio_play_directive.payload.stream.id == next_track_stream_id, 'Track after disliked shot is expected to be kept in shots playlist'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
class TestPalmMusicShotsWhatIsPlaying(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2387
    """

    owners = ('klim-roma', 'abc:alice_scenarios_music',)

    def test_alice_2387(self, alice):
        response = alice('включи плейлист с алисой')
        _assert_has_audio_play_directive(response, intent=intent.MusicPlay)

        if not _is_shot_playing(response):
            response = alice('следующая')
            _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)
            assert _is_shot_playing(response), 'Tracks and shots are expected to alternate'

        response = alice('что сейчас играет?')
        assert response.text in PLAYER_SHOT_WHAT_IS_PLAYING_RESPONSE_TEXTS

        response = alice('следующая')
        _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)
        assert not _is_shot_playing(response), 'Tracks and shots are expected to alternate'

        expected_track_response = r'(Это|Сейчас играет) (.+), (песня|композиция) "(.+)"'
        response = alice('что сейчас играет?')
        assert re.match(expected_track_response, response.text)


@pytest.mark.oauth(auth.YandexPlus)
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
class TestPalmMusicShotsOtherSurfaces(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2388
    """

    owners = ('klim-roma', 'abc:alice_scenarios_music',)

    @pytest.mark.voice
    @pytest.mark.parametrize('surface', [
        surface.searchapp,
    ])
    def test_alice_2388_searchapp(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.output_speech_text == 'Я бы с радостью, но умею такое только в приложении Яндекс Музыка, или в умных колонках.'
        assert response.directives[0].name == directives.names.OpenUriDirective
        assert len(response.cards) == 1
        assert response.div_card
        assert response.cards[0].type == 'div2_card'

    @pytest.mark.parametrize('surface', [
        surface.navi,
    ])
    def test_alice_2388_navi(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.text == 'Я бы с радостью, но умею такое только в приложении Яндекс.Музыка, или в умных колонках.'
        assert response.directives[0].name == directives.names.OpenUriDirective


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
])
class TestPalmMusicShotsLikes(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2385
    """

    owners = ('klim-roma', 'abc:alice_scenarios_music',)

    def test_alice_2385(self, alice):
        response = alice('включи плейлист с алисой')
        _assert_has_audio_play_directive(response, intent=intent.MusicPlay)

        if _is_shot_playing(response):
            response = alice('следующая')
            _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)
            assert not _is_shot_playing(response), 'Tracks and shots are expected to alternate'

        response = alice('лайк')
        _assert_player_successful_like(response)

        response = alice('следующая')
        _assert_has_audio_play_directive(response, intent=intent.PlayNextTrack)
        assert _is_shot_playing(response), 'Tracks and shots are expected to alternate'

        response = alice('лайк')
        _assert_player_successful_like(response)
