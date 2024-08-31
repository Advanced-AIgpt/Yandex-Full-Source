import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

import iot.configs.vacuum_cleaner as config
from iot.common import is_iot, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.nlg as nlg


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.vacuum_cleaner)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestVacuumCleanerWithConfig(object):

    owners = ('norchine', 'abc:alice_iot')

    names = [
        'робот пылесос',
        'робот-пылесос',
        'пылесос',
        'Аркадия',
    ]

    @pytest.mark.parametrize('name', names)
    def test_vacuum_cleaner_stop(self, alice, name):
        response = alice(f'останови {name}')
        assert is_iot(response)
        assert_response_text(response.text, nlg.ok + nlg.turn_off)

    @pytest.mark.parametrize('name', names)
    @pytest.mark.parametrize('place', [
        'домой',
        'на зарядку',
        'заряжаться',
    ])
    def test_vacuum_cleaner_send(self, alice, name, place):
        response = alice(f'отправь {name} {place}')
        assert is_iot(response)
        assert_response_text(response.text, nlg.ok)

    @pytest.mark.parametrize('task', [
        'уборку',
        'пылесосить',
        'убираться',
    ])
    def test_vacuum_cleaner_continue(self, alice, task):
        response = alice(f'продолжи {task}')
        assert is_iot(response)
        assert_response_text(response.text, nlg.ok)
