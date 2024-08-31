import re

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [
    surface.automotive,
    surface.launcher,
    surface.navi,
    surface.searchapp,
    surface.yabro_win
])
class TestPalmSingSong(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1090
    https://testpalm.yandex-team.ru/testcase/alice-1524
    https://testpalm.yandex-team.ru/testcase/alice-1560
    """

    owners = ('zhigan', )
    common_expected_suggests = {
        'Еще песню!',
        'Слушать на Яндекс.Музыке!',
    }
    opus_regexp = re.compile(r'<speaker audio="sing_song_\d+.opus">')

    def _assert_response(self, alice, response, expected_intent, command=None):
        assert response.intent == expected_intent
        assert response.text in [
            'Сами напросились!',
            'Танцуют все!',
            'Сейчас спою!',
            'Танцуйте!',
            'Поехали!',
            'Марш нейронных штурмовиков!',
            'Вот нетленка, например.',
            'Вот кое-что из раннего творчества.',
            'Вот из моего последнего альбома. И единственного.',
            'Стихи автопоэта, музыка нейронная. Исполняет Алиса.',
            'У меня альбом тут вышел, вот например из свежего что есть.',
        ]
        assert response.has_voice_response()
        assert re.search(self.opus_regexp, response.output_speech_text)

        response_suggests = {suggest.title for suggest in response.suggests}
        expected_suggests = self.common_expected_suggests.copy()
        if surface.is_navi(alice):
            expected_suggests.add('Что ты умеешь?')
        elif surface.is_launcher(alice) or surface.is_searchapp(alice) or surface.is_yabro_win(alice):
            expected_suggests |= {'Что ты умеешь?', '👍', '👎'}
            if command is not None:
                expected_suggests.add(f'🔍 "{command.lower()}"')

        assert response_suggests == expected_suggests

    def test_sing_song(self, alice):
        command = 'Спой песню'
        response = alice(command)
        self._assert_response(alice, response, intent.MusicSingSong, command)
        first_song = re.search(self.opus_regexp, response.output_speech_text)[0]

        command = 'Еще песню'
        response = alice(command)
        self._assert_response(alice, response, intent.MusicSingSongNext, command)
        other_song = re.search(self.opus_regexp, response.output_speech_text)[0]
        assert other_song != first_song

        if not surface.is_auto(alice):
            listen_on_yandex_music = response.suggest('Слушать на Яндекс.Музыке!')
            assert listen_on_yandex_music
            response = alice.click(listen_on_yandex_music)

            if surface.is_navi(alice):
                assert response.text == 'Включаю: Алиса, альбом "YANY".'
            else:
                assert response.text == 'Включаю альбом'
            assert response.intent == intent.MusicPlay
            assert response.directive.name == directives.names.OpenUriDirective
            assert '4924870' in response.directive.payload.uri

        command = 'Зачитай рэп'
        response = alice(command)
        self._assert_response(alice, response, intent.MusicSingSong, command)

        command = 'Спой что нибудь'
        response = alice(command)
        self._assert_response(alice, response, intent.MusicSingSong, command)
