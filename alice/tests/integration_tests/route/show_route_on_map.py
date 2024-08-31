import re
import time
from urllib.parse import unquote

import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import route.div_card as div_card


@pytest.mark.parametrize('surface', [surface.automotive, surface.navi])
class TestShowRouteNaviScheme(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1221
    https://testpalm.yandex-team.ru/testcase/alice-1584
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
                    }
                ],
                'raw_time_to_destination': 1588,
                'time_in_traffic_jam': 0,
                'time_to_destination': 1443
            }
        }
    }

    @pytest.mark.parametrize('command', [
        'обзор',
        'обзор маршрута',
        'покажи маршрут',
        'есть маршрут быстрее?',
        'покажи дорогу',
        'как едем?',
        'обзор карта',
        'покажи весь маршрут движения',
        'что там дальше?',
        'как проехать иначе',
        'маршрут сверху',
        'полный маршрут',
    ])
    def test_show_route(self, alice, command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.NaviShowRouteOnMap
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == 'yandexnavi://show_route_overview'
        assert response.text in ['Это я мигом.', 'Сейчас покажу.', 'Хорошо.']

        alice.reset_route()

        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert not response.directive
        assert response.text in ['Куда нужно добраться?', 'Куда?']


@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.searchapp,
    surface.yabro_win,
])
class TestShowRouteOpenUri(object):

    owners = ('isiv',)

    @pytest.mark.parametrize('command', [
        'построй маршрут из Москвы в Волгоград',
        'поехали в Сочи из Смоленска',
    ])
    def test_show_route(self, alice, command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert not response.directive

        response = alice('открой на карте')
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRouteOnMap
        assert response.directive.name == directives.names.OpenUriDirective


@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.smart_tv,
    surface.station,
])
class TestShowRouteUnsupported(object):

    owners = ('isiv',)

    @pytest.mark.parametrize('command', [
        'построй маршрут из Москвы в Волгоград',
        'поехали в Сочи из Смоленска',
    ])
    def test_show_route(self, alice, command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert not response.directive

        response = alice('открой на карте')
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRouteOnMap
        assert not response.directive
        assert (
            response.text.startswith(('Я справлюсь с этим лучше на', 'Я бы и рада', 'Я могу разве что')) or
            response.text == 'Сейчас под рукой нет карты. Да и рук у меня нет. Давайте сменим тему.'
        )


@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.searchapp,
    surface.yabro_win,
])
class TestShowRouteVia(object):

    owners = ('isiv',)

    @pytest.mark.parametrize('command', [
        'построй маршрут из Москвы через Ростов в Волгоград',
        'поехали в Краснодар через Сочи из Смоленска',
    ])
    def test_show_route_via(self, alice, command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert not response.directive

        assert len(response.cards) in {1, 2}
        assert response.div_card

        routes = div_card.RouteGallery(response.div_card)
        assert len(routes) >= 1, 'Expect at least one route'
        for route in routes:
            assert route.map
            map_url = unquote(route.map.image_url)
            assert map_url.startswith('https://static-maps.yandex.ru')
            assert re.search(r'pt=(\d|\.|,)+ya_ru~(\d|\.|,)+round', map_url), 'Expext start/finish route points'
            assert re.search(r'pl=c:8822DDC0,w:5(\d|\.|,)', map_url), 'Expect route line'

            if 'МАРШРУТ ПЕШКОМ' in route.footer.text:
                assert re.search(r'очень много времени, \d+ км', route.distance)
            else:
                assert 'МАРШРУТ НА АВТО' in route.footer.text
                assert re.search(r'\d+ ч( \d+ мин)?, \d+ км', route.distance)
                assert 'via' in route.action_url
            assert route.icon.image_url.startswith('https://avatars.mds.yandex.net')
            assert route.footer.action_url, 'Expect link to Ya.Maps'
            assert route.footer.directives[0]['name'] == directives.names.OpenUriDirective


@pytest.mark.parametrize('surface', [
    surface.searchapp,
    surface.yabro_win,
])
class TestShowRouteDivCards(object):
    """
    cards:
    https://testpalm.yandex-team.ru/testcase/alice-1667
    https://testpalm.yandex-team.ru/testcase/alice-2170
    links:
    https://testpalm.yandex-team.ru/testcase/alice-1666
    https://testpalm.yandex-team.ru/testcase/alice-2171
    """

    owners = ('isiv',)

    @pytest.mark.parametrize('command', [
        'Как доехать до ресторана',
    ])
    def test_show_route_via(self, alice, command):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert not response.directive

        assert len(response.cards) >= 1, 'Expect at least one route'
        assert response.div_card

        routes = div_card.RouteGallery(response.div_card)
        for route in routes:
            assert route.map
            map_url = unquote(route.map.image_url)
            assert map_url.startswith('https://static-maps.yandex.ru')
            assert re.search(r'pt=(\d|\.|,)+ya_ru~(\d|\.|,)+round', map_url), 'Expext start/finish route points'
            assert re.search(r'pl=c:8822DDC0,w:5(\d|\.|,)', map_url), 'Expect route line'
            assert route.icon.image_url.startswith('https://avatars.mds.yandex.net')
            assert route.footer.action_url, 'Expect link to Ya.Maps'
            directive = route.footer.directives[0]
            assert directive['name'] == directives.names.OpenUriDirective
            assert directive['sub_name'] == 'show_route_button'
            uri = directive['payload']['uri']
            if surface.is_searchapp(alice):
                assert uri.startswith('intent://yandex.ru/maps?rtext=') or uri.startswith('intent://build_route_on_map')
            else:
                assert uri.startswith('https://yandex.ru/maps?rtext=')

            if 'МАРШРУТ ПЕШКОМ' in route.footer.text:
                assert re.search(r'\d+ мин, \d+ м', route.distance)
                assert '&rtt=pd' in uri
            elif 'МАРШРУТ НА ТРАНСПОРТЕ' in route.footer.text:
                assert re.search(r'\d+ мин', route.distance)
                assert '&rtt=mt' in uri
            elif 'МАРШРУТ НА АВТО' in route.footer.text:
                assert re.search(r'\d+ мин,( \d+ км)?( \d+ м)?', route.distance)
                assert '&rtt=auto' in uri or 'rtt%252525253Dauto' in uri
