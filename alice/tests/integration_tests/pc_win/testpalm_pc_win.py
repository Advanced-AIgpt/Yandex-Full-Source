import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.experiments('stroka_yabro')
@pytest.mark.parametrize('surface', [surface.yabro_win])
class TestPcPowerOff(object):
    owners = ('nkodosov', )

    @pytest.mark.parametrize('command', ['выключи компьютер', 'завершение работы пк', 'выруби ноут'])
    def test_alice_2178(self, alice, command):
        response = alice(command)
        assert response.directive.name == directives.names.PowerOffDirective
        assert response.scenario == scenario.Commands
        assert response.text in [
            'Будет сделано',
            'Выключаю',
            'Выключаю компьютер',
            'Выключаю, до встречи',
            'Выключаю, до скорых встреч',
            'Выключаю, увидимся завтра',
            'Выключаю, услышимся завтра',
            'Выполняю',
            'Ок, выключаю',
            'Ок, выключаю компьютер',
            'Хорошо, выключаю',
        ]

    @pytest.mark.parametrize('command', ['стоп', 'выключись'])
    def test_ignore_power_off_on_phrase(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Commands
        assert not response.directive
