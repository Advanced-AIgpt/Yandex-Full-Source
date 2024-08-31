import json

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice, callback
from alice.megamind.protos.scenarios.directives_pb2 import TCallbackDirective
from alice.protos.analytics.dummy_response.response_pb2 import TResponse
from google.protobuf import json_format


def _proto_to_dict(proto_message):
    json_str = json_format.MessageToJson(proto_message)
    return json.loads(json_str)


def _parse_callback(payload_pb):
    callback_pb = TCallbackDirective()
    callback_pb.Payload.CopyFrom(payload_pb)
    callback_pb.Name = 'update_form'

    result = _proto_to_dict(callback_pb)
    result.setdefault('payload', {}).update({
        '@scenario_name': 'Alarm'
    })
    return callback(**result)


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['alarm']


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='Alarm', handle='alarm')
@pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
class Tests:
    @pytest.mark.experiments('hw_alarm_morning_show_exp')
    @pytest.mark.supported_features('change_alarm_sound')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_morning_show_set(self, alice):
        r = alice(voice('поставь утреннее шоу на будильник'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech == \
            'Сделано. Теперь как проснетесь, будем слушать утреннее шоу. ' \
            'Чтобы оно включилось, после мелодии будильника скажите: дальше.'
        return str(r)

    @pytest.mark.experiments('hw_alarm_morning_show_exp')
    @pytest.mark.supported_features('change_alarm_sound')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_morning_show_what_sound_is_set(self, alice):
        r = alice(voice('поставь утреннее шоу на будильник'))
        r = alice(voice('что стоит на будильнике?'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech == 'На вашем будильнике стоит утреннее шоу.'
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_set_single_on_time(self, alice):
        r = alice(voice('поставь будильник на 8:40'))
        assert r.scenario_stages() == {'run'}
        assert 'на #acc 8 часов #acc 40 минут' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_set_single_on_time_with_day_part(self, alice):
        r = alice(voice('поставь будильник на 8 вечера'))
        assert r.scenario_stages() == {'run'}
        assert 'на #acc 20 часов' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_set_regular_on_weekday(self, alice):
        r = alice(voice('поставь будильник по понедельникам в 13:37'))
        assert r.scenario_stages() == {'run'}
        assert 'по понедельникам в #acc 13 часов #acc 37 минут' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_set_regular_on_many_weekdays(self, alice):
        r = alice(voice('поставь будильник по средам, вторникам и пятницам в 2:28'))
        assert r.scenario_stages() == {'run'}
        assert 'по вторникам, средам и пятницам в #acc 2 часа #acc 28 минут' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_set_regular_on_all_days(self, alice):
        r = alice(voice('поставь будильник ежедневно в 4:20'))
        assert r.scenario_stages() == {'run'}
        assert 'каждый день в #acc 4 часа #acc 20 минут' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_set_regular_on_weekdays(self, alice):
        r = alice(voice('поставь будильник по будням в 3:22'))
        assert r.scenario_stages() == {'run'}
        assert 'по будням в #acc 3 часа #acc 22 минуты' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_set_regular_on_weekends(self, alice):
        r = alice(voice('поставь будильник по воскресеньям и субботам в 3:22'))
        assert r.scenario_stages() == {'run'}
        assert 'по выходным в #acc 3 часа #acc 22 минуты' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_ask_time(self, alice):
        r = alice(voice('поставь будильник'))
        assert r.scenario_stages() == {'run'}
        assert 'на какое время' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        r = alice(voice('по воскресеньям и субботам в 3:22'))
        assert r.scenario_stages() == {'run'}
        assert 'по выходным в #acc 3 часа #acc 22 минуты' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_ask_time'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.experiments('alarm_day_part')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_ask_time_with_day_part(self, alice):
        r = alice(voice('поставь будильник утром'))
        assert r.scenario_stages() == {'run'}
        assert 'утро - это во сколько?' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        r = alice(voice('в 4'))
        assert r.scenario_stages() == {'run'}
        assert '#acc 4 часа утра' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_ask_time'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_ask_time_with_date(self, alice):
        r = alice(voice('поставь будильник на сегодня'))
        assert r.scenario_stages() == {'run'}
        assert 'на какое время' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        r = alice(voice('на девять часов десять минут'))
        assert r.scenario_stages() == {'run'}
        assert any(phrase in r.run_response.ResponseBody.Layout.OutputSpeech.lower() for phrase in [
            'в #acc 9 часов #acc 10 минут',
            'на #acc 9 часов #acc 10 минут',
        ])
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_ask_time'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    @pytest.mark.device_state(alarm_state={"currently_playing": True})
    def test_alarm_snooze_rel(self, alice):
        r = alice(voice('разбуди через пятнадцать минут'))
        assert r.scenario_stages() == {'run'}
        assert 'в #acc 5 часов #acc 59 минут' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_snooze_rel'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    @pytest.mark.device_state(alarm_state={"currently_playing": True})
    def test_alarm_snooze_rel_without_time(self, alice):
        r = alice(voice('дай поспать'))
        assert r.scenario_stages() == {'run'}
        assert 'в #acc 5 часов #acc 54 минуты' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_snooze_rel'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.experiments('hw_alarm_megamind_2906_fix')
    @pytest.mark.experiments('bg_fresh_granet_form=personal_assistant.scenarios.alarm_ask_time')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_set_on_00_15(self, alice):
        r = alice(voice('поставь будильники на 15 минут'))
        assert r.scenario_stages() == {'run'}
        assert 'номер 1. на завтра в #acc 0 часов #acc 15 минут' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert 'номер 2. на сегодня в #acc 5 часов #acc 59 минут' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        r = alice(voice('номер один'))
        assert r.scenario_stages() == {'run'}
        assert 'на #acc 0 часов #acc 15 минут' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_ask_time'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.experiments('hw_alarm_megamind_2906_fix')
    @pytest.mark.experiments('bg_fresh_granet_form=personal_assistant.scenarios.alarm_ask_time')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_set_on_15_minutes(self, alice):
        r = alice(voice('поставь будильники на 15 минут'))
        assert r.scenario_stages() == {'run'}
        assert 'номер 1. на завтра в #acc 0 часов #acc 15 минут' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert 'номер 2. на сегодня в #acc 5 часов #acc 59 минут' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        r = alice(voice('номер два'))
        assert r.scenario_stages() == {'run'}
        assert 'на #acc 5 часов #acc 59 минут' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_ask_time'
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_cancel')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_cancel_all_alarms(self, alice):
        r = alice(voice('поставь будильник по воскресеньям и субботам в 3:22'))
        r = alice(voice('поставь будильник по воскресеньям и субботам в 2:28'))

        r = alice(voice('отключи все будильники'))
        assert r.scenario_stages() == {'run'}
        assert 'выключила все будильники' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_cancel'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_cancel')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_cancel_alone(self, alice):
        r = alice(voice('поставь будильник по воскресеньям и субботам в 3:22'))

        r = alice(voice('отключи будильник'))
        assert r.scenario_stages() == {'run'}
        assert 'по выходным в #acc 3 часа #acc 22 минуты' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_cancel'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_cancel')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_cancel_one_of_two(self, alice):
        r = alice(voice('поставь будильник по воскресеньям и субботам в 3:22'))
        r = alice(voice('поставь будильник по воскресеньям и субботам в 2:28'))

        r = alice(voice('отключи будильник'))
        assert r.scenario_stages() == {'run'}
        assert 'по выходным в #acc 3 часа #acc 22 минуты' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert 'по выходным в #acc 2 часа #acc 28 минут' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_cancel'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        r = alice(voice('номер один'))
        assert r.scenario_stages() == {'run'}
        assert 'по выходным в #acc 3 часа #acc 22 минуты' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_cancel__ellipsis'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_cancel')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_cancel_no_alarms(self, alice):
        r = alice(voice('отключи все будильники'))

        assert r.scenario_stages() == {'run'}
        assert any(phrase in r.run_response.ResponseBody.Layout.OutputSpeech.lower() for phrase in [
            'у вас нет включенных будильников.',
            'будильников не обнаружено.'
        ])

        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_cancel'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_cancel')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_cancel_by_id(self, alice):
        hours = [4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14]
        for hour in hours:
            r = alice(voice(f'поставь будильник на {hour}'))

        r = alice(voice('удали будильник'))

        assert 'установлено несколько будильников' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert 'какой из них вы хотите выключить?' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()

        r = alice(voice('пятый'))
        assert 'выключила будильник на сегодня в #acc 8 часов утра' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()

        r = alice(voice('удали шестой будильник'))
        assert 'выключила будильник на сегодня в #acc 9 часов утра' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()

        return str(r)

    @pytest.mark.supported_features('set_timer', multiroom=None)
    def test_timer_cancel_noone(self, alice):
        r = alice(voice('удали таймер'))
        assert r.scenario_stages() == {'run'}
        assert any(phrase in r.run_response.ResponseBody.Layout.OutputSpeech.lower() for phrase in [
            'я ничего не засекала',
            'у вас нет установленных т+аймеров'
        ])

        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.timer_cancel'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'timer'

    @pytest.mark.supported_features('set_timer', multiroom=None)
    def test_timer_resume_noone(self, alice):
        r = alice(voice('сними таймер с паузы'))
        assert r.scenario_stages() == {'run'}
        assert any(phrase in r.run_response.ResponseBody.Layout.OutputSpeech.lower() for phrase in [
            'я ничего не засекала',
            'у вас нет установленных т+аймеров'
        ])

        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.timer_resume'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'timer'

    @pytest.mark.supported_features('set_timer', multiroom=None)
    def test_timer_pause_noone(self, alice):
        r = alice(voice('таймер на паузу'))
        assert r.scenario_stages() == {'run'}
        assert any(phrase in r.run_response.ResponseBody.Layout.OutputSpeech.lower() for phrase in [
            'я ничего не засекала',
            'у вас нет установленных т+аймеров'
        ])

        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.timer_pause'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'timer'

    @pytest.mark.supported_features('set_timer', multiroom=None)
    def test_timer_pause_and_resume(self, alice):
        r = alice(voice('таймер на 10 минут'))
        assert r.scenario_stages() == {'run'}
        assert any(phrase in r.run_response.ResponseBody.Layout.OutputSpeech.lower() for phrase in [
            '#nom 10 минут',
            '#acc 10 минут'
        ])

        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.timer_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'timer'

        assert len(r.run_response.ResponseBody.Layout.Directives) == 2
        assert r.run_response.ResponseBody.Layout.Directives[0].SetTimerDirective is not None
        assert r.run_response.ResponseBody.Layout.Directives[0].SetTimerDirective.Duration == 600
        assert len(r.run_response.ResponseBody.Layout.Directives[0].SetTimerDirective.Directives) == 0
        assert r.run_response.ResponseBody.Layout.Directives[1].TtsPlayPlaceholderDirective is not None

        r = alice(voice('поставь таймер на паузу'))
        assert r.scenario_stages() == {'run'}
        assert any(phrase in r.run_response.ResponseBody.Layout.OutputSpeech.lower() for phrase in [
            'поставила на паузу'
        ])

        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.timer_pause'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'timer'

        assert len(r.run_response.ResponseBody.Layout.Directives) == 2
        assert r.run_response.ResponseBody.Layout.Directives[0].PauseTimerDirective is not None
        assert r.run_response.ResponseBody.Layout.Directives[1].TtsPlayPlaceholderDirective is not None
        timer_id = r.run_response.ResponseBody.Layout.Directives[0].PauseTimerDirective.TimerId

        r = alice(voice('поставь таймер на паузу'))
        assert r.scenario_stages() == {'run'}
        assert any(phrase in r.run_response.ResponseBody.Layout.OutputSpeech.lower() for phrase in [
            'т+аймер уже остановлен'
        ])

        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.timer_pause'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'timer'
        assert len(r.run_response.ResponseBody.Layout.Directives) == 0

        r = alice(voice('сними таймер с паузы'))
        assert r.scenario_stages() == {'run'}
        assert any(phrase in r.run_response.ResponseBody.Layout.OutputSpeech.lower() for phrase in [
            'продолжаю',
            'сняла с паузы'
        ])

        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.timer_resume'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'timer'

        assert len(r.run_response.ResponseBody.Layout.Directives) == 2
        assert r.run_response.ResponseBody.Layout.Directives[0].ResumeTimerDirective is not None
        assert r.run_response.ResponseBody.Layout.Directives[0].ResumeTimerDirective.TimerId == timer_id
        assert r.run_response.ResponseBody.Layout.Directives[1].TtsPlayPlaceholderDirective is not None

        r = alice(voice('сними таймер с паузы'))
        assert r.scenario_stages() == {'run'}
        assert any(phrase in r.run_response.ResponseBody.Layout.OutputSpeech.lower() for phrase in [
            'т+аймер уже запущен'
        ])

        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.timer_resume'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'timer'
        assert len(r.run_response.ResponseBody.Layout.Directives) == 0


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='Alarm', handle='alarm')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestsOnSearchapp:
    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_set_on_searchapp(self, alice):
        r = alice(voice('поставь будильник на 8'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == ''

        assert len(layout.Directives) == 1
        directive = layout.Directives[0].AlarmNewDirective

        on_success_callback = _parse_callback(directive.OnSuccessCallbackPayload)

        r = alice(on_success_callback)
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert '#acc 8 часов' in layout.OutputSpeech
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.supported_features('set_alarm', multiroom=None)
    def test_alarm_ask_time_on_searchapp(self, alice):
        r = alice(voice('поставь будильник'))
        assert r.scenario_stages() == {'run'}
        assert 'на какое время' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        r = alice(voice('на 8'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.alarm_ask_time'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'alarm'

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == ''

        assert len(layout.Directives) == 1
        directive = layout.Directives[0].AlarmNewDirective

        on_success_callback = _parse_callback(directive.OnSuccessCallbackPayload)

        r = alice(on_success_callback)
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert '#acc 8 часов' in layout.OutputSpeech
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__timer_set')
    @pytest.mark.supported_features('set_timer', multiroom=None)
    def test_timer_set_on_searchapp(self, alice):
        r = alice(voice('поставь таймер на 10 минут'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.timer_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'timer'

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == ''

        assert len(layout.Directives) == 1
        directive = layout.Directives[0].SetTimerDirective

        on_success_callback = _parse_callback(directive.OnSuccessCallbackPayload)

        r = alice(on_success_callback)
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert '#nom 10 минут' in layout.OutputSpeech
        return str(r)

    @pytest.mark.experiments('hw_alarm_relocation_exp__timer_set')
    @pytest.mark.supported_features('set_timer', multiroom=None)
    def test_timer_ask_time_on_searchapp(self, alice):
        r = alice(voice('поставь таймер'))
        assert r.scenario_stages() == {'run'}
        assert 'на какое время' in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.timer_set'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'timer'

        r = alice(voice('на 10 минут'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.AnalyticsInfo.Intent == 'personal_assistant.scenarios.timer_set__ellipsis'
        assert r.run_response.ResponseBody.AnalyticsInfo.ProductScenarioName == 'timer'

        layout = r.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == ''

        assert len(layout.Directives) == 1
        directive = layout.Directives[0].SetTimerDirective

        on_success_callback = _parse_callback(directive.OnSuccessCallbackPayload)

        r = alice(on_success_callback)
        assert r.scenario_stages() == {'run'}
        layout = r.run_response.ResponseBody.Layout
        assert '#nom 10 минут' in layout.OutputSpeech
        return str(r)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='Alarm', handle='alarm')
@pytest.mark.parametrize('surface', [surface.webtouch])
class TestsShowPromo:

    @pytest.mark.experiments('hw_alarm_morning_show_exp')
    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_set')
    @pytest.mark.experiments('hw_alarm_relocation_exp__alarm_cancel')
    @pytest.mark.supported_features('change_alarm_sound')
    @pytest.mark.parametrize('command', [
        pytest.param('поставь утреннее шоу на будильник', id="morning_show"),
        pytest.param('поставь будильник на 8:40', id="single_on_time"),
        pytest.param('поставь будильник на 8 вечера', id="single_on_time_with_day_part"),
        pytest.param('поставь будильник по понедельникам в 13:37', id="regular_on_weekday"),
        pytest.param('поставь будильник по средам, вторникам и пятницам в 2:28', id="regular_on_many_weekdays"),
        pytest.param('поставь будильник ежедневно в 4:20', id="regular_on_all_days"),
        pytest.param('поставь будильник по будням в 3:22', id="regular_on_weekdays"),
        pytest.param('поставь будильник по воскресеньям и субботам в 3:22', id="regular_on_weekends"),
        pytest.param('поставь будильник', id="time_not_stated"),
        pytest.param('поставь будильник утром', id="day_part"),
        pytest.param('поставь будильник на сегодня', id="day_time_not_stated"),
        pytest.param('разбуди через пятнадцать минут', id="snooze_rel")
    ])
    def test_alarm(self, alice, command):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech
        assert r.run_response.ResponseBody.Layout.Directives[0].ShowPromoDirective
        assert r.run_response.ResponseBody.AnalyticsInfo.Objects[0].DummyResponse.Reason == TResponse.EReason.SurfaceInability

    @pytest.mark.parametrize('command', [pytest.param('поставь таймер на 10 минут', id="simple")])
    @pytest.mark.experiments('hw_alarm_relocation_exp__timer_set')
    def test_timer(self, alice, command):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech
        assert r.run_response.ResponseBody.Layout.Directives[0].ShowPromoDirective
        assert r.run_response.ResponseBody.AnalyticsInfo.Objects[0].DummyResponse.Reason == TResponse.EReason.SurfaceInability
