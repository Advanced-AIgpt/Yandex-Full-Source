import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

import iot.configs.white_lamp as config
from iot.common import is_iot, get_iot_reaction, get_selected_hypothesis, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.nlg as nlg


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.white_lamp)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestWhiteLamp(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1845
    https://testpalm.yandex-team.ru/testcase/alice-1846
    https://testpalm.yandex-team.ru/testcase/alice-1856
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи свет на кухне',
        'включи свет',
        'включи лампочку',
        'включи свет в комнате кухня',
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
            assert action_parameters['devices'] == ['evo-test-lamp-id-1']
            assert action_parameters['capability_type'] == 'devices.capabilities.on_off'
            assert action_parameters['capability_value'] == 'true'
            return

        response_info = get_selected_hypothesis(response)
        assert response_info['devices'] == ['evo-test-lamp-id-1']
        assert response_info['action']['type'] == 'devices.capabilities.on_off'
        assert response_info['action']['on_of_capability_value']

    @pytest.mark.parametrize('command', [
        'лампочку выключи',
        'лампочка выключить',
        'свет выключить',
        'выключи лампочку',
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
            assert action_parameters['devices'] == ['evo-test-lamp-id-1']
            assert action_parameters['capability_type'] == 'devices.capabilities.on_off'
            assert action_parameters['capability_value'] == 'false'
            return

        response_info = get_selected_hypothesis(response)
        assert response_info['devices'] == ['evo-test-lamp-id-1']
        assert response_info['action']['type'] == 'devices.capabilities.on_off'
        assert not response_info['action']['on_of_capability_value']

    @pytest.mark.parametrize('command', [
        'Сделай свет лампочки холоднее',
    ])
    def test_increase_temperature_k(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.increase_temperature_k)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert action_parameters['devices'] == ['evo-test-lamp-id-1']
            assert action_parameters['capability_type'] == 'devices.capabilities.color_setting'
            assert action_parameters['capability_instance'] == 'temperature_k'
            assert action_parameters['relativity_type'] == 'increase'
            return

        response_info = get_selected_hypothesis(response)
        assert response_info['devices'] == ['evo-test-lamp-id-1']
        assert response_info['action']['type'] == 'devices.capabilities.color_setting'
        assert response_info['action']['instance'] == 'temperature_k'
        assert response_info['action']['relative'] == 'increase'

    @pytest.mark.parametrize('command', [
        'Сделай свет лампочки потеплее',
    ])
    def test_decrease_temperature_k(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.decrease_temperature_k)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert action_parameters['devices'] == ['evo-test-lamp-id-1']
            assert action_parameters['capability_type'] == 'devices.capabilities.color_setting'
            assert action_parameters['capability_instance'] == 'temperature_k'
            assert action_parameters['relativity_type'] == 'decrease'
            return

        response_info = get_selected_hypothesis(response)
        assert response_info['devices'] == ['evo-test-lamp-id-1']
        assert response_info['action']['type'] == 'devices.capabilities.color_setting'
        assert response_info['action']['instance'] == 'temperature_k'
        assert response_info['action']['relative'] == 'decrease'

    @pytest.mark.parametrize('command', [
        'Включи холодный белый свет на лампочке',
        'Сделай свет лампочки холодный белый',
    ])
    def test_cold_white_color(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.color_change)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert action_parameters['devices'] == ['evo-test-lamp-id-1']
            assert action_parameters['capability_type'] == 'devices.capabilities.color_setting'
            assert action_parameters['capability_instance'] == 'color'
            assert action_parameters['capability_value'] == 'cold_white'
            return

        response_info = get_selected_hypothesis(response)
        assert response_info['devices'] == ['evo-test-lamp-id-1']
        assert response_info['action']['type'] == 'devices.capabilities.color_setting'
        assert response_info['action']['instance'] == 'color'
        assert response_info['action']['color_setting_capability_value'] == 'cold_white'
