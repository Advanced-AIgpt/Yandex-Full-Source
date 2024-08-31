import alice.tests.library.surface as surface
import pytest


class TestOnlineCinema(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-36
    """

    owners = ('akormushkin',)

    @pytest.mark.xfail(reason='теперь по запросу фильма отвечаем фактом, это норм (см. DIALOG-6982)')
    @pytest.mark.parametrize('surface', [surface.watch])
    def test_mock(self, alice):
        response = alice('Найди фильм терминатор 2')
        assert response.text in [
            'Я не могу на это ответить.',
            'У меня нет ответа на такой запрос.',
            'Простите, я не знаю что ответить.',
            'Извините, у меня нет хорошего ответа.',
            'Я пока не умею отвечать на такие запросы.',
        ]
        assert not response.directive
