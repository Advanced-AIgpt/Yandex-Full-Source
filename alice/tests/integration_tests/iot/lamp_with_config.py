import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

import iot.configs.lamp_with_config as config
from iot.common import is_iot, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.nlg as nlg


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.lamp)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestColorLampWithConfig(object):

    owners = ('galecore', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи свет',
        'включи лампочку',
    ])
    def test_turn_on(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Включаю.',
            ]
        )
