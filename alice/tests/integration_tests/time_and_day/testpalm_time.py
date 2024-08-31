import re

import alice.tests.library.intent as intent
import alice.tests.library.locale as locale
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from .common import DateTime, Time, DateTimeRange


def _assert_current_time(text, time_before, time_after):
    # matches 'Сейчас 21:34.'
    m = re.search(r'^(Сейчас )?(?P<hour>[\d]{1,2})(:(?P<minute>\d{2}))?.*?\.$', text)
    response_time = Time(m['hour'], m['minute'])
    if response_time.minute == 0 and response_time.hour >= 13:
        response_time.hour -= 12
    assert response_time in DateTimeRange(time_before, time_after)


def _assert_city_time(text, city, timezone, time_before, time_after):
    # matches 'Сейчас в Верхоянске 21:34, пятница, 3 апреля.'
    # matches 'В Красноярске 11 утра, воскресенье, 12 июля.'
    m = re.search(r'^(Сейчас в|В) ' + city + r'е (?P<hour>\d{1,2})(:(?P<minute>\d{2}))?.*?, (?P<weekday>.+), (?P<day>\d+) (?P<month>.+)\.$', text)
    response_time = DateTime(
        m['weekday'], m['month'], m['day'], m['hour'], m['minute'],
    )
    assert response_time in DateTimeRange(time_before, time_after, timezone)


@pytest.mark.region(region.Moscow)
class _TestPalmTimeBase(object):
    owners = ('sparkle',)


@pytest.mark.voice
@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestPalmTime(_TestPalmTimeBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-607
    """

    @pytest.mark.parametrize('command', [
        'сколько время?',
        'сколько времени',
        'который час?',
        'время сколько?',
        'местное время',
        'точное время',
    ])
    def test_current_time_transition(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.GetTime
        _assert_current_time(response.text, *alice.request_time)

        city, timezone = ('Верхоянск', 'Asia/Vladivostok')
        response = alice(f'А в {city}е?')
        assert response.intent == intent.GetTimeEllipsis
        _assert_city_time(response.text, city, timezone, *alice.request_time)

    @pytest.mark.parametrize('command', ['время в ', 'точное время в '])
    @pytest.mark.parametrize('city, timezone', [
        ('Калининград', 'Europe/Kaliningrad'),
        ('Краснодар', 'Europe/Moscow'),
        ('Саратов', 'Europe/Samara'),
        ('Екатеринбург', 'Asia/Yekaterinburg'),
        ('Омск', 'Asia/Omsk'),
        ('Красноярск', 'Asia/Krasnoyarsk'),
        ('Иркутск', 'Asia/Irkutsk'),
        ('Якутск', 'Asia/Yakutsk'),
        ('Верхоянск', 'Asia/Vladivostok'),
        ('Вилючинск', 'Asia/Kamchatka'),
    ])
    def test_exact_time_in_city(self, alice, command, city, timezone):
        response = alice(f'{command} {city}е')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.GetTime
        _assert_city_time(response.text, city, timezone, *alice.request_time)


@pytest.mark.surface([surface.loudspeaker])
@pytest.mark.locale(locale.actual_locales)
class TestPalmTimeText(_TestPalmTimeBase):
    @pytest.mark.parametrize('command', [
        'сколько время?',
        'сколько времени',
        'который час?',
        'время сколько?',
        'местное время',
        'точное время',
    ])
    def test_current_time_transition(self, alice, command):
        response = alice(command)
        assert response.intent == intent.GetTime
        assert response.scenario in [scenario.GetTime, scenario.Vins]

        if locale.is_ru(alice):
            _assert_current_time(response.text, *alice.request_time)

            city, timezone = ('Верхоянск', 'Asia/Vladivostok')
            response = alice(f'А в {city}е?')
            assert response.intent == intent.GetTimeEllipsis
            _assert_city_time(response.text, city, timezone, *alice.request_time)
