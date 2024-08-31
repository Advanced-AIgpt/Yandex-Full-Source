import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [
    surface.automotive,
    surface.navi,
    surface.watch,
])
class TestPalmBiometry(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-21
    """

    owners = ('tolyandex',)

    @pytest.mark.parametrize('command', [
        'Как меня зовут',
        'Запомни моё имя',
    ])
    def test_alice_21(self, alice, command):
        response = alice(command)

        assert response.intent == intent.GeneralConversation
        assert not response.directive
