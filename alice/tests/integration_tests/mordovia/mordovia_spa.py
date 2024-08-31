import json

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.surface as surface
import pytest


main_view_key = 'VideoStationSPA:main'
video_view_key = 'VideoStationSPA:video'


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('tv_channels_webview')
class TestVideoMordoviaSPA(object):

    owners = ('akormushkin',)

    def _check_mordovia_command(self, response, view_key, need_clear_history, target_path=None):
        assert response.directive.name == directives.names.MordoviaCommandDirective
        assert response.directive.payload.command == 'change_path'
        if target_path:
            assert json.loads(response.directive.payload.meta)['path'].startswith(target_path)
        assert response.directive.payload.view_key == view_key
        assert json.loads(response.directive.payload.meta)['clear_history'] == need_clear_history

    def _check_mordovia_show(self, response, view_key, need_clear_history, target_path=None):
        assert response.directive.name == directives.names.MordoviaShowDirective
        if target_path:
            assert target_path in response.directive.payload.url
        assert response.directive.payload.view_key == view_key
        assert response.directive.payload.go_back == need_clear_history

    @pytest.mark.parametrize('command', [
        'Найди видео с котиками',
        'Найди фильмы с Джеки Чаном',
        'Фильм ла ла лэнд',
        'Список сезонов Игры Престолов',
        'Что по тв',
    ])
    def test_main_view_key(self, alice, command):  # остаёмся во view_key Главной
        response = alice('домой')  # открыли Главную
        self._check_mordovia_show(response=response, target_path='/video/quasar/home/', view_key=main_view_key, need_clear_history=True)

        response = alice(command)  # открыли другой вебвью-экран в рамках того же вебвью
        self._check_mordovia_command(response=response, view_key=main_view_key, need_clear_history=False)

        response = alice('домой')  # возврат на Главную
        self._check_mordovia_command(response=response, view_key=main_view_key, need_clear_history=True)

    def test_long_chain(self, alice):  # остаёмся во view_key Главной, длинная цепочка запросов
        response = alice('домой')
        self._check_mordovia_show(response=response, target_path='/video/quasar/home/', view_key=main_view_key, need_clear_history=True)

        response = alice('Описание номера 2')
        self._check_mordovia_command(response=response, view_key=main_view_key, need_clear_history=False)

        response = alice('Покажи подробное описание')
        self._check_mordovia_command(response=response, view_key=main_view_key, need_clear_history=False)

        response = alice('Найди сериалы про врачей')
        self._check_mordovia_command(response=response, view_key=main_view_key, need_clear_history=False)

        response = alice('Третий')
        self._check_mordovia_command(response=response, view_key=main_view_key, need_clear_history=False)

        response = alice('Список сезонов')
        self._check_mordovia_command(response=response, view_key=main_view_key, need_clear_history=False)

        response = alice('Найди видео с котиками')
        self._check_mordovia_command(response=response, view_key=main_view_key, need_clear_history=False)

        response = alice('Открой яндекс эфир')
        self._check_mordovia_command(response=response, view_key=main_view_key, need_clear_history=False)

        response = alice('Домой')
        self._check_mordovia_command(response=response, target_path='/video/quasar/home/', view_key=main_view_key, need_clear_history=True)

    @pytest.mark.oauth(auth.YandexPlus)
    def test_two_view_keys(self, alice):
        # Добавляем в последовательность переходов нативные экраны
        # В стеке должен быть только один фрагмент с view_key == 'VideoStationSPA:main' (main_view_key),
        # поэтому после появления нативного экрана поднимаем новый webview-фрагмент командой mordovia_show
        response = alice('домой')
        self._check_mordovia_show(response=response, target_path='/video/quasar/home/', view_key=main_view_key, need_clear_history=True)

        # переход в рамках вебвью-экрана, mordovia_command
        response = alice('Описание номера 2')
        self._check_mordovia_command(response=response, view_key=main_view_key, need_clear_history=False)

        # муз-плеер: нативный экран
        response = alice('Включи музыку')
        assert response.directive.name == directives.names.MusicPlayDirective

        # поднимаем новый вебвью, в стеке экранов будут:
        # (Главная -> описание фильма) -> муз. плеер -> список каналов
        response = alice('Открой яндекс эфир')
        self._check_mordovia_show(response=response, view_key=video_view_key, need_clear_history=False)

        # переход в рамках вебвью-экрана, mordovia_command
        response = alice('Найди фильмы про животных')
        self._check_mordovia_command(response=response, view_key=video_view_key, need_clear_history=False)

        # видео-плеер: нативный экран
        response = alice('Включи первый')
        assert response.directive.name == directives.names.VideoPlayDirective

        # поднимаем новый вебвью, в стеке экранов будут:
        # (Главная -> описание фильма) -> муз. плеер -> (список каналов -> фильмы про животных) -> видео-плеер -> сезоны Игры Престолов
        response = alice('Список серий Игры престолов')
        self._check_mordovia_show(response=response, view_key=video_view_key, need_clear_history=False)

        # переход в рамках вебвью-экрана, mordovia_command
        response = alice('Второй сезон')
        self._check_mordovia_command(response=response, view_key=video_view_key, need_clear_history=False)

        # переход в рамках вебвью-экрана, mordovia_command
        response = alice('Найди видео с котиками')
        self._check_mordovia_command(response=response, view_key=video_view_key, need_clear_history=False)

        # В стеке экранов на данный момент:
        # (Главная -> описание фильма) -> муз. плеер -> (список каналов -> фильмы про животных) -> видео-плеер ->
        # -> (сезоны Игры Престолов -> 2 сезон -> видео с котиками)
        # Выбрасываем из стека все фрагменты, кроме Главной, а Главную обновляем (директива mordovia_show с go_back=True)
        response = alice('Домой')
        self._check_mordovia_show(response=response, target_path='/video/quasar/home/', view_key=main_view_key, need_clear_history=True)
