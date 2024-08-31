import re

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.station])
class TestMusicAlarm(object):

    owners = ('olegator', 'abc:alice_scenarios_alarm',)

    @pytest.mark.oauth(auth.YandexPlus)
    def test_with_plus(self, alice):
        response = alice('поставь на будильник queen')
        assert response.intent == intent.AlarmSetSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert re.search(
            '(Хорошо. Вас разбудит|Отличный выбор. Теперь на вашем будильнике|'
            'Запомнила. Вас разбудит|Отличный выбор. Вас разбудит|Установила. На будильнике) Queen',
            response.text
        )

    @pytest.mark.oauth(auth.Yandex)
    def test_without_plus(self, alice):
        response = alice('поставь на будильник queen')
        assert response.intent == intent.AlarmSetSound
        assert not response.directive
        assert response.text == 'Чтобы ставить на будильник музыку, необходимо купить подписку на Яндекс.Плюс.'

    @pytest.mark.no_oauth
    def test_without_auth(self, alice):
        response = alice('поставь на будильник queen')
        assert response.intent == intent.AlarmSetSound
        assert not response.directive
        assert response.text == 'Вы не авторизовались.'


@pytest.mark.parametrize('surface', [surface.dexp])
class TestMusicThickClientAlarm(object):

    owners = ('olegator', 'vitvlkv', 'abc:alice_scenarios_alarm',)

    @pytest.mark.oauth(auth.YandexPlus)
    def test_with_plus_currently_playing(self, alice):
        response = alice('включи seether fine again')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective

        response = alice('поставь на будильник этот трек')
        assert response.intent == intent.AlarmSetSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        artist = alice.device_state.Music.CurrentlyPlaying.RawTrackInfo['artists'][0]['name']
        album = alice.device_state.Music.CurrentlyPlaying.RawTrackInfo['albums'][0]['title']
        track_title = alice.device_state.Music.CurrentlyPlaying.RawTrackInfo['title']
        assert re.search(
            '(Хорошо. Вас разбудит|Отличный выбор. Теперь на вашем будильнике|'
            f'Запомнила. Вас разбудит|Отличный выбор. Вас разбудит|Установила. На будильнике) {artist}, альбом "{album}", песня "{track_title}"',
            response.text
        )


@pytest.mark.parametrize('surface', [surface.station])
class TestMusicThinClientAlarm(object):

    owners = ('vitvlkv', 'abc:alice_scenarios_alarm',)

    @pytest.mark.oauth(auth.YandexPlus)
    def test_with_plus_currently_playing(self, alice):
        response = alice('включи seether fine again')
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.AudioPlayDirective

        response = alice('поставь на будильник этот трек')
        assert response.intent == intent.AlarmSetSound
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert re.search(
            '(Хорошо. Вас разбудит|Отличный выбор. Теперь на вашем будильнике|'
            'Запомнила. Вас разбудит|Отличный выбор. Вас разбудит|Установила. На будильнике) Seether, песня "Fine Again"',
            response.text
        )


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('alarm_semantic_frame')
@pytest.mark.supported_features('set_alarm_semantic_frame_v2')
@pytest.mark.parametrize('surface', [surface.station])
class TestMusicAlarmSemanticFrame(object):

    owners = ('zhigan', 'abc:alice_scenarios_alarm',)

    def test_set_alarm(self, alice):
        response = alice('поставь на будильник queen')
        assert response.intent == intent.AlarmSetSound
        assert re.search(
            '(Хорошо. Вас разбудит|Отличный выбор. Теперь на вашем будильнике|'
            'Запомнила. Вас разбудит|Отличный выбор. Вас разбудит|Установила. На будильнике) Queen',
            response.text
        )
        assert response.directive.name == directives.names.AlarmSetSoundDirective
        assert 'music_play_semantic_frame' in response.directive.payload.server_action.payload.typed_semantic_frame
