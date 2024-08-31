import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
class TestShowVideoSettings(object):

    owners = ('amullanurov',)

    def test_show_video_settings_kinopoisk(self, alice):
        alice('включи рик и морти сезон 3 серия 1')
        response = alice('покажи настройки видео')
        assert response.scenario == scenario.VideoCommand
        assert response.directive.name == directives.names.ShowVideoSettingsDirective

    def test_show_video_settings_other(self, alice):
        alice('включи канал звезда')
        response = alice('покажи настройки видео')
        assert response.scenario == scenario.VideoCommand
        assert not response.directive
        assert response.text == 'Ой... кажется, для этого видео у меня пока ничего нет.'


@pytest.mark.version(videobass=85)
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.legatus])
class TestShowVideoSettingsLegatus(object):

    owners = ('amullanurov',)

    def test_show_video_settings(self, alice):
        response = alice('покажи настройки видео')
        assert response.scenario == scenario.VideoCommand
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
