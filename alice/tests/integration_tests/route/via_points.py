import re
import time
from urllib.parse import unquote

import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [surface.automotive, surface.navi])
class TestViaPoints(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1077
    https://testpalm.yandex-team.ru/testcase/alice-1460
    https://testpalm.yandex-team.ru/testcase/alice-1461
    https://testpalm.yandex-team.ru/testcase/alice-1462
    https://testpalm.yandex-team.ru/testcase/alice-1578
    https://testpalm.yandex-team.ru/testcase/alice-1582
    """

    owners = ('isiv',)
    _STATE_ROUTE_FROM_KR_TO_AURORA = {
        'current_route': {
            'arrival_timestamp': round(time.time())+17*60,
            'distance_in_traffic_jam': 0,
            'distance_to_destination': 3*1000,
            'points': [
                {
                    'lat': 55.7341,
                    'lon': 37.5896
                },
                {
                    'lat': 55.734427,
                    'lon': 37.641852
                },
            ],
            'raw_time_to_destination': 17*60,
            'time_in_traffic_jam': 0,
            'time_to_destination': 17*60,
        }
    }

    @pytest.mark.parametrize('command, goal, via', [
        ('до Зеленограда по Кутузовскому проспекту', 'до Зеленограда', 'через адрес Кутузовский проспект'),
        ('Поехали в МГТУ им. Н.Э.Баумана с заездом на улицу Ботаническая, дом 27',
         r'до "МГТУ им. Н.( )?Э. Баумана" по адресу 2-я Бауманская улица 5', 'через адрес Ботаническая улица 27'),
        ('Поехали сначала на улицу Строителей, дом 5, корпус 3, а потом в МакДональдс',
         'до "Макдоналдс" по адресу улица Вавилова 66', 'через адрес улица Строителей 5к3'),
    ])
    def test_show_via_points(self, alice, command, goal, via):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute

        # Сейчас эта проверка не работает на авто из-за https://st.yandex-team.ru/ALICEINFRA-536
        if not surface.is_auto(alice):
            assert re.search(f'Едем {goal}', response.text)
            assert via in response.text

        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri.startswith('yandexnavi://build_route_on_map?confirmation=1')
        for param in ['&lon_to=', '&lat_to=', '&lon_via_0=', '&lat_via_0=']:
            assert param in response.directive.payload.uri

    @pytest.mark.parametrize('command, goal, search', [
        ('Поехали на Воробьёвы горы по Садовому кольцу',
         'до "Воробьёвы горы"', 'садовое кольцо'),
        ('Поехали на улицу Ботаническая, дом 27 через МГТУ им. Н.Э.Баумана',
         'до адреса Ботаническая улица 27', 'мгту им н э баумана'),
        ('Поехали сначала в МГТУ им. Н.Э.Баумана, а потом на улицу Ботаническая, дом 27',
         'до адреса Ботаническая улица 27', 'мгту им н э баумана'),
        ('Поехали на улицу Строителей, дом 5, корпус 3 через МакДональдс',
         'до адреса улица Строителей 5к3', 'макдональдс'),
        ('Поехали на улицу Строителей, дом 5, корпус 3 с заездом в МакДональдс',
         'до адреса улица Строителей 5к3', 'макдональдс'),
    ])
    def test_show_via_search_points(self, alice, command, goal, search):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute

        # Сейчас эта проверка не работает на авто из-за https://st.yandex-team.ru/ALICEINFRA-536
        if not surface.is_auto(alice):
            response_text = response.text.lower()
            assert f'Едем {goal}' in response.text
            assert 'отметила на карте' in response_text
            assert (
                'все нужные места' in response_text
                or 'все нужные точки' in response_text
                or 'нужные места' in response_text
            )

        assert response.directive.name == directives.names.OpenUriDirective
        uri = unquote(response.directive.payload.uri)
        assert uri.startswith('yandexnavi://map_search?')
        assert f'text={search}' in uri

    @pytest.mark.parametrize('command, goal', [
        ('Заехать на улицу Ботаническая, дом 27',
         'до адреса Садовническая набережная 79 через адрес Ботаническая улица 27.'),
    ])
    @pytest.mark.device_state(navigator=_STATE_ROUTE_FROM_KR_TO_AURORA)
    def test_on_route_add_via_points(self, alice, command, goal):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute

        # Сейчас эта проверка не работает на авто из-за https://st.yandex-team.ru/ALICEINFRA-536
        if not surface.is_auto(alice):
            assert response.text.startswith((
                'Принято', 'Хорошо', 'В путь!',
            ))
            assert f'Едем {goal}' in response.text

        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri.startswith('yandexnavi://build_route_on_map?confirmation=1')

    @pytest.mark.parametrize('command, search', [
        ('Заехать в МакДональдс', 'макдональдс'),
        ('Заехать в МГТУ им. Баумана', 'мгту им баумана'),
        ('Заехать на заправку', 'заправку'),
    ])
    @pytest.mark.device_state(navigator=_STATE_ROUTE_FROM_KR_TO_AURORA)
    def test_on_route_add_via_search_points(self, alice, command, search):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute

        # Сейчас эта проверка не работает на авто из-за https://st.yandex-team.ru/ALICEINFRA-536
        if not surface.is_auto(alice):
            response_text = response.text.lower()
            assert 'отметила на карте' in response_text
            assert (
                'все нужные места' in response_text
                or 'все нужные точки' in response_text
                or 'нужные места' in response_text
            )

        assert response.directive.name == directives.names.OpenUriDirective
        uri = unquote(response.directive.payload.uri)
        assert uri.startswith('yandexnavi://map_search?')
        assert f'text={search}' in uri

        # TODO: https://st.yandex-team.ru/ALICEINFRA-731
        # эмулировать поиск и выбор точки, проверить, что точка присутствует после "заехать"

        response = alice('Заехать')
        # Сейчас эта проверка не работает на авто из-за https://st.yandex-team.ru/ALICEINFRA-536
        if not surface.is_auto(alice):
            assert response.text.startswith((
                'Принято', 'Хорошо', 'В путь!',
            ))
            uri = unquote(response.directive.payload.uri)
            # cейчас тут ничего интересного нет
            assert 'yandexnavi://show_ui/map/travel' == uri
