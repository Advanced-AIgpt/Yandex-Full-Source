import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.station])
class TestShowAlarm(object):

    owners = ('danibw', 'abc:alice_scenarios_alarm',)

    def test_without_alarms(self, alice):
        response = alice('покажи будильник')
        assert response.intent == intent.AlarmShow
        assert response.text in [
            'У вас нет установленных будильников.',
            'Вы меня не просили вас разбудить.',
        ]


@pytest.mark.parametrize('surface', [surface.station])
class TestUpdateAlarms(object):

    owners = ('danibw', 'abc:alice_scenarios_alarm',)

    def test_one_alarm(self, alice):
        response = alice('поставь будильник')
        assert response.intent == intent.AlarmSet
        assert not response.directive
        assert response.text == 'На какое время поставить будильник?'

        response = alice('на 7 утра')
        assert response.directive.name == directives.names.AlarmsUpdateDirective

    def test_set_seven_am_alarm(self, alice):
        response = alice('поставь будильник на 7 утра')
        assert response.directive.name == directives.names.AlarmsUpdateDirective


@pytest.mark.parametrize('surface', [surface.station])
class TestAlarmCancelEllipsis(object):

    owners = ('danibw', 'abc:alice_scenarios_alarm',)

    @pytest.mark.parametrize('command', [
        'да',
        'выключи',
        'да выключи',
        'выключи будильник'
    ])
    def test_cancel_one(self, alice, command):
        response = alice('поставь будильник на 7 утра')
        assert response.intent == intent.AlarmSet
        assert response.scenario == scenario.Alarm
        assert response.directive.name == directives.names.AlarmsUpdateDirective

        response = alice('удали будильник на 8 утра')
        assert response.intent == intent.AlarmCancel
        assert response.scenario == scenario.Alarm
        assert not response.directive
        assert response.text == 'Сейчас установлен только один будильник на завтра в 7 часов утра, выключить его?'

        response = alice(command)
        assert response.intent == intent.AlarmCancelEllipsis
        assert response.scenario == scenario.Alarm
        assert response.directive.name == directives.names.AlarmsUpdateDirective
