import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

import iot.configs.curtain as config
from iot.common import is_iot, get_selected_hypothesis, get_iot_reaction, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.nlg as nlg


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.curtain)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestCurtain(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2892
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'открой шторы',
        'подними шторы',
    ])
    def test_turn_on(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.open)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert action_parameters['devices'] == ['evo-test-curtain-id-1']
            assert action_parameters['capability_type'] == 'devices.capabilities.on_off'
            assert action_parameters['capability_value'] == 'true'
            return

        response_info = get_selected_hypothesis(response)
        assert response_info['devices'] == ['evo-test-curtain-id-1']
        assert response_info['action']['type'] == 'devices.capabilities.on_off'
        assert response_info['action']['on_of_capability_value']

    @pytest.mark.parametrize('command', [
        'закрой шторы',
        'опусти шторы',
    ])
    def test_turn_off(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.close)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert action_parameters['devices'] == ['evo-test-curtain-id-1']
            assert action_parameters['capability_type'] == 'devices.capabilities.on_off'
            assert action_parameters['capability_value'] == 'false'
            return

        response_info = get_selected_hypothesis(response)
        assert response_info['devices'] == ['evo-test-curtain-id-1']
        assert response_info['action']['type'] == 'devices.capabilities.on_off'
        assert not response_info['action']['on_of_capability_value']

    @pytest.mark.parametrize('command', [
        'приоткрой шторы на половину',
        'приоткрой шторы ещё на половину',
    ])
    def test_range(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.ok)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert action_parameters['devices'] == ['evo-test-curtain-id-1']
            assert action_parameters['capability_type'] == 'devices.capabilities.range'
            assert action_parameters['capability_instance'] == 'open'
            assert action_parameters['capability_value'] == '50'
            return

        response_info = get_selected_hypothesis(response)
        assert response_info['devices'] == ['evo-test-curtain-id-1']
        assert response_info['action']['type'] == 'devices.capabilities.range'
        assert not response_info['action']['range_capability_value'] == 50
