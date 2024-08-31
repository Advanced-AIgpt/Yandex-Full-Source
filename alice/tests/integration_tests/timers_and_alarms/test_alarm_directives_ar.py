import alice.tests.library.directives as directives
import alice.tests.library.locale as locale
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.loudspeaker])
@pytest.mark.parametrize('locale', [locale.ar_sa(use_tanker=False)])
class _TestBase(object):
    owners = ('moath-alali', 'g:alice_quality')


class TestShowAlarm(_TestBase):

    requests = [
        'أرني المنبهات',  # show me alarms
        'اعرضي المنبهات المضبوطة',  # shwo the set alarms
        'ما هي المنبهات المفعلة',  # what alarms are active
        'اعرضي المنبهات ليوم الخميس'  # show alarms on Thursday
    ]

    @pytest.mark.parametrize('req', requests)
    def test_without_alarms(self, alice, req):
        response = alice(req)
        assert response.intent == intent.AlarmShow


class TestUpdateAlarms(_TestBase):

    requests_with_ellipsis = [
        (
            'اضبط منبه',  # set an alarm
            'على العشره وخمسين دقيقه صباحا',  # at fifty minutes past ten am
            10,
            50
        ), (
            'ضعي منبه',  # put an alarm
            'على الرابعة والربع ظهرا',  # at quarter past four pm
            16,
            15
        ), (
            'اضبط منبه',  # set an alarm
            'على الرابعة والربع ظهرا',  # at quarter past four pm
            16,
            15
        ), (
            'ضعي منبه',  # put an alarm
            'على العشره وخمسين دقيقه صباحا',  # at fifty minuts past ten am
            10,
            50
        ), (
            'اضبط منبه',  # set an alarm
            'عالخامسه وثلث',  # at third past five (05:20)
            5,
            20
        ), (
            'ضعي منبه',  # put an alarm
            'عالتاسعه وعشر دقائق',  # at ten minutes past nine
            9,
            10
        ), (
            'ضعي منبه',  # put an alarm
            'على التاسعه عشره',  # at ten past nine
            19,
            0
        )
    ]

    concrete_activation_requests = [
        pytest.param(
            'اضبط منبه على الساعة السابعة صباحا',  # set an alarm at seven am
            7,
            0,
            marks=pytest.mark.xfail(reason='Classification mistake')
        ), (
            'اضبط منبه على الساعة العاشرة الا ربع صباحا',  # set an alarm at quarter to ten am
            9,
            45
        )
    ]

    @pytest.mark.xfail(reason='Ellipses have not been implemented yet, some tests passed by chance, because set_alarm'
                       ' is working')
    @pytest.mark.parametrize('inconcrete_activation_request, ellipsis_request, hours, minutes',
                             requests_with_ellipsis)
    def test_alarm_set_ellipsis(self, alice, inconcrete_activation_request, ellipsis_request, hours, minutes):
        response = alice(inconcrete_activation_request)
        assert response.intent == intent.AlarmSet
        assert not response.directive
        response = alice(ellipsis_request)
        assert response.directive.name == directives.names.AlarmsUpdateDirective
        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].hour == hours
        assert alarms[0].minute == minutes

    @pytest.mark.parametrize('concrete_activation_request, hours, minutes', concrete_activation_requests)
    def test_alarm_set_concrete(self, alice, concrete_activation_request, hours, minutes):
        response = alice(concrete_activation_request)
        assert response.directive.name == directives.names.AlarmsUpdateDirective
        alarms = alice.device_state.alarms
        assert len(alarms) == 1
        assert alarms[0].hour == hours
        assert alarms[0].minute == minutes
