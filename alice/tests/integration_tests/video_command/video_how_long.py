import alice.tests.library.auth as auth
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
class TestVideoHowLong(object):

    owners = ('amullanurov',)

    def test_serie_how_long(self, alice):
        response = alice('включи рик и морти 3 сезон 3 серия')
        assert response.scenario == scenario.Video

        response = alice('сколько осталось до конца серии')
        assert response.scenario == scenario.VideoCommand
        assert not response.directive
        assert response.text == 'До финальных титров осталось 22 минуты.'

        alice.skip(minutes=1)
        response = alice('сколько осталось до конца серии')
        assert response.scenario == scenario.VideoCommand
        assert not response.directive
        assert response.text == 'До финальных титров осталась 21 минута.'

        alice.skip(minutes=20)
        response = alice('сколько осталось до конца серии')
        assert response.scenario == scenario.VideoCommand
        assert not response.directive
        assert response.text == 'До финальных титров осталась 1 минута.'

    def test_serie_how_long_less_than_minute(self, alice):
        response = alice('включи рик и морти 3 сезон 3 серия')
        assert response.scenario == scenario.Video

        alice.skip(minutes=21, seconds=40)
        response = alice('сколько осталось до конца серии')
        assert response.scenario == scenario.VideoCommand
        assert not response.directive
        assert response.text == 'До финальных титров осталось меньше минуты.'

    def test_cartoon_how_long(self, alice):
        response = alice('включи мультфильм зверополис')
        assert response.scenario == scenario.Video

        response = alice('сколько осталось до конца мультфильма')
        assert response.scenario == scenario.VideoCommand
        assert not response.directive
        assert response.text == 'До финальных титров остался 1 час 35 минут.'

    def test_movie_how_long(self, alice):
        response = alice('включи лалалэнд')
        assert response.scenario == scenario.Video

        response = alice('сколько осталось до конца фильма')
        assert response.scenario == scenario.VideoCommand
        assert not response.directive
        assert response.text == 'До финальных титров остался 1 час 54 минуты.'

        alice.skip(minutes=3)
        response = alice('сколько осталось до конца фильма')
        assert response.scenario == scenario.VideoCommand
        assert not response.directive
        assert response.text == 'До финальных титров остался 1 час 51 минута.'


@pytest.mark.version(videobass=85)
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.legatus])
class TestVideoHowLongLegatus(object):

    owners = ('amullanurov',)

    def test_video_how_long(self, alice):
        response = alice('сколько осталось до конца фильма')
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
