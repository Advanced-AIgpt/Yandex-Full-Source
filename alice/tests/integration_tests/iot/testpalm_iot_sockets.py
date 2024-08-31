import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

from iot.common import is_iot, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.nlg as nlg


@pytest.mark.oauth(auth.Smarthome)
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.navi,
    surface.searchapp,
    surface.station,
    surface.yabro_win,
])
class TestPalmIotSockets(object):
    """
        https://testpalm.yandex-team.ru/testcase/alice-1853
        https://testpalm.yandex-team.ru/testcase/alice-1859
    """

    owners = ('norchine', 'abc:alice_iot')

    def test_alice_1853(self, alice):
        response = alice('включи розетку')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Включаю.',
            ]
        )

    def test_alice_1859(self, alice):
        response = alice('выключи розетку')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Выключаю.',
                'Окей, выключаем.',
                'Окей, выключаю.',
            ]
        )
