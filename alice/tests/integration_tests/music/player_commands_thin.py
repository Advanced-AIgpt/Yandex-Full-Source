import re

import alice.megamind.protos.scenarios.directives_pb2 as directives_pb2
import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


ANNOUNCE_PREFIX = '|'.join([
    r'А сейчас',
    r'А дальше нас ждет',
    r'А теперь поставлю',
    r'Послушайте',
    r'Дальше послушаем',
    r'Для вас -',
    r'Следующий трек -',
    r'Следующим номером нашей программы',
    r'Дальше -',
    r'А теперь',
])
ANNOUNCE_ALL = fr'({ANNOUNCE_PREFIX} )?.*(П|п)есня .*'
ANNOUNCE_ALBUM = fr'({ANNOUNCE_PREFIX} )?.*(А|а)льбом .*, песня .*'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('music_force_show_first_track')
@pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
class TestMusicStartStop(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-3146
    '''

    owners = ('vitvlkv',)

    @pytest.mark.parametrize('stop, start', [
        ('остановить', 'продолжить'),
        ('выключить', 'продолжай играть'),
        ('стоп', 'играй'),
        ('нажми стоп', 'продолжи играть'),
        ('пауза', 'продолжай'),
        ('поставь на паузу', 'запускай'),
        ('на паузу', 'снова запусти'),
        ('нажми паузу', 'запустить'),
        ('выруби', 'возобнови'),
        ('выключи', 'вруби'),
        pytest.param(
            'замолчи', 'верни музыку',
            marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-13930'),
        ),
    ])
    def test_start_stop_player(self, alice, stop, start):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Включаю.'

        response = alice(stop)
        assert response.intent == intent.PlayerPause
        assert response.directives[-1].name == directives.names.ClearQueueDirective
        assert not response.text

        response = alice(start)
        assert response.intent == intent.PlayerContinue
        audio_play = response.get_directive(directives.names.AudioPlayDirective)
        assert audio_play.name == directives.names.AudioPlayDirective
        assert not response.text


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('music_force_show_first_track')
@pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
class TestMusicTrackCommand(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-387
    https://testpalm.yandex-team.ru/testcase/alice-1436
    '''

    owners = ('nkodosov',)

    @pytest.mark.parametrize('command', [
        'Вперед',
        'Дальше',
        'Следующий трек',
        'Давай следующую',
        'Перемотай вперед',
        'Перемотай, пожалуйста',
        'Пропустить',
        'Скип',
        'Хочу что-нибудь другое',
    ])
    def test_next(self, alice, command):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Включаю.'

        response = alice(command)
        assert response.intent == intent.PlayNextTrack
        audio_play = response.get_directive(directives.names.AudioPlayDirective)
        assert audio_play.name == directives.names.AudioPlayDirective
        assert not response.text

    @pytest.mark.parametrize('command', [
        'Предыдущий трек',
        'Перемотай назад',
    ])
    def test_previous(self, alice, command):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Включаю.'

        response = alice('Следующий трек')
        assert response.intent == intent.PlayNextTrack

        response = alice(command)
        assert response.intent == intent.PlayPreviousTrack
        audio_play = response.get_directive(directives.names.AudioPlayDirective)
        assert audio_play.name == directives.names.AudioPlayDirective
        assert not response.text

    @pytest.mark.parametrize('command', [
        'Предыдущий трек',
        'Перемотай назад',
    ])
    def test_not_previous(self, alice, command):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Включаю.'

        response = alice(command)
        assert not response.intent
        assert not response.directive
        assert response.text in [
            'Простите, я отвлеклась и не запомнила, что играло до этого.',
            'Простите, я совершенно забыла, что включала до этого.',
            'Извините, я не запомнила, какой трек был предыдущим.',
            'Извините, но я выходила во время предыдущего трека и не знаю, что играло.',
        ]

    @pytest.mark.parametrize('command', [
        'Включи сначала',
        'Перемотай в начало',
        'Начни сначала',
    ])
    def test_replay(self, alice, command):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Включаю.'

        response = alice(command)
        assert response.intent == intent.PlayerReplay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert not response.text


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('music_force_show_first_track')
@pytest.mark.device_state(is_tv_plugged_in=False)
@pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
class TestMusicCommandsWithoutScreen(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-3148
    '''

    owners = ('nkodosov',)

    def test(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.text == 'Включаю.'

        response = alice('выключи музыку')
        assert response.intent == intent.PlayerPause
        assert response.directives[-1].name == directives.names.ClearQueueDirective
        assert not response.text

        response = alice('продолжить слушать')
        assert response.intent == intent.PlayerContinue
        audio_play = response.get_directive(directives.names.AudioPlayDirective)
        assert audio_play.name == directives.names.AudioPlayDirective
        assert not response.text

        response = alice('стоп')
        assert response.intent == intent.PlayerPause
        assert response.directives[-1].name == directives.names.ClearQueueDirective
        assert not response.text

        response = alice('играй')
        assert response.intent == intent.PlayerContinue
        audio_play = response.get_directive(directives.names.AudioPlayDirective)
        assert audio_play.name == directives.names.AudioPlayDirective
        assert not response.text

        response = alice('вперед')
        assert response.intent == intent.PlayNextTrack
        audio_play = response.get_directive(directives.names.AudioPlayDirective)
        assert audio_play.name == directives.names.AudioPlayDirective
        assert not response.text

        response = alice('назад')
        assert response.intent == intent.PlayPreviousTrack
        audio_play = response.get_directive(directives.names.AudioPlayDirective)
        assert audio_play.name == directives.names.AudioPlayDirective
        assert not response.text


AudioRewindType = directives_pb2.TAudioRewindDirective.EType


def _assert_audio_rewind(response, rewind_type, rewind_amount):
    assert response.intent == intent.PlayerRewind
    assert response.directive.name == directives.names.AudioRewindDirective
    assert response.directive.payload.type == AudioRewindType.Name(rewind_type)
    assert response.directive.payload.amount_ms == rewind_amount*1000


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('music_force_show_first_track')
@pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
class TestMusicRewindCommand(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-3145
    '''

    owners = ('vitvlkv',)

    def test_rewind_command(self, alice):
        response = alice('включи Queen – Bohemian Rhapsody')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.directive.payload.metadata.title == 'Bohemian Rhapsody'
        assert response.directive.payload.metadata.subtitle == 'Queen'
        assert response.directive.payload.stream.id == '1710808'
        assert re.match(r'Включаю: .*Queen.*Bohemian Rhapsody', response.text)

        response = alice('Перемотай на 2 минуты вперед')
        _assert_audio_rewind(response, AudioRewindType.Forward, 120)

        response = alice('Перемотай на минуту назад')
        _assert_audio_rewind(response, AudioRewindType.Backward, 60)

        response = alice('пауза')
        assert response.intent == intent.PlayerPause
        assert response.directives[-1].name == directives.names.ClearQueueDirective
        assert not response.text

        response = alice('Перемотай на 40 секунд назад')
        _assert_audio_rewind(response, AudioRewindType.Backward, 40)

        response = alice('Продолжить')
        assert response.intent == intent.PlayerContinue
        audio_play = response.get_directive(directives.names.AudioPlayDirective)
        assert audio_play.name == directives.names.AudioPlayDirective
        assert audio_play.payload.stream.id == '1710808'
        assert not response.text

        response = alice('Перемотай на 20 секунд')
        _assert_audio_rewind(response, AudioRewindType.Absolute, 20)

        response = alice('Перемотай на 10 минут вперед')
        _assert_audio_rewind(response, AudioRewindType.Forward, 600)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', surface.yandex_smart_speakers)
class TestMusicShuffleCommand(object):

    owners = ('nkodosov',)

    def test_shuffle(self, alice):
        response = alice('Включи танцевальную музыку')
        response = alice('перемешай')
        assert response.intent == intent.PlayerShuffle
        assert not response.directive
        assert response.text in [
            'А я уже.', 'Да тут и так все вперемешку.', 'Ок, еще раз пропустила через блендер.',
        ]


@pytest.mark.experiments('enable_shuffle_in_hw_music')
class TestMusicShuffleCommandExp(TestMusicShuffleCommand):
    pass


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('music_force_show_first_track')
@pytest.mark.parametrize('surface', [surface.loudspeaker, surface.station])
class TestRemotePlayerCommands(object):

    owners = ('sparkle',)

    def test_next_prev_track(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_next_track()
        assert response.intent == intent.PlayNextTrack
        assert response.directive.name == directives.names.AudioPlayDirective
        assert not response.text

        response = alice.remote_player_prev_track()
        assert response.intent == intent.PlayPreviousTrack
        assert response.directive.name == directives.names.AudioPlayDirective
        assert not response.text

    def test_prev_track(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_prev_track()
        assert not response.intent
        assert response.text in [
            'Простите, я отвлеклась и не запомнила, что играло до этого.',
            'Простите, я совершенно забыла, что включала до этого.',
            'Извините, я не запомнила, какой трек был предыдущим.',
            'Извините, но я выходила во время предыдущего трека и не знаю, что играло.',
        ]

    def test_replay(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_replay()
        assert response.intent == intent.PlayerReplay
        assert response.directive.name == directives.names.AudioPlayDirective
        assert not response.text

    def test_pause_and_continue(self, alice):
        response = alice('включи альбом mutter')

        # not yet remote "pause" command
        response = alice('на паузу')  # TODO(sparkle): find out about "pause" remote command
        assert response.intent == intent.PlayerPause
        assert response.directive.name == directives.names.ClearQueueDirective
        assert not response.text

        # remote "continue" command
        response = alice.remote_player_continue()
        assert response.intent == intent.PlayerContinue
        assert response.directive.name == directives.names.AudioPlayDirective
        assert not response.text

    def test_shuffle(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_shuffle()
        assert response.intent == intent.PlayerShuffle
        assert response.directive.name == directives.names.SetGlagolMetadataDirective
        assert response.directive.payload.glagol_metadata.music_metadata.shuffled is True
        assert response.text in [
            'Сделала. После этого трека всё будет вперемешку.',
            'Пожалуйста. После этого трека - всё вперемешку.',
            'Готово. После этого трека включаю полный винегрет.',
        ]

    def test_rewind(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_rewind('{"minutes":2,"seconds":10}')
        _assert_audio_rewind(response, AudioRewindType.Forward, 130)

    def test_repeat(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_repeat()
        assert response.intent == intent.PlayerRepeat
        assert response.directive.name == directives.names.SetGlagolMetadataDirective
        assert response.directive.payload.glagol_metadata.music_metadata.repeat_mode == 'One'
        assert response.text in [
            'Хорошо, буду повторять.',
            'Ок, ставлю на повтор.',
        ]

    def test_what_is_playing(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_what_is_playing()
        assert response.intent == intent.MusicWhatIsPlaying
        assert 'Rammstein' in response.text
        assert not response.directive

    def test_like(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_like()
        assert response.intent == intent.PlayerLike
        assert not response.directive
        assert response.text in [
            'Буду включать такое чаще.',
            'Запомню, что вам такое по душе.',
            'Рада, что вы оценили.',
            'Поставила лайк.',
            'Круто! Ставлю лайк.',
            'Уже поставила лайк.',
            'Поставила лайк за вас.',
        ]

    def test_dislike(self, alice):
        response = alice('включи егора крида')
        track_id = response.directive.payload.stream.id
        response = alice.remote_player_dislike()
        assert response.intent == intent.PlayerDislike
        assert response.directive.name == directives.names.AudioPlayDirective
        assert response.directive.payload.stream.id != track_id
        assert response.text in [
            'Дизлайк принят.',
            'Хорошо, ставлю дизлайк.',
            'Окей, не буду такое ставить.',
            'Поняла. Больше не включу.',
            'Нет проблем, поставила дизлайк.',
        ]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
class TestMixOfMusicAndVideoCommands(object):
    def test_continue_music_after_video_play(self, alice):
        alice('включи музыку')
        alice('стоп')
        alice('включи доктор хаус первый сезон первая серия')
        response = alice('продолжи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.PlayerContinue
        assert response.directive.name == directives.names.AudioPlayDirective


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('hw_music_announce')
@pytest.mark.parametrize('surface', [surface.station])
class TestTrackAnnounce(object):

    owners = ('ardulat', 'abc:alice_scenarios_music',)

    @pytest.mark.parametrize('command', [
        pytest.param('включи музыку', id='radio'),
        pytest.param('включи queen', id='artist'),
        pytest.param('включи песню sia cheap thrills', id='track'),
        pytest.param('включи альбом dark side of the moon', id='album'),
    ])
    def test_track_announce(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('следующий трек')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.PlayNextTrack
        assert response.directive.name == directives.names.AudioPlayDirective
        assert re.match(ANNOUNCE_ALL, response.text)
        assert not re.match(ANNOUNCE_ALBUM, response.text)

    @pytest.mark.experiments('hw_music_announce_album')
    def test_track_announce_album(self, alice):
        response = alice('включи вечные хиты')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('следующий трек')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.PlayNextTrack
        assert response.directive.name == directives.names.AudioPlayDirective
        assert re.match(ANNOUNCE_ALBUM, response.text)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('hw_music_send_song_text')
@pytest.mark.parametrize('surface', [surface.station])
class TestSendSongText(object):

    owners = ('ardulat', 'abc:alice_scenarios_music',)

    @pytest.mark.parametrize('what, answer, is_missing_lyrics', [
        pytest.param('кобзон', 'Отправила ссылку, чтобы посмотреть, откройте приложение Яндекса.', False, id='song'),
        pytest.param('чайковский', 'В этой песне нет текста или его еще нет на Яндекс.Музыке.', True, id='classics'),
        pytest.param('подкаст хрум', 'Этот текст показать не могу, но всегда смогу отправить вам ссылку на текст песни.', True, id='podcast'),
    ])
    def test_send_song_text(self, alice, what, answer, is_missing_lyrics):
        response = alice(f'включи {what}')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('пришли текст песни')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicSendSongText
        assert response.text == answer
        assert not response.directive
        if not is_missing_lyrics:
            assert len(response.voice_response.directives) == 1
            assert 'send_push_directive' == response.voice_response.directives[0].name

    @pytest.mark.version(hollywood=206)
    def test_nothing_playing(self, alice):
        response = alice('пришли текст песни')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicSendSongText
        assert response.text == 'Как только вы включите музыку, я смогу прислать текст.'
        assert not response.directive


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('hw_music_what_album_is_this_song_from')
@pytest.mark.parametrize('surface', [surface.station])
class TestWhatAlbumIsThisSongFrom(object):

    owners = ('ardulat', 'abc:alice_scenarios_music',)

    @pytest.mark.parametrize('what', [
        pytest.param('кобзон', id='song'),
        pytest.param('чайковский', id='classics'),
        pytest.param('подкаст хрум', id='podcast'),
    ])
    def test_what_album_is_this_song_from(self, alice, what):
        response = alice(f'включи {what}')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('с какого альбома эта песня')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicWhatAlbumIsThisSongFrom
        assert response.text.startswith('С альбома ')
        assert not response.directive

    @pytest.mark.version(megamind=250)
    def test_nothing_playing(self, alice):
        response = alice('с какого альбома эта песня')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicWhatAlbumIsThisSongFrom
        assert response.text == 'Как только вы включите музыку, я отвечу.'
        assert not response.directive


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('hw_music_what_year_is_this_song')
@pytest.mark.parametrize('surface', [surface.station])
class TestWhatYearIsThisSong(object):

    owners = ('ardulat', 'abc:alice_scenarios_music',)

    @pytest.mark.parametrize('what, is_missing', [
        pytest.param('кобзон', False, id='song'),
        pytest.param('чайковский', False, id='classics'),
        pytest.param('подкаст хрум', True, id='podcast'),
    ])
    def test_what_album_is_this_song_from(self, alice, what, is_missing):
        response = alice(f'включи {what}')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('какого года песня')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicWhatYearIsThisSong
        if not is_missing:
            assert response.text.startswith('С альбома ')
        else:
            assert response.text == 'Не смогу сказать год этой записи.'
        assert not response.directive

    @pytest.mark.version(megamind=250)
    def test_nothing_playing(self, alice):
        response = alice('какого года песня')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicWhatYearIsThisSong
        assert response.text == 'Как только вы включите музыку, я отвечу.'
        assert not response.directive
