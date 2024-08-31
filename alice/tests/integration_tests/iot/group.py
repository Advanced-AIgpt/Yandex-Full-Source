import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

import iot.configs.group as config
from iot.common import is_iot, get_selected_hypothesis, get_iot_reaction, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.nlg as nlg


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.group_and_room)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestGroup(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1883
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи всё в группе люстра',
        'включи группу люстра',
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

    @pytest.mark.parametrize('command', [
        'выключи всё в группе люстра',
        'выключи группу люстра',
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
@pytest.mark.iot(config.group_and_room)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestGroupStateChange(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2610
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи свет в группе люстра',
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

    @pytest.mark.parametrize('command', [
        'смени цвет на синий группе люстра',
    ])
    def test_switch_color(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.color_change)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert set(action_parameters['devices']) == {'evo-test-lamp-id-1', 'evo-test-lamp-id-2'}
            assert action_parameters['capability_type'] == 'devices.capabilities.color_setting'
            assert action_parameters['capability_instance'] == 'color'
            assert action_parameters['capability_value'] == 'blue'
            return

        response_info = get_selected_hypothesis(response)
        assert set(response_info['devices']) == {'evo-test-lamp-id-1', 'evo-test-lamp-id-2'}
        assert response_info['action']['type'] == 'devices.capabilities.color_setting'
        assert response_info['action']['instance'] == 'color'
        assert response_info['action']['color_setting_capability_value'] == 'blue'

    @pytest.mark.parametrize('command', [
        'поставь яркость в группе люстра на 30%',
    ])
    def test_change_brightness(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.brightness_change)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert set(action_parameters['devices']) == {'evo-test-lamp-id-1', 'evo-test-lamp-id-2'}
            assert action_parameters['capability_type'] == 'devices.capabilities.range'
            assert action_parameters['capability_instance'] == 'brightness'
            assert action_parameters['capability_value'] == '30'
            return

        response_info = get_selected_hypothesis(response)
        assert set(response_info['devices']) == {'evo-test-lamp-id-1', 'evo-test-lamp-id-2'}
        assert response_info['action']['type'] == 'devices.capabilities.range'
        assert response_info['action']['instance'] == 'brightness'
        assert response_info['action']['range_capability_value'] == '30'
