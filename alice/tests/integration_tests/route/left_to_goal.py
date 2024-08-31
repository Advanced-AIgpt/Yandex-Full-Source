import time

import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [surface.navi])
class TestLeftToGoal(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1187
    """

    owners = ('isiv',)
    commands = [
        'Сколько мне еще ехать',
        'Сколько мне осталось ехать',
        'Сколько осталось',
        'Сколько до конца маршрута',
        'Сколько осталось маршрута',
        'Сколько километров до финиша',
        'Сколько до финиша',
        'Когда приеду',
        'Сколько еще км',
        'Когда я доеду',
        'Уже приехали?',
        'Долго еще?',
        'Когда уже?',
        'А теперь приехали?',
        'Далеко еще?',
        'Когда доедем?',
    ]

    @pytest.mark.device_state(navigator={
        'current_route': {
            'arrival_timestamp': round(time.time())+25*60,
            'distance_in_traffic_jam': 0,
            'distance_to_destination': 4*1000,
            'raw_time_to_destination': 25*60,
            'time_in_traffic_jam': 0,
            'time_to_destination': 25*60
        }
    })
    @pytest.mark.parametrize('command', commands)
    def test_show_left_to_far_point(self, alice, command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.HowLongToDrive
        assert response.text.startswith((
            'Ехать ещё', 'Осталось', 'До конца маршрута',
        ))
        assert '4 километра' in response.text
        assert '25 минут' in response.text

    @pytest.mark.device_state(navigator={
        'current_route': {
            'arrival_timestamp': round(time.time())+40,
            'distance_in_traffic_jam': 0,
            'distance_to_destination': 200,
            'raw_time_to_destination': 45,
            'time_in_traffic_jam': 0,
            'time_to_destination': 40
        }
    })
    @pytest.mark.parametrize('command', commands)
    def test_show_left_to_near_point(self, alice, command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.HowLongToDrive
        assert response.text in ['Ещё чуть-чуть.', 'Почти приехали.']

    @pytest.mark.device_state(navigator={
        'current_route': {
            'arrival_timestamp': round(time.time())+13*60,
            'distance_in_traffic_jam': 0,
            'distance_to_destination': 2*1000,
            'raw_time_to_destination': 13*60,
            'time_in_traffic_jam': 0,
            'time_to_destination': 12*60
        }
    })
    @pytest.mark.parametrize('command', commands)
    def test_clear_route(self, alice, command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.HowLongToDrive
        assert response.text.startswith((
            'Ехать ещё', 'Осталось', 'До конца маршрута',
        ))
        assert '2 километра' in response.text
        assert '12 минут' in response.text

        response = alice('Удалить маршрут')
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ResetRoute

        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert response.text in ['Куда нужно добраться?', 'Куда?']
