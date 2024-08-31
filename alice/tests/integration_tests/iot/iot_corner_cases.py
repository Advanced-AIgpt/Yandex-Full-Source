import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

import iot.configs.iot_corner_cases as config
from iot.common import is_iot, get_iot_reaction, get_selected_hypothesis, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.nlg as nlg


@pytest.mark.no_oauth
@pytest.mark.parametrize('surface', [
    surface.navi,
    surface.searchapp,
    surface.yabro_win,
])
class TestIotNotAuthorized(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1860
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи лампочку',
        'включи свет в прихожей',
        'включи чайник',
    ])
    @pytest.mark.xfail(reason='https://st.yandex-team.ru/IOT-931')
    def test_alice_1860(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            [
                'Чтобы управлять Умным домом, вам нужно войти в свой Яндекс аккаунт.'
            ]
        )

        # test fails here - tv gallery should be show instead
        response = alice('включи телевизор')
        assert is_iot(response) is False


@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestIotWrongDevices(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1862
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.oauth(auth.SmarthomeOther)
    def test_alice_1862_1(self, alice):
        response = alice('включи пылесос')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            [
                'Не нашла устройство "пылесос". Проверьте настройки умного дома.'
            ]
        )

        response = alice('включи кофеварку')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            [
                'Не нашла устройство "кофеварка". Проверьте настройки умного дома.'
            ]
        )

        response = alice('включи свет')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            [
                'Не нашла устройств с типом "освещение". Проверьте настройки умного дома.'
            ]
        )

    @pytest.mark.oauth(auth.Smarthome)
    def test_alice_1862_2(self, alice):
        response = alice('включи свет в офисе')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            [
                'Не нашла комнату "офис". Проверьте настройки умного дома.'
            ]
        )

        response = alice('включи кофеварку в офисе')
        assert is_iot(response) is True
        assert not response.suggests
        assert_response_text(
            response.text,
            [
                'Не могу выполнить запрос. Проверьте настройки умного дома.'
            ]
        )


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.multiple_devices)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestMultipleDevices(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2232
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи свет и пылесос',
        'включи пылесос и лампочку',
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
            assert set(action_parameters['devices']) == {'evo-test-lamp-id-1', 'evo-test-vacuum-id-1'}
            assert action_parameters['capability_type'] == 'devices.capabilities.on_off'
            assert action_parameters['capability_value'] == 'true'
            return

        response_info = get_selected_hypothesis(response)
        assert set(response_info['devices']) == {'evo-test-lamp-id-1', 'evo-test-vacuum-id-1'}
        assert response_info['action']['type'] == 'devices.capabilities.on_off'
        assert response_info['action']['on_of_capability_value']


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.lamp_without_room)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestNotConfiguredDevice(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1946
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        pytest.param('включи лампочку', marks=pytest.mark.iot(config.lamp_without_room)),
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


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestXiaomiNames(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2240
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command, evo_test_id', [
        pytest.param('включи чайник', 'evo-test-kettle-id-1', marks=pytest.mark.iot(config.xiaomi_kettle)),
        pytest.param('включи розетку', 'evo-test-socket-id-1', marks=pytest.mark.iot(config.xiaomi_socket)),
        pytest.param('включи лампочку', 'evo-test-bulb-id-1', marks=pytest.mark.iot(config.xiaomi_bulb)),
        pytest.param('включи розетку', 'evo-test-outlet-id-1', marks=pytest.mark.iot(config.xiaomi_outlet)),
        pytest.param('включи пылесос', 'evo-test-vacuum-id-1', marks=pytest.mark.iot(config.xiaomi_vacuum)),
        pytest.param('включи ленту', 'evo-test-strip-id-1', marks=pytest.mark.iot(config.xiaomi_strip)),
        pytest.param('включи мультипекарь', 'evo-test-multibaker-id-1', marks=pytest.mark.iot(config.xiaomi_multibaker)),
        pytest.param('включи переключатель', 'evo-test-switch-id-1', marks=pytest.mark.iot(config.xiaomi_switch)),
        pytest.param('включи кофемашину', 'evo-test-coffeemaker-id-1', marks=pytest.mark.iot(config.xiaomi_coffeemaker)),
    ])
    def test_turn_on(self, alice, command, evo_test_id):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.turn_on)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert action_parameters['devices'] == [evo_test_id]
            assert action_parameters['capability_type'] == 'devices.capabilities.on_off'
            assert action_parameters['capability_value'] == 'true'
            return

        response_info = get_selected_hypothesis(response)
        assert response_info['devices'] == [evo_test_id]
        assert response_info['action']['type'] == 'devices.capabilities.on_off'
        assert response_info['action']['on_of_capability_value']


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.sdc])
class TestSelfDrivingCar(object):
    """
    https://st.yandex-team.ru/IOT-1601
    """

    owners = ('aaulayev', 'abc:alice_iot')

    def test_turn_on(self, alice):
        response = alice('включи свет')
        assert is_iot(response)
        assert_response_text(response.text, nlg.sdc_not_supported)
