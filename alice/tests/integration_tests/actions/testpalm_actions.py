import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.watch])
class TestPalmActions(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-23
    """

    owners = ('leletko',)

    @pytest.mark.parametrize('command', [
        'Открой сайт авито',
        'Открой сайт газеты ру',
        'Открой приложение инстаграм',
        'Открой контакты',
    ])
    def test_alice_23(self, alice, command):
        response = alice(command)

        assert response.text in [
            'В часах такое провернуть сложновато.',
            'Я бы и рада, но здесь не могу. Эх.',
            'Здесь точно не получится.',
        ]
        assert response.intent == intent.ProhibitionError
        assert not response.directive
