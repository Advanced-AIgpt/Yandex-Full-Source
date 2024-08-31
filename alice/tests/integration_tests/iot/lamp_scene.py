import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

import iot.configs.lamp_scene as config
from iot.common import is_iot, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.nlg as nlg


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.lamp_scene)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestLampModeCheck(object):

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'Включи светильник в режиме свечи',
        'Сделай на светильнике режим Вечеринки',
        'Вруби режим Алиса на светильнике',
        'Включи режим океана в спальне',
        'Поставь светильник в режим чтения',
        'Включи режим света океан',
        'Включи режим тревоги',
    ])
    def test_turn_on(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert_response_text(response.text, nlg.ok)
