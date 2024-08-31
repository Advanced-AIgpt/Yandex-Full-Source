import datetime
import re
import time

import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest
import pytz

from . import CoordEps


@pytest.mark.region(region.Moscow)
class TestPalmRoute(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1189
    https://testpalm.yandex-team.ru/testcase/alice-1222
    https://testpalm.yandex-team.ru/testcase/alice-1579
    https://testpalm.yandex-team.ru/testcase/alice-1585
    https://testpalm.yandex-team.ru/testcase/alice-1980
    """

    owners = ('isiv',)
    device_state = {
        'navigator': {
            'current_route': {
                'arrival_timestamp': round(time.time()) + 1443,
                'distance_in_traffic_jam': 0,
                'distance_to_destination': 11827.91699,
                'points': [
                    {
                        'lat': 55.88913727,
                        'lon': 37.65043259
                    },
                    {
                        'lat': 55.83304596,
                        'lon': 37.59512329
                    },
                ],
                'raw_time_to_destination': 1588,
                'time_in_traffic_jam': 0,
                'time_to_destination': 1443,
            }
        }
    }

    _on_route_expected_phrases = (
        'Приедем в ',
        'Будем на месте в ',
        'В ',
        'Расчётное время прибытия ',
    )
    _no_route_expected_phrases = [
        'Куда нужно добраться?',
        'Куда?',
    ]
    _reset_route_expected_phrases = [
        'Сбрасываю маршрут.',
        'Сбросила маршрут.',
    ]
    _moscow_tz = pytz.timezone('Europe/Moscow')

    @pytest.mark.parametrize('command', [
        'Во сколько приеду',
        'Во сколько буду',
        'Во сколько приедем',
    ])
    @pytest.mark.parametrize('surface', [surface.automotive, surface.navi])
    def test_alice_1189_1579(self, alice, command):
        response = alice(command)

        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.WhenWeGetThere
        assert response.text.startswith(self._on_route_expected_phrases)

        ts = alice.device_state.Navigator.CurrentRoute.ArrivalTimestamp
        arrival_date = datetime.datetime.fromtimestamp(ts, self._moscow_tz)

        assert response.text.startswith(('Расчётное время прибытия ', 'В ', 'Приедем в ', 'Будем на месте в '))
        # There is a special mode with answers like 'Расчётное время прибытия 6 вечера.', 'полночь' etc.
        # This mode triggers sometimes if minutes are 0
        if arrival_date.minute:
            arrival_minutes = arrival_date.strftime('%M')
            assert f'{arrival_date.hour}:{arrival_minutes}' in response.text
        else:
            part_day = 'вечера' if arrival_date.hour > 12 else 'утра'
            part_hour = arrival_date.hour % 12
            assert f'{part_hour} {part_day}' in response.text

        alice.reset_route()

        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert response.text in self._no_route_expected_phrases

    @pytest.mark.parametrize('add_command', [
        'Отметь',
        'добавь',
        'укажи',
        'установи',
        'зафиксируй',
    ])
    @pytest.mark.parametrize('point', [
        'точку',
        'событие',
        'метку',
    ])
    @pytest.mark.parametrize('surface', [surface.automotive, surface.navi])
    def test_alice_1117_1597(self, alice, add_command, point):
        response = alice(f'{add_command} {point}')
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.AddPoint
        assert response.text in [
            'Что именно? Авария, разговорчик, камера или что-то другое?',
            'Что именно? ДТП, ремонт дороги, ошибка или что-то ещё?'
        ]
        assert not response.directive

        response = alice('Хватит')
        assert response.scenario == scenario.Commands
        assert response.intent == intent.PlayerPause

    @pytest.mark.parametrize('command', [
        'сбрось маршрут',
        'отмени маршрут',
        'отменить маршрут',
        'сбросить маршрут',
        'отмена маршрута',
        'закрыть маршрут',
        'удалить маршрут',
        'остановить маршрут',
        'отменить текущий маршрут',
        'прекратить ведение маршрута',
        'сбрось ведение маршрута',
    ])
    @pytest.mark.parametrize('surface', [surface.automotive, surface.navi])
    def test_alice_1222_1585(self, alice, command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ResetRoute
        assert any(phrase in response.text for phrase in self._reset_route_expected_phrases)
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexnavi://clear_route'

        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ResetRoute
        assert response.text == 'Так вроде никуда не едем.'

    @pytest.mark.parametrize('surface', [surface.navi])
    def test_alice_1980(self, alice):
        response = alice('Сколько ехать до Питера?')
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert re.search(
            r'^.+ с учетом пробок. Это путь до адреса Санкт-Петербург.$',
            response.text
        )


@pytest.mark.parametrize('surface', [surface.automotive, surface.navi])
class TestPalmParkingRoute(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1402
    https://testpalm.yandex-team.ru/testcase/alice-1569
    """

    owners = ('isiv',)

    @pytest.mark.parametrize('command', [
        'Найди парковку',
        'Ищи парковку',
        'Поиск парковок',
        'Где парковка',
        'Парковочный маршрут',
        'Маршрут до парковки',
        'Давай поищем парковку',
        'Где бросить машину',
        'Где бросить тачку',
        'Где оставить машину',
        'Где оставить тачку',
        'Построй маршрут до парковки',
        'Построй маршрут до ближайшей парковки',
        'Поехали на парковку',
    ])
    def test_parking_route(self, alice, command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ParkingRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexnavi://carparks_route'


@pytest.mark.region(region.Moscow)
class TestRoutesConfirm(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1079
    https://testpalm.yandex-team.ru/testcase/alice-1600
    Частично https://testpalm.yandex-team.ru/testcase/alice-1802
    """

    owners = ('isiv',)

    @pytest.mark.parametrize('surface', [surface.automotive, surface.navi])
    @pytest.mark.parametrize('command, lat, lon', [
        ('Поехали на улицу Широкая, дом 1, корпус 1', 55.8884, 37.6494),
        ('Поехали на улицу Широкая, дом один, корпус один', 55.8884, 37.6494),
    ])
    def test_routes_confirm(self, alice, command, lat, lon):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://build_route_on_map?confirmation=1' in response.directive.payload.uri
        assert f'&lat_to={lat}' in response.directive.payload.uri
        assert f'&lon_to={lon}' in response.directive.payload.uri

        response = alice('Отменить')
        assert response.scenario == scenario.Commands
        assert response.intent == intent.PlayerPause
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://external_confirmation?confirmed=0' in response.directive.payload.uri

        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://build_route_on_map?confirmation=1' in response.directive.payload.uri

        response = alice('Поехали')
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://external_confirmation?confirmed=1' in response.directive.payload.uri

    @pytest.mark.parametrize('surface', [surface.old_automotive])
    @pytest.mark.parametrize('command, lat, lon', [
        ('Поехали на улицу Широкая, дом 1, корпус 1', 55.8884, 37.6494),
        ('Поехали на улицу Широкая, дом один, корпус один', 55.8884, 37.6494),
    ])
    def test_alice_1802(self, alice, command, lat, lon):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.YandexNaviDirective
        assert response.directive.payload.application == 'yandexnavi'
        assert response.directive.payload.intent == 'build_route_on_map'
        assert response.directive.payload.params.confirmation == '1'
        assert abs(response.directive.payload.params.lat_to - lat) < CoordEps
        assert abs(response.directive.payload.params.lon_to - lon) < CoordEps

        response = alice('Отменить')
        assert response.scenario in {scenario.Vins, scenario.Commands}
        assert response.intent in {intent.AutoAppConfirmationNo, intent.PlayerPause}
        assert response.directive.name == directives.names.YandexNaviDirective
        assert response.directive.payload.application == 'yandexnavi'
        assert response.directive.payload.intent == 'external_confirmation'
        assert response.directive.payload.params.app == 'yandexnavi'
        assert response.directive.payload.params.confirmed == '0'

        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.YandexNaviDirective
        assert response.directive.payload.application == 'yandexnavi'
        assert response.directive.payload.intent == 'build_route_on_map'

        response = alice('Поехали')
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.AutoAppConfirmationYes
        assert response.directive.name == directives.names.YandexNaviDirective
        assert response.directive.payload.application == 'yandexnavi'
        assert response.directive.payload.intent == 'external_confirmation'
        assert response.directive.payload.params.app == 'yandexnavi'
        assert response.directive.payload.params.confirmed == '1'
