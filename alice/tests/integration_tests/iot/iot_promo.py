import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

from iot.common import is_iot, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.nlg as nlg


@pytest.mark.oauth(auth.Smarthome)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPromo(object):

    owners = ('norchine', 'abc:alice_iot')

    def test_light(self, alice):
        response = alice('зажги свет')
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Включаю.',
            ]
        )

    def test_brightness(self, alice):
        response = alice('сделай лампу поярче')
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Окей, сделаем поярче.',
                'Окей, делаем ярче.',
                'Окей, добавим яркости.',
                'Добавляю яркости.',
                'Больше яркости. Окей.',
            ]
        )

    def test_moon_color(self, alice):
        response = alice('поменяй цвет на лунный')
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Меняю цвет.',
            ]
        )

    def test_room(self, alice):
        response = alice('выключи свет на кухне')
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Выключаю.',
                'Окей, выключаю.',
                'Окей, выключаем.',
            ]
        )

    @pytest.mark.parametrize('command', [
        'скоро буду дома',
        'да будет свет',
        'я дома',
        'давай смотреть кино',
        'стало душно',
        'детям пора спать',
    ])
    def test_scenarios(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert response.text.startswith((
            'Окей', 'Начнём', 'Активирую программу', 'Запускаю программу', 'Запускаю вашу программу',
        ))
