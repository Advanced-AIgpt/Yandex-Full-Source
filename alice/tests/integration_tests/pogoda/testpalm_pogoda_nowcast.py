import json

import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest

from .common import PogodaDialogBase, get_day


class TestPalmPogodaNowcastMisc(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1186
    https://testpalm.yandex-team.ru/testcase/alice-1259
    https://testpalm.yandex-team.ru/testcase/alice-1493

    https://testpalm.yandex-team.ru/testcase/alice-1176
    https://testpalm.yandex-team.ru/testcase/alice-1257
    https://testpalm.yandex-team.ru/testcase/alice-1495

    https://testpalm.yandex-team.ru/testcase/alice-1254
    https://testpalm.yandex-team.ru/testcase/alice-1498

    https://testpalm.yandex-team.ru/testcase/alice-1173
    https://testpalm.yandex-team.ru/testcase/alice-1255
    https://testpalm.yandex-team.ru/testcase/alice-1497
    """

    commands = [
        # block 1
        # ¯\_(ツ)_/¯ Не используем одиночные слова, не попадают в NLU - https://st.yandex-team.ru/DIALOG-3565#5c48d9f92f38ef001f5c1ead
        # ('Осадки', False),
        # ('Снег', False),
        # ('Дождь со снегом', False),
        ('Когда будет снег', False),
        ('Когда выпадет снег', False),
        ('Осадки будут', False),
        ('Осадки ожидаются', False),

        # block 2
        ('Закончится ли дождь через 10 минут?', True),
        ('Дождь через час?', True),
        ('Будет ли дождь через 2 часа?', True),
        ('Закончится ли дождь через 3 секунды', True),

        # block 3
        ('Будет ли дождь сегодня?', False),
        # ('Дождь пойдёт?', False),
        ('Зонт брать?', False),
        ('Зонтик нужен сегодня?', False),

        # block 4
        ('Будет ли дождь утром?', True),
        ('Что с дождем утром?', True),
        ('Дождь будет к обеду', True),
        ('Будет ли дождь днем?', True),
        ('Закончится ли дождь вечером?', True),
        ('Будет ли дождь сегодня вечером?', True),
    ]

    @pytest.mark.parametrize('command, can_swap_to_get_weather', commands)
    @pytest.mark.parametrize('surface', [surface.station])
    def test_pogoda_nowcast_misc_station(self, alice, command, can_swap_to_get_weather):
        response = alice(command)
        if can_swap_to_get_weather and response.intent == intent.GetWeather:
            return
        assert response.intent == intent.GetWeatherNowcast
        assert response.text


@pytest.mark.parametrize('surface', [
    surface.navi,
    surface.searchapp,
    surface.station,
    surface.yabro_win,
])
class TestPalmPogodaNowcastContext(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1192
    https://testpalm.yandex-team.ru/testcase/alice-1258
    https://testpalm.yandex-team.ru/testcase/alice-1494
    https://testpalm.yandex-team.ru/testcase/alice-2072
    """

    @staticmethod
    def _assert_city(slots, city):
        forecast_location = json.loads(slots['forecast_location'].string)
        assert forecast_location['city'] == city

    def test_pogoda_nowcast_context_change_city(self, alice):
        response = alice('Будет сегодня дождь?')
        assert response.intent == intent.GetWeatherNowcast
        self._assert_city(response.slots, 'Москва')

        response = alice('А в Архангельске?')
        assert response.intent == intent.GetWeatherNowcastEllipsis
        self._assert_city(response.slots, 'Архангельск')

        response = alice('А вечером?')
        assert response.intent in {intent.GetWeatherNowcastEllipsis, intent.GetWeather}
        self._assert_city(response.slots, 'Архангельск')

    def test_pogoda_nowcast_context_change_day_part(self, alice):
        response = alice('Сегодня пойдет снег?')
        assert response.intent == intent.GetWeatherNowcast
        self._assert_city(response.slots, 'Москва')

        response = alice('А вечером будет?')
        assert response.intent in {intent.GetWeatherNowcastEllipsis, intent.GetWeather}
        self._assert_city(response.slots, 'Москва')

    def test_pogoda_nowcast_context_change_intent(self, alice):
        response = alice('Будут сегодня осадки?')
        assert response.intent == intent.GetWeatherNowcast
        self._assert_city(response.slots, 'Москва')

        response = alice('А завтра?')
        assert response.intent == intent.GetWeather
        self._assert_city(response.slots, 'Москва')


@pytest.mark.parametrize('surface', [
    surface.searchapp,
    surface.station,
    surface.navi,
])
class TestPalmPogodaNowcastNotToday(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1174
    https://testpalm.yandex-team.ru/testcase/alice-1260
    https://testpalm.yandex-team.ru/testcase/alice-1492
    """

    @pytest.mark.parametrize('command', [
        'Будет дождь завтра?',
        'Будет ли дождь завтра утром?',
        f'{get_day(delta=3)} будет дождь?',
        'Пойдет ли дождь послезавтра в Питере?',
    ])
    def test_pogoda_nowcast_not_today(self, alice, command):
        response = alice(command)
        assert response.intent == intent.GetWeather  # меняем на GetWeather если спрашиваем не про сегодняшний день
