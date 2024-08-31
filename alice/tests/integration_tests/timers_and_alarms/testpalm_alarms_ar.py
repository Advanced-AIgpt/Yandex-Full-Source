import alice.tests.library.intent as intent
from datetime import timedelta

import alice.tests.library.locale as locale
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.loudspeaker])
@pytest.mark.parametrize('locale', [locale.ar_sa(use_tanker=False)])
class _TestBase(object):
    owners = ('moath-alali', 'g:alice_quality')


class TestPalmAlarms(_TestBase):

    @pytest.mark.xfail(reason='HollywoodMusic instead of alarm_set')
    def test_alarm_0(self, alice):
        # set an alarm at seven am
        response = alice('اضبط منبه على السابعة صباحا')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].hour == 7

        # set an alarm after three minutes
        response = alice('اضبط منبه بعد ثلاث دقائق')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet

        second_alarm = alice.datetime_now + timedelta(minutes=3)
        alarms = alice.device_state.alarms
        assert len(alarms) == 2
        assert alarms[0].hour == 7
        assert alarms[1].minute == second_alarm.minute

        # remove the alarm set at seven am
        alice('احذف المنبه المضبوط على السابعة صباحا')
        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].minute == second_alarm.minute

    @pytest.mark.xfail(reason='Separators now are being removed by SmartTokenizer, and they are being used in parsing '
                              'time/date, we can parse without them or keep them in text')
    def test_alarm_1(self, alice):
        # set an alarm at 20:30
        response = alice('اضبط المنبه على الساعة 20:30')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].hour == 20
        assert alarms[0].minute == 30

    def test_alarm_2(self, alice):
        # set an alarm at five am every Tuesday
        response = alice('اضبطي منبه على الخامسة صباحا كل يوم الثلاثاء')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].hour == 5
        assert alarms[0].minute == 0

    def test_alarm_3(self, alice):
        # set an alarm at seven on working days
        response = alice('اضبط منبه على الساعة السابعة في أيام العمل')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].hour == 7
        assert alarms[0].minute == 0
        assert 'BYDAY=MO,TU,WE,TH,SU' in response.directive.payload.state

    def test_alarm_4(self, alice):
        # set an alarm at fifteen minutes past seven on working days
        response = alice('ضع منبه على الساعة السابعة وخمسة عشر دقيقة في أيام العمل')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].hour == 7
        assert alarms[0].minute == 15
        assert 'BYDAY=MO,TU,WE,TH,SU' in response.directive.payload.state

        # do I have any alarms?
        response = alice('هل لدي منبهات؟')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmShow

    @pytest.mark.xfail(reason='alarm_set and alarm_show instead of alarm_how_to_set_sound')
    @pytest.mark.parametrize('command', [
        'كيف يمكنني ضبط نغمة المنبه؟',  # how can I set alarm sound?
        'ما هي النغمات التي يمكنك ضبطها للمنبه؟'  # what sounds can I set for the alarm?
    ])
    def test_alarm_5(self, alice, command):
        response = alice(command)
        assert response.intent == intent.AlarmHowToSetSound

    @pytest.mark.xfail(reason='HollywoodMusic instead of alarm_set')
    def test_alarm_6(self, alice):
        # set an alarm after nine hours
        response = alice('اضبطي منبه بعد تسع ساعات')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        alarms = alice.device_state.alarms
        alarm = alice.datetime_now + timedelta(hours=9)
        assert len(alarms) == 1
        assert alarms[0].hour == alarm.hour
        assert alarms[0].minute == alarm.minute

    @pytest.mark.xfail(reason='HollywoodMusic instead of alarm_set')
    def test_alarm_7(self, alice):
        # set an alarm at third to ten am (09:40 am)
        response = alice('اضبطي منبه على العاشرة الا ثلث صباحا')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].hour == 9
        assert alarms[0].minute == 40

    @pytest.mark.xfail(reason='Vins instead of Alarm')
    def test_alarm_8(self, alice):
        # set an alarm at ten am
        response = alice('اضبطي منبه على العاشرة صباحا')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].hour == 10
        assert alarms[0].minute == 0

        # remove the alarm
        response = alice('احذف المنبه')
        assert len(alice.device_state.alarms) == 0

    @pytest.mark.xfail(reason='Alarm cancel Ellipses does not work')
    def test_alarm_9(self, alice):
        # set an alarm at ten am
        response = alice('اضبطي منبه على العاشرة صباحا')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].hour == 10
        assert alarms[0].minute == 0

        # set an alarm at seven am
        response = alice('اضبطي منبه على السابعة صباحا')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        alarms = alice.device_state.alarms
        assert len(alarms) == 2
        assert alarms[1].hour == 7
        assert alarms[1].minute == 0

        # set an alarm at three in the afternoon
        response = alice('اضبطي منبه على الثالثة ظهرا')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
        alarms = alice.device_state.alarms
        assert len(alarms) == 3
        assert alarms[2].hour == 15
        assert alarms[2].minute == 0

        # remove the alarm
        response = alice('احذف المنبه')
        assert len(alice.device_state.alarms) == 3

        # set at seven
        alice('المضبوط على الساعة السابعة')
        assert len(alice.device_state.alarms) == 2
        assert alarms[0].hour == 10
        assert alarms[1].hour == 15

        # remove the alarm
        response = alice('احذف المنبه')
        assert len(alice.device_state.alarms) == 2

        # set at 10
        alice('المضبوط على الساعة العاشرة')
        assert len(alice.device_state.alarms) == 1
        assert alarms[0].hour == 15

    def test_alarm_10(self, alice):
        # set an alarm at ten am
        response = alice('اضبطي منبه على العاشرة صباحا')
        assert response.scenario == scenario.Alarm
        assert response.intent == intent.AlarmSet
