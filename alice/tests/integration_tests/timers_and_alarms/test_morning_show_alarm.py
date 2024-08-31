import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('hw_alarm_morning_show_exp')
class TestMorningShowAlarm(object):

    owners = ('danibw', 'abc:alice_scenarios_alarm',)

    @pytest.mark.oauth(auth.YandexPlus)
    def test_with_plus(self, alice):
        response = alice('поставь утреннее шоу на будильник')

        assert response.scenario == scenario.Alarm
        assert response.directive.name == directives.names.AlarmSetSoundDirective
