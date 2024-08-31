import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

import iot.configs.scenario as config
from iot.common import is_iot, get_selected_hypothesis, get_iot_reaction, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.nlg as nlg


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.scenario)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestScenario(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1886
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи музыку',
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
        'выключи звук',
    ])
    def test_turn_off(self, alice, command):
        response = alice(command)
        assert_response_text(response.text, nlg.scenario_run)
        assert is_iot(response)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'scenario'
            scenario_parameters = iot_reaction['scenario_parameters']
            assert scenario_parameters is not None
            assert scenario_parameters['scenarios'] == ['evo-test-scenario-id-1']
            return

        response_info = get_selected_hypothesis(response)
        assert response_info['scenario'] == 'evo-test-scenario-id-1'


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.station_scenario)
@pytest.mark.parametrize('surface', [surface.station])
class TestScenarioStation(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2635
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'запусти сценарий елочка гори',
        'включи сценарий елочка гори',
        'сценарий включи елочка гори',
        'елочка гори',
    ])
    def test_turn_on(self, alice, command):
        response = alice(command)
        assert_response_text(response.text, nlg.scenario_run)
        assert is_iot(response)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'scenario'
            scenario_parameters = iot_reaction['scenario_parameters']
            assert scenario_parameters is not None
            assert scenario_parameters['scenarios'] == ['evo-test-scenario-id-1']
            return

        response_info = get_selected_hypothesis(response)
        assert response_info['scenario'] == 'evo-test-scenario-id-1'
