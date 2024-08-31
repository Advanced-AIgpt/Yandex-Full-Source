import pytest

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface


@pytest.mark.voice
@pytest.mark.experiments(
    f'mm_enable_protocol_scenario={scenario.CecCommands}',
)
@pytest.mark.parametrize('surface', surface.actual_surfaces)
class _TestBase(object):
    owners = ('vl-trifonov', 'olegator', 'abc:alice_quality')


class TestScreenOn(_TestBase):

    @pytest.mark.oauth(auth.Smarthome)
    @pytest.mark.device_state(is_tv_plugged_in=True)
    @pytest.mark.parametrize('command', [
        'включи телевизор',
    ])
    def test_plugged_iot(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.IoT

    @pytest.mark.oauth(auth.Smarthome)
    @pytest.mark.device_state(is_tv_plugged_in=False)
    @pytest.mark.parametrize('command', [
        'включи телевизор',
    ])
    def test_unplugged_iot(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.IoT

    @pytest.mark.device_state(is_tv_plugged_in=True)
    @pytest.mark.parametrize('command', [
        'включи телевизор',
        'вруби экран',
        'вруби монитор',
    ])
    def test_plugged(self, alice, command):
        response = alice(command)

        if 'cec_available' not in alice.supported_features:
            assert response.scenario != scenario.CecCommands
            return

        assert response.scenario == scenario.CecCommands
        assert response.text in {
            'Включаю.',
            'Запускаю.',
            'Сейчас включу.',
            'Секунду.',
            'Секундочку.',
        }
        assert len(response.directives) == 1
        assert response.directive.name == directives.names.ScreenOnDirective

    @pytest.mark.device_state(is_tv_plugged_in=False)
    @pytest.mark.parametrize('command', [
        'включи телевизор',
        'вруби экран',
        'вруби монитор',
    ])
    def test_unplugged(self, alice, command):
        response = alice(command)
        assert response.scenario != scenario.CecCommands

    @pytest.mark.oauth(auth.Smarthome)
    @pytest.mark.device_state(is_tv_plugged_in=True)
    @pytest.mark.parametrize('command', [
        'вруби экран',
        'вруби монитор',
    ])
    def test_plugged_iot_only_cec_commands(self, alice, command):
        response = alice(command)

        if 'cec_available' not in alice.supported_features:
            assert response.scenario != scenario.CecCommands
            return

        assert response.scenario == scenario.CecCommands
        assert response.text in {
            'Включаю.',
            'Запускаю.',
            'Сейчас включу.',
            'Секунду.',
            'Секундочку.',
        }
        assert len(response.directives) == 1
        assert response.directive.name == directives.names.ScreenOnDirective

    @pytest.mark.oauth(auth.Smarthome)
    @pytest.mark.device_state(is_tv_plugged_in=False)
    @pytest.mark.parametrize('command', [
        'вруби экран',
        'вруби монитор',
    ])
    def test_unplugged_iot_only_cec_commands(self, alice, command):
        response = alice(command)
        assert response.scenario != scenario.CecCommands


class TestScreenOff(_TestBase):

    @pytest.mark.oauth(auth.Smarthome)
    @pytest.mark.device_state(is_tv_plugged_in=True)
    @pytest.mark.parametrize('command', [
        'выключи телевизор',
    ])
    def test_plugged_iot(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.IoT

    @pytest.mark.oauth(auth.Smarthome)
    @pytest.mark.device_state(is_tv_plugged_in=False)
    @pytest.mark.parametrize('command', [
        'выключи телевизор',
    ])
    def test_unplugged_iot(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.IoT

    @pytest.mark.device_state(is_tv_plugged_in=True)
    @pytest.mark.parametrize('command', [
        'выключи телевизор',
        'выруби экран',
        'выруби монитор',
    ])
    def test_plugged(self, alice, command):
        response = alice(command)

        if 'cec_available' not in alice.supported_features:
            assert response.scenario != scenario.CecCommands
            return

        assert response.scenario == scenario.CecCommands
        assert response.text in {
            'Выключаю.',
            'Сейчас выключу.',
            'Секунду.',
            'Секундочку.',
        }
        assert len(response.directives) == 1
        assert response.directive.name == directives.names.ScreenOffDirective

    @pytest.mark.device_state(is_tv_plugged_in=False)
    @pytest.mark.parametrize('command', [
        'выключи телевизор',
        'выруби экран',
        'выруби монитор',
    ])
    def test_unplugged(self, alice, command):
        response = alice(command)
        assert response.scenario != scenario.CecCommands

    @pytest.mark.oauth(auth.Smarthome)
    @pytest.mark.device_state(is_tv_plugged_in=True)
    @pytest.mark.parametrize('command', [
        'выруби экран',
        'выруби монитор',
    ])
    def test_plugged_iot_only_cec_commands(self, alice, command):
        response = alice(command)

        if 'cec_available' not in alice.supported_features:
            assert response.scenario != scenario.CecCommands
            return

        assert response.scenario == scenario.CecCommands
        assert response.text in {
            'Выключаю.',
            'Сейчас выключу.',
            'Секунду.',
            'Секундочку.',
        }
        assert len(response.directives) == 1
        assert response.directive.name == directives.names.ScreenOffDirective

    @pytest.mark.oauth(auth.Smarthome)
    @pytest.mark.device_state(is_tv_plugged_in=False)
    @pytest.mark.parametrize('command', [
        'выруби экран',
        'выруби монитор',
    ])
    def test_unplugged_iot_only_cec_commands(self, alice, command):
        response = alice(command)
        assert response.scenario != scenario.CecCommands
