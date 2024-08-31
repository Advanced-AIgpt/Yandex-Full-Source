import datetime

import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.surface as surface
import pytest
import pytz


def _get_hours_minutes_string(time):
    return f'{time.hour}:{time.strftime("%M")}'


@pytest.mark.parametrize('surface', [surface.watch])
class TestPalmContextSaving(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-45
    """

    owners = ('g-kostin',)

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-583#602ece193ecb4c692d71ae7b')
    @pytest.mark.region(region.Moscow)
    def test_alice_45(self, alice):
        response = alice('Где я?')
        assert response.intent == intent.GetMyLocation
        assert 'Москва' in response.text

        moscow_tz = pytz.timezone('Europe/Moscow')
        moscow_before = _get_hours_minutes_string(datetime.datetime.now(moscow_tz))
        response = alice('Сколько времени?')
        moscow_after = _get_hours_minutes_string(datetime.datetime.now(moscow_tz))
        assert response.intent == intent.GetTime
        assert (moscow_before in response.text) or (moscow_after in response.text)

        london_tz = pytz.timezone('Europe/London')
        moscow_before = _get_hours_minutes_string(datetime.datetime.now(london_tz))
        response = alice('А в Лондоне?')
        moscow_after = _get_hours_minutes_string(datetime.datetime.now(london_tz))
        assert response.intent == intent.GetTimeEllipsis
        assert (moscow_before in response.text) or (moscow_after in response.text)

        response = alice('Какая погода?')
        assert response.intent == intent.GetWeather
        assert 'Лондон' in response.text
