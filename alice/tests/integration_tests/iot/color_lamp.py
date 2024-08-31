import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import pytest

from iot.common import is_iot, get_selected_hypothesis, get_iot_reaction, assert_response_text
from iot.common import no_config_skip   # noqa: F401
import iot.nlg as nlg
import iot.configs.color_lamp as config


@pytest.mark.oauth(auth.Smarthome)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestColorLamp(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1847
    https://testpalm.yandex-team.ru/testcase/alice-1854
    https://testpalm.yandex-team.ru/testcase/alice-1855
    https://testpalm.yandex-team.ru/testcase/alice-1863
    https://testpalm.yandex-team.ru/testcase/alice-1935
    https://testpalm.yandex-team.ru/testcase/alice-1956
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи свет на кухне',
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

    @pytest.mark.parametrize('command', [
        'лампочку выключи',
        'лампочка выключить',
        'свет выключить',
        'выключи лампочку',
    ])
    def test_turn_off(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Выключаю.',
                'Окей, выключаем.',
                'Окей, выключаю.',
            ]
        )

    @pytest.mark.parametrize('command', [
        'включи лампочку поярче',
        'сделай свет поярче',
        'увеличь яркость света',
        'включи побольше света',
        'увеличь яркость лампочки на 20%',
    ])
    def test_brightness_increase(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Окей, сделаем поярче.',
                'Окей, делаем ярче.',
                'Окей, добавим яркости.',
                'Добавляю яркости.',
                'Больше яркости. Окей.',
                'Окей.',
            ]
        )

    @pytest.mark.parametrize('command', [
        'приглуши свет',
        'убавь яркость лампочки',
        'уменьшить яркость на лампочке',
        'сделай яркость лампочки поменьше',
        'сделай свет тусклее',
        'включи поменьше света',
        'уменьши яркость света на 10%',
    ])
    def test_brightness_decrease(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Окей, сделаем темнее.',
                'Окей, делаем темнее.',
                'Окей, убавим яркость.',
                'Убавляю яркость.',
                'Меньше яркости. Окей.',
            ]
        )

    @pytest.mark.parametrize('command', [
        'включи яркость лампочки на максимум',
        'включи яркость на лампочке на максимум',
        'включи свет на полную',
        'включи свет на полную катушку',
    ])
    def test_brightness_max(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Окей, яркость на максимум.',
                'Хорошо, яркость на максимум.',
                'Хорошо. Максимум яркости.',
                'Как скажете. Максимальная яркость.',
            ]
        )

    @pytest.mark.parametrize('command', [
        'сделай яркость лампочки на минимум',
        'включи яркость лампочки миниум',
        'включи тусклый свет',
    ])
    def test_brightness_min(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Хорошо. Минимум яркости.',
                'Как скажете. Минимальная яркость.',
            ]
        )

    @pytest.mark.parametrize('command', [
        'яркость лампочки 20%',
        'сделай яркость лампочки на 77%',
    ])
    def test_brightness_value(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Меняю яркость.',
            ]
        )

    @pytest.mark.parametrize('command', [
        'cделай свет лампочки потеплее',
    ])
    def test_temperature_increase(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Включаю свет потеплее.',
                'Окей, больше тёплого света.',
                'Окей, больше тёплого.',
            ]
        )

    @pytest.mark.parametrize('command', [
        'включи на лампочке свет похолоднее',
    ])
    def test_temperature_decrease(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Включаю свет похолоднее.',
                'Окей, больше холодного света.',
                'Окей, больше холодного. ',
            ]
        )

    @pytest.mark.parametrize('command', [
        'cделай свет лампочки',
    ])
    @pytest.mark.parametrize('temp', [
        'холодный белый',
        'дневной белый',
        'теплый белый',
    ])
    def test_temperature_change(self, alice, command, temp):
        response = alice(f'{command} {temp}')
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Меняю цвет.',
            ]
        )

    @pytest.mark.parametrize('command', [
        'поменяй цвет лампочки на зеленый',
        'включи оранжевый свет',
        'поставь синий цвет на лампочке',
        'измени цвет на коралловый',
        'включи зеленый цвет',
        'вруби красный цвет',
        'включи красный цвет в кухне',
        'включи красный цвет лампочке',
        'включи оранжевый лампочке',
        'оранжевый в кухне',
        'оранжевый на лампочке',
    ])
    def test_color_change(self, alice, command):
        response = alice(command)
        assert is_iot(response) is True
        assert_response_text(
            response.text,
            nlg.ok + [
                'Окей.',
                'Меняю цвет.',
            ]
        )

    @pytest.mark.parametrize('command', [
        'желтый',
        'зеленый',
        'малиновый',
        'синий цвет',
        'коралловый цвет',
    ])
    def test_color_incorrect(self, alice, command):
        response = alice(command)
        assert is_iot(response) is False


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.double_names)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestDifferentWordsOrder(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2238
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи мою лампочку',
        'включи лампочку мою',
    ])
    def test_turn_on_one(self, alice, command):
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
        'включи свет в зале',
        'включи в зале свет',
    ])
    def test_turn_on_two(self, alice, command):
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


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.lamp_turn_on_all)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestTurnOnAll(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1849
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'включи свет',
        'включи освещение',
        'алиса включи свет',
        'включи пожалуйста свет',
    ])
    def test_turn_on_all(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.turn_on)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert set(action_parameters['devices']) == {'evo-test-lamp-id-1', 'evo-test-lamp-id-2', 'evo-test-lamp-id-3'}
            assert action_parameters['capability_type'] == 'devices.capabilities.on_off'
            assert action_parameters['capability_value'] == 'true'
            return

        response_info = get_selected_hypothesis(response)
        assert set(response_info['devices']) == {'evo-test-lamp-id-1', 'evo-test-lamp-id-2', 'evo-test-lamp-id-3'}
        assert response_info['action']['type'] == 'devices.capabilities.on_off'
        assert response_info['action']['on_of_capability_value']

    @pytest.mark.parametrize('command', [
        'свет',
    ])
    def test_turn_on_none(self, alice, command):
        response = alice(command)
        assert not is_iot(response)


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.iot(config.lamp_turn_off_change_brightness)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestTurnOffAll(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1850
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command', [
        'выключи свет',
        'выключи пожалуйста свет',
        'выключи освещение',
        'выруби свет',
    ])
    def test_turn_off_all(self, alice, command):
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
@pytest.mark.iot(config.lamp_turn_off_change_brightness)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestChangeBrightness(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1852
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command, range_capability_value, nlg_response', [
        ('включи яркость света на максимум', 'max', nlg.max_brightness),
        ('сделай яркость освещения на минимум', 'min', nlg.min_brightness),
        ('поставь яркость света на минимум', 'min', nlg.min_brightness),
    ])
    def test_extreme_values_brightness(self, alice, command, range_capability_value, nlg_response):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg_response)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert set(action_parameters['devices']) == {'evo-test-lamp-id-1', 'evo-test-lamp-id-2'}
            assert action_parameters['capability_type'] == 'devices.capabilities.range'
            assert action_parameters['capability_instance'] == 'brightness'
            assert action_parameters['capability_value'] == range_capability_value
            return

        response_info = get_selected_hypothesis(response)
        assert set(response_info['devices']) == {'evo-test-lamp-id-1', 'evo-test-lamp-id-2'}
        assert response_info['action']['type'] == 'devices.capabilities.range'
        assert response_info['action']['instance'] == 'brightness'
        assert response_info['action']['range_capability_value'] == range_capability_value

    @pytest.mark.parametrize('command, relative, nlg_response', [
        ('приглуши свет', 'decrease', nlg.dim),
        ('увеличь яркость света', 'increase', nlg.brighten),
    ])
    def test_change_brightness(self, alice, command, relative, nlg_response):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg_response)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert set(action_parameters['devices']) == {'evo-test-lamp-id-1', 'evo-test-lamp-id-2'}
            assert action_parameters['capability_type'] == 'devices.capabilities.range'
            assert action_parameters['capability_instance'] == 'brightness'
            assert action_parameters['relativity_type'] == relative
            return

        response_info = get_selected_hypothesis(response)
        assert set(response_info['devices']) == {'evo-test-lamp-id-1', 'evo-test-lamp-id-2'}
        assert response_info['action']['type'] == 'devices.capabilities.range'
        assert response_info['action']['instance'] == 'brightness'
        assert response_info['action']['relative'] == relative


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestSimilarNames(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1885
    """

    owners = ('norchine', 'abc:alice_iot')

    @pytest.mark.parametrize('command, evo_devices', [
        pytest.param('включи лампа', {'evo-test-lamp-id-1', 'evo-test-lamp-id-2'}, marks=pytest.mark.iot(config.lamp_similar_names_all_lamps)),
        pytest.param('включи лампу', {'evo-test-lamp-id-1', 'evo-test-lamp-id-2'}, marks=pytest.mark.iot(config.lamp_similar_names_all_lamps)),
        pytest.param('включи лампу', {'evo-test-lamp-id-4'}, marks=pytest.mark.iot(config.lamp_similar_names_one_lamp)),
    ])
    def test_turn_on(self, alice, command, evo_devices):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(response.text, nlg.turn_on)

        iot_reaction = get_iot_reaction(response)
        if iot_reaction:
            assert iot_reaction['type'] == 'action'
            action_parameters = iot_reaction['action_parameters']
            assert action_parameters is not None
            assert set(action_parameters['devices']) == evo_devices
            assert action_parameters['capability_type'] == 'devices.capabilities.on_off'
            return

        response_info = get_selected_hypothesis(response)
        assert set(response_info['devices']) == evo_devices
        assert response_info['action']['type'] == 'devices.capabilities.on_off'
        assert response_info['action']['on_of_capability_value']

    @pytest.mark.parametrize('command', [
        pytest.param('включи лампу', marks=pytest.mark.iot(config.lamp_similar_names_two_lamps))
    ])
    def test_turn_on_two(self, alice, command):
        response = alice(command)
        assert is_iot(response)
        assert_response_text(
            response.text,
            [
                'Не нашла устройство "лампа". Проверьте настройки умного дома.'
            ]
        )
