import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

import iot.configs.room as config
from iot.common import is_iot, get_selected_hypothesis, get_iot_reaction, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.nlg as nlg


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.group_and_room)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestRoom(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1884
    """
    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи всё на кухне',  # https://st.yandex-team.ru/IOT-801
        'открой мой дом'  # https://st.yandex-team.ru/ALICE-15614
    ])
    def test_turn_on_all(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.turn_on_all_stub)

    @pytest.mark.parametrize('command', [
        'выключи всё на кухне',
    ])
    def test_turn_off(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.turn_off)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert set(action_parameters['devices']) == {'evo-test-lamp-id-1', 'evo-test-lamp-id-2'}
            assert action_parameters['capability_type'] == 'devices.capabilities.on_off'
            assert action_parameters['capability_value'] == 'false'
            return

        response_info = get_selected_hypothesis(response)
        assert set(response_info['devices']) == {'evo-test-lamp-id-1', 'evo-test-lamp-id-2'}
        assert response_info['action']['type'] == 'devices.capabilities.on_off'
        assert not response_info['action']['on_of_capability_value']


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.multiple_rooms)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestMultipleRooms(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2594
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи свет',
    ])
    def test_turn_on(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.turn_on)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert set(action_parameters['devices']) == {'evo-test-lamp-id-1', 'evo-test-lamp-id-2'}
            assert action_parameters['capability_type'] == 'devices.capabilities.on_off'
            assert action_parameters['capability_value'] == 'true'
            return

        response_info = get_selected_hypothesis(response)
        assert set(response_info['devices']) == {'evo-test-lamp-id-1', 'evo-test-lamp-id-2'}
        assert response_info['action']['type'] == 'devices.capabilities.on_off'
        assert response_info['action']['on_of_capability_value']
