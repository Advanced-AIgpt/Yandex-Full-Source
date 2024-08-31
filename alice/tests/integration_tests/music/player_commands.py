import re

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('music_force_show_first_track')
@pytest.mark.parametrize('surface', [surface.dexp])
class TestMusicStartStop(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-491
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
    ])
    def test_start_stop_player(self, alice, stop, start):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text == 'Включаю.'

        response = alice(stop)
        assert response.intent == intent.PlayerPause
        assert response.directive.name == directives.names.PlayerPauseDirective
        assert not response.text

        response = alice(start)
        assert response.intent == intent.PlayerContinue
        assert response.directive.name == directives.names.PlayerContinueDirective
        assert not response.text

    @pytest.mark.parametrize('stop, start, intent_names', [
        ('замолчи', 'верни музыку', [intent.PlayerPause]),
        ('выключи', 'вруби', [intent.Cancel, intent.PlayerPause]),
    ])
    def test_start_stop_handcrafted(self, alice, stop, start, intent_names):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text == 'Включаю.'

        response = alice(stop)
        assert response.intent in intent_names
        assert response.directive.name == directives.names.PlayerPauseDirective

        response = alice(start)
        assert response.intent in [intent.PlayerContinue, intent.MusicPlay]
        assert response.directive.name in [directives.names.PlayerContinueDirective, directives.names.MusicPlayDirective]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('music_force_show_first_track')
@pytest.mark.parametrize('surface', [surface.dexp])
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
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text == 'Включаю.'

        response = alice(command)
        assert response.intent == intent.PlayNextTrack
        assert response.directive.name == directives.names.PlayerNextTrackDirective
        assert not response.text

    @pytest.mark.parametrize('command', [
        'Предыдущий трек',
        'Перемотай назад',
    ])
    def test_previous(self, alice, command):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text == 'Включаю.'

        response = alice(command)
        assert response.intent == intent.PlayPreviousTrack
        assert response.directive.name == directives.names.PlayerPreviousTrackDirective
        assert not response.text

    @pytest.mark.parametrize('command', [
        'Включи сначала',
        'Перемотай в начало',
        'Начни сначала',
    ])
    def test_replay(self, alice, command):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text == 'Включаю.'

        response = alice(command)
        assert response.intent == intent.PlayerReplay
        assert response.directive.name == directives.names.PlayerReplayDirective
        assert not response.text


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('music_force_show_first_track')
@pytest.mark.parametrize('surface', [surface.dexp])
class TestMusicCommandsWithoutScreen(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-604
    '''

    owners = ('nkodosov',)

    def test(self, alice):
        response = alice('включи музыку')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text == 'Включаю.'

        response = alice('выключи музыку')
        assert response.intent == intent.PlayerPause
        assert response.directive.name == directives.names.PlayerPauseDirective
        assert not response.text

        response = alice('продолжить слушать')
        assert response.intent == intent.PlayerContinue
        assert response.directive.name == directives.names.PlayerContinueDirective
        assert not response.text

        response = alice('стоп')
        assert response.intent == intent.PlayerPause
        assert response.directive.name == directives.names.PlayerPauseDirective
        assert not response.text

        response = alice('играй')
        assert response.intent == intent.PlayerContinue
        assert response.directive.name == directives.names.PlayerContinueDirective
        assert not response.text

        response = alice('вперед')
        assert response.intent == intent.PlayNextTrack
        assert response.directive.name == directives.names.PlayerNextTrackDirective
        assert not response.text

        response = alice('назад')
        assert response.intent == intent.PlayPreviousTrack
        assert response.directive.name == directives.names.PlayerPreviousTrackDirective
        assert not response.text


def _assert_rewind(response, rewind_type, rewind_amount):
    assert response.intent == intent.PlayerRewind
    assert response.directive.name == directives.names.PlayerRewindDirective
    assert response.directive.payload.type == rewind_type
    assert response.directive.payload.amount == rewind_amount


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('music_force_show_first_track')
@pytest.mark.parametrize('surface', [surface.dexp])
class TestMusicRewindCommand(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-490
    '''

    owners = ('vitvlkv',)

    def test_rewind_command(self, alice):
        response = alice('включи Queen – Bohemian Rhapsody')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.directive.payload.first_track_id == '1710808'
        assert re.search(r'^Включаю: .*Queen.*Bohemian Rhapsody', response.text)

        response = alice('Перемотай на 2 минуты вперед')
        _assert_rewind(response, 'forward', 120)

        response = alice('Перемотай на минуту назад')
        _assert_rewind(response, 'backward', 60)

        response = alice('пауза')
        assert response.intent == intent.PlayerPause
        assert response.directive.name == directives.names.PlayerPauseDirective
        assert not response.text

        response = alice('Перемотай на 40 секунд назад')
        _assert_rewind(response, 'backward', 40)

        response = alice('Продолжить')
        assert response.intent == intent.PlayerContinue
        assert response.directive.name == directives.names.PlayerContinueDirective
        assert not response.text

        response = alice('Перемотай на 20 секунд')
        _assert_rewind(response, 'absolute', 20)

        response = alice('Перемотай на 10 минут вперед')
        _assert_rewind(response, 'forward', 600)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.dexp])
