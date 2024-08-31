import datetime
import re
from dateutil.relativedelta import relativedelta

import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from .common import Date, DateTimeRange, Months, WeekdaysIndex

#
# Note the current version supports both VINS and HOLLYWOOD scenarios.
# Both scenarios work well but have some minor differences in answers.
# Vins version will be removed after https://st.yandex-team.ru/HOLLYWOOD-522 will be done
#


@pytest.mark.voice
@pytest.mark.region(region.Moscow)
# TODO: Need to remove this experiment later (also - remove all VINS-related checks below)
@pytest.mark.experiments('mm_enable_protocol_scenario=GetDate')
@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestPalmDay(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-13
    https://testpalm.yandex-team.ru/testcase/alice-1086
    https://testpalm.yandex-team.ru/testcase/alice-1517
    https://testpalm.yandex-team.ru/testcase/alice-1564
    https://testpalm.yandex-team.ru/testcase/alice-2160
    """

    owners = ('d-dima',)

    @pytest.mark.parametrize('city, timezone', [
        ('в Москве', None),
        pytest.param(
            'во Владивостоке',
            'Asia/Vladivostok',
            marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/HOLLYWOOD-522')
        )
    ])
    def test_what_day_is_today(self, alice, city, timezone):
        where = f' {city}' if timezone else ''
        response = alice(f'какой сегодня день{where}?')
        # TODO: Need to remove scenario.Vins
        assert response.scenario in [scenario.Vins, scenario.GetDate]
        assert response.intent == intent.GetDate

        m = re.match(rf'(Сегодня( {city})? )?(?P<weekday>.+), (?P<day>\d+) (?P<month>.+)\.$', response.text)
        assert m, f'No match in response "{response.text}"'
        response_time = Date(m['weekday'], m['month'], m['day'])
        assert response_time in DateTimeRange(*alice.request_time, timezone)
        assert m['day'] in response.output_speech_text
        assert m['month'] in response.output_speech_text
        assert m['weekday'] in response.output_speech_text

    def test_what_day_yes_or_no(self, alice):
        city, timezone = ('в Токио', 'Asia/Tokyo')
        response = alice(f'Сегодня {city} понедельник или вторник')
        # TODO: Need to remove scenario.Vins
        assert response.scenario in [scenario.Vins, scenario.GetDate]
        assert response.intent == intent.GetDate
        if response.scenario == scenario.Vins:
            m = re.match(rf'(Да|Нет), (сегодня {city} )?(?P<weekday>\w+), (?P<day>\d+) (?P<month>.+)\.$', response.text)
            assert m, f'No match in response "{response.text}"'
            response_time = Date(m['weekday'], m['month'], m['day'])
            assert response_time in DateTimeRange(*alice.request_time, timezone)
            assert m['day'] in response.output_speech_text
            assert m['month'] in response.output_speech_text
        else:
            m = re.match(rf'(Да|Нет). Сегодня {city} (?P<weekday>\w+)\.$', response.text)
            assert m, f'No match in response "{response.text}"'
        assert m['weekday'] in response.output_speech_text

    @pytest.mark.parametrize('command, weekday', [
        ('Понедельник это какое число?', 'понедельник'),
        ('Четверг это 25 число или нет', 'четверг'),
    ])
    def test_what_day_by_weekday(self, alice, command, weekday):
        response = alice(command)
        # TODO: Need to remove scenario.Vins
        assert response.scenario in [scenario.Vins, scenario.GetDate]
        assert response.intent == intent.GetDate

        shift = WeekdaysIndex[weekday] - alice.datetime_now.weekday()
        if shift <= 0:
            shift += 7
        target_day = alice.datetime_now + relativedelta(days=shift)

        # matches 'В понедельник будет 28 мая' or '28 мая.'
        if response.scenario == scenario.Vins:
            m = re.match(rf'(В {weekday} будет )?(?P<day>\d+) (?P<month>.+)\.$', response.text)
        else:
            # Ответ Hollywood на первый вопрос будет простым, на второй вопрос - с дополнительным Да|Нет
            m = re.match(rf'(В {weekday}( в Москве)? будет )?(?P<day>\d+) (?P<month>.+)\.$', response.text)
            if not m:
                m = re.match(rf'(Да|Нет). (В {weekday}( в Москве)?( будет)? )?(?P<day>\d+) (?P<month>.+)\.$', response.text)
        assert m, f'No match in response "{response.text}"'
        response_time = Date(weekday, m['month'], m['day'])
        assert response_time.weekday == target_day.weekday()
        # TODO [DD] Temporary blocked: request doesn't work in Thursday!
        # if response_time.day and response_time.month:
        #    assert response_time == target_day
        # assert m['day'] in response.output_speech_text
        # assert m['month'] in response.output_speech_text

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-7257')
    def test_what_day_is_date_year_shift(self, alice):
        response = alice('Какой день будет 13 октября через пять лет?')
        # TODO: Need to remove scenario.Vins
        assert response.scenario in [scenario.Vins, scenario.GetDate]
        assert response.intent == intent.GetDate

        target_date = datetime.date(alice.datetime_now.year + 5, 10, 13)
        m = re.match(r'(Через 5 лет 13 октября будет )?(?P<weekday>.+), (?P<day>\d+) (?P<month>.+) (?P<year>.+) года\.', response.text)
        assert m, f'No match in response "{response.text}"'
        response_time = Date(m['weekday'], m['month'], m['day'])
        assert response_time == target_date
        assert m['day'] in response.output_speech_text
        assert m['month'] in response.output_speech_text
        assert m['weekday'] in response.output_speech_text

    def test_date_illegal_question(self, alice):
        response = alice('Сегодня сегодня')
        assert response.scenario != scenario.GetDate

    def test_what_weekday_in_past(self, alice):
        response = alice('6 декабря 1985 года это какой день недели?')
        # TODO: Need to remove scenario.Vins
        assert response.scenario in [scenario.Vins, scenario.GetDate]
        assert response.intent == intent.GetDate
        assert response.text in ['6 декабря 1985 года была пятница.', '6 декабря 1985 года в Москве была пятница.', 'Пятница.']
        assert 'пятница' in response.output_speech_text.lower()

    @pytest.mark.parametrize('command, shift, answer', [
        ('Какой день будет завтра в Москве?', relativedelta(days=1), 'Завтра в Москве будет'),
        ('Какой день будет через неделю в Москве?', relativedelta(days=7), 'Через неделю в Москве будет'),
        ('Какой день будет в Москве через год?', relativedelta(years=1), 'Через год в Москве будет'),
        pytest.param(
            'Какой день  в Москве будет через 99 лет?',
            relativedelta(years=99),
            'Через 99 лет в Москве будет',
            marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-7257')
        ),
        pytest.param(
            ' в Москве Какой день будет через одиннадцать месяцев?',
            relativedelta(months=11),
            'Через одиннацать месяцев в Москве будет',
            marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-10565')
        ),
        ('Какой день был три недели назад в Москве?', relativedelta(days=-21), '3 недели назад в Москве был.?'),
    ])
    def test_what_day_is_with_shift(self, alice, command, shift, answer):
        response = alice(command)
        # TODO: Need to remove scenario.Vins
        assert response.scenario in [scenario.Vins, scenario.GetDate]
        assert response.intent == intent.GetDate

        target_date = alice.datetime_now + shift
        # year is optional (in case if year in answer is different with current)
        # "'Через неделю в Москве будет' вторник, 7 декабря."
        # "'Через год в Москве будет' среда, 30 ноября 2022 года."

        if response.scenario == scenario.Vins:
            m = re.match(rf'({answer} )?(?P<weekday>.+), (?P<day>\d+) (?P<month>.+)\.', response.text)
        else:
            # HW scenario add 'YEAR' in case if it not equal to current
            m = re.match(rf'({answer} )?(?P<weekday>.+), (?P<day>\d+) (?P<month>.+) (?P<year>.+) года\.', response.text)
            if not m:
                m = re.match(rf'({answer} )?(?P<weekday>.+), (?P<day>\d+) (?P<month>.+)\.', response.text)
        assert m, f'No match in response "{response.text}"'
        response_time = Date(m['weekday'], m['month'], m['day'])
        assert response_time == target_date

    def test_what_day_is_after_some_days(self, alice):
        target_day = alice.datetime_now + datetime.timedelta(days=5)
        # 'какой день недели будет 28 мая?'
        response = alice(f'какой день недели будет {target_day.day} {Months[target_day.month - 1]}?')
        # TODO: Need to remove scenario.Vins
        assert response.scenario in [scenario.Vins, scenario.GetDate]
        assert response.intent == intent.GetDate

        # matches '28 мая в Москве будет пятница.' or 'Пятница.'
        m = re.match(r'((?P<day>\d+) (?P<month>\S+)( в Москве)? будет )?(?P<weekday>.*)\.$', response.text)
        assert m, f'No match in response "{response.text}"'
        response_time = Date(m['weekday'], m['month'], m['day'])
        if response_time.day and response_time.month:
            assert response_time == target_day
        assert response_time.weekday == target_day.weekday()

    def test_what_year(self, alice):
        response = alice('какой сегодня год')
        # TODO: Need to remove scenario.Vins
        assert response.scenario in [scenario.Vins, scenario.GetDate]
        assert response.intent == intent.GetDate

        # итоговый ответ должен содержать номер текущего года
        assert str(alice.datetime_now.year) in response.text
