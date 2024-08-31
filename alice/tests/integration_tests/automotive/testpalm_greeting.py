import re

import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


def _assert_text(expected_text, text):
    assert re.search(expected_text, text), f'No match "{expected_text}" in response "{text}"'


class TestPalmGreeting(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1385
    Частично https://testpalm.yandex-team.ru/testcase/alice-1802
    """

    owners = ('kupriyanov-m', 'g:maps-auto-crossplatform')
    greet_phrases = (
        'Привет',
        'Здравствуйте',
        'Приветствую',
        'Рада вас видеть',
        'Здорово, что вы здесь',
        'Вот вы и в Драйве',
        'Устраивайтесь поудобнее',
        'Наконец-то вы здесь',
        'С возвращением',
        'Ура, вы снова с нами',
    )

    @pytest.mark.parametrize('surface', [
        surface.automotive,
        surface.old_automotive,
        surface.uma,
    ])
    @pytest.mark.parametrize('name', [
        None, 'Пользователь', 'Ваше величество',
    ])
    def test_alice_1385(self, alice, name):
        response = alice.greet(name)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.Greeting
        assert response.text.startswith(tuple(_ + (f', {name}' if name else '') for _ in self.greet_phrases))
        _assert_text(r'(Сейчас|За бортом|За окном|О погоде).*', response.text)
        _assert_text(r'(На дорогах|В пробках|Пробки|Оцениваем дороги на).*', response.text)

    @pytest.mark.parametrize('surface', [surface.old_automotive])
    def test_alice_1802(self, alice):
        response = alice('Привет')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.Greeting
        assert response.text.startswith(self.greet_phrases)
        _assert_text(r'(Сейчас|За бортом|За окном|О погоде).*', response.text)
        _assert_text(r'(На дорогах|В пробках|Пробки|Оцениваем дороги на).*', response.text)
