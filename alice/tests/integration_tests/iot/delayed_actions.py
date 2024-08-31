import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

from iot.common import is_iot, assert_response_text
from iot.common import no_config_skip   # noqa: F401


@pytest.mark.oauth(auth.Smarthome)
@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestIotDelayedActions(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2731
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'Выключи свет через 1 секунду',
        'Включи лампочку через 5 минут',
        'Включи кондиционер через два часа',
        'Включи кондиционер через три часа 15 минут',
        'Включи кондиционер через полтора часа',
    ])
    def test_alice_2731_1(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert not response.suggests
        assert response.text.startswith('Хорошо, запомнила: сделаю')

    def test_cancel_just_created_action(self, alice):
        response = alice('Выключи свет через 1 секунду')
        assert is_iot(response) is True
        assert not response.suggests
        assert response.text.startswith('Хорошо, запомнила: сделаю')

        response = alice('Отмена')
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            [
                'Передумали? Ладно. Отменила этот план!'
            ]
        )

    def test_time_specify(self, alice):
        response = alice('Выключи свет завтра')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            [
                'Окей. А в какое время?',
                'Ладно. Но вы должны назвать точное время.',
                'А вдруг я сделаю это не вовремя? Укажите точное время.',
            ]
        )

        response = alice('В 12:00')
        assert is_iot(response) is True
        assert response.text.startswith('Хорошо, запомнила: сделаю')