class TestMusicShuffleCommand(object):

    owners = ('nkodosov',)

    def test_shuffle(self, alice):
        response = alice('Включи танцевальную музыку')
        response = alice('перемешай')
        assert response.directive.name == directives.names.PlayerShuffleDirective
        assert response.text in [
            'Перемешала все треки.', 'Люблю беспорядок.', 'Ок',
        ]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('music_force_show_first_track')
@pytest.mark.parametrize('surface', [surface.dexp])
class TestRemotePlayerCommands(object):

    owners = ('sparkle',)

    def test_next_track(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_next_track()
        assert response.intent == intent.PlayNextTrack
        assert response.directive.name == directives.names.PlayerNextTrackDirective
        assert not response.text

    def test_prev_track(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_prev_track()
        assert response.intent == intent.PlayPreviousTrack
        assert response.directive.name == directives.names.PlayerPreviousTrackDirective
        assert not response.text

    def test_replay(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_replay()
        assert response.intent == intent.PlayerReplay
        assert response.directive.name == directives.names.PlayerReplayDirective
        assert not response.text

    def test_pause_and_continue(self, alice):
        response = alice('включи альбом mutter')

        # not yet remote "pause" command
        response = alice('на паузу')  # TODO(sparkle): find out about "pause" remote command
        assert response.intent == intent.PlayerPause
        assert response.directive.name == directives.names.PlayerPauseDirective
        assert not response.text

        # remote "continue" command
        response = alice.remote_player_continue()
        assert response.intent == intent.PlayerContinue
        assert response.directive.name == directives.names.PlayerContinueDirective
        assert not response.text

    def test_shuffle(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_shuffle()
        assert response.directive.name == directives.names.PlayerShuffleDirective
        assert response.text in [
            'Перемешала все треки.', 'Люблю беспорядок.', 'Ок',
        ]

    def test_rewind(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_rewind('{"minutes":2,"seconds":10}')
        _assert_rewind(response, 'forward', 130)

    def test_repeat(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_repeat()
        assert response.intent == intent.PlayerRepeat
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text == 'Ставлю на повтор'

    def test_what_is_playing(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_what_is_playing()
        assert response.intent == intent.MusicWhatIsPlaying
        assert all(part in response.text for part in [
            'Fake EVO Artist',
            'Fake EVO Title',
        ])
        assert not response.directive

    def test_like(self, alice):
        response = alice('включи альбом mutter')
        response = alice.remote_player_like()
        assert response.intent == intent.PlayerLike
        assert response.directive.name == directives.names.PlayerLikeDirective

    def test_dislike(self, alice):
        response = alice('включи егора крида')
        response = alice.remote_player_dislike()
        assert response.intent == intent.PlayerDislike
        assert response.directive.name == directives.names.PlayerDislikeDirective
