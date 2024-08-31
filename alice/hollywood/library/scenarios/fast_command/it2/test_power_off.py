import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['fast_command']


@pytest.mark.scenario(name='Commands', handle='fast_command')
@pytest.mark.experiments('stroka_yabro')
class TestCommandsPowerOff:

    @pytest.mark.parametrize('surface', [surface.yabro_win])
    def test_power_off(self, alice):
        r = alice(voice('заверши работу компьютера'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech in [
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

        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('PowerOffDirective')
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'commands_other'
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.stroka.power_off'

        return str(r)

    @pytest.mark.parametrize('surface', [surface.yabro_win])
    def test_power_off_non_pc_phrase(self, alice):
        r = alice(voice('выключи'))
        assert r.scenario_stages() == {'run'}
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0

    @pytest.mark.parametrize('surface', [surface.station])
    def test_power_off_disabled_on_surface(self, alice):
        r = alice(voice('заверши работу компьютера'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.Features.IsIrrelevant
