import math
import re
from urllib.parse import unquote

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import route.div_card as div_card

from . import (CoordEps, get_point)


def assert_find_route(response, goal):
    response_text = response.text.lower()
    assert goal.lower() in response_text
    assert (
        'нашла вот такие маршруты' in response_text
        or 'вот разные маршруты' in response_text
        or 'прикинула, как добраться' in response_text
        or 'можно добраться вот так' in response_text
    )


@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [
    surface.searchapp,
    surface.yabro_win,
])
class TestHowToGetToPlace(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1665
    https://testpalm.yandex-team.ru/testcase/alice-1668
    """

    owners = ('isiv',)

    @pytest.mark.parametrize('use_suggest', [True, False])
    @pytest.mark.parametrize('how_to_get_by, suggest_title', [
        ('на общественном транспорте', 'На транспорте'),
        ('пешком', 'Пешком'),
        ('на машине', 'На авто'),
    ])
    def test_routes(self, alice, how_to_get_by, suggest_title, use_suggest):
        place = 'до "медси"'
        response = alice('Как доехать до аптеки медси')
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert_find_route(response, place)

        assert response.div_card, 'Expect div_card gallery with different routes'
        routes = div_card.RouteGallery(response.div_card)
        for route in routes:
            assert route.map
            map_url = unquote(route.map.image_url)
            assert map_url.startswith('https://static-maps.yandex.ru')
            assert re.search(r'pt=(\d|\.|,)+ya_ru~(\d|\.|,)+round', map_url), 'Expext start/finish route points'
            assert re.search(r'pl=c:8822DDC0,w:5(\d|\.|,)', map_url), 'Expect route line'

            assert route.icon.image_url.startswith('https://avatars.mds.yandex.net')
            assert 'мин' in route.distance
            assert 'МАРШРУТ' in route.footer.text
            assert route.footer.action_url, 'Expect link to Ya.Maps'
            assert route.footer.directives[0]['name'] == directives.names.OpenUriDirective

        suggest = response.suggest(suggest_title)
        assert suggest, 'Expect "{suggest_title}" suggest'

        response = alice.click(suggest) if use_suggest else alice(f'А {how_to_get_by}?')

        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRouteEllipsis
        response_text = response.text.lower()
        assert place in response_text and how_to_get_by in response_text and 'минут' in response_text
        assert response.div_card, 'Expect div_card with route'
        routes = div_card.RouteGallery(response.div_card)
        assert len(routes) == 1, 'Expect only one route'

    @pytest.mark.oauth(auth.RobotTaxi)
    @pytest.mark.parametrize('use_suggest', [True, False])
    def test_route_by_taxi(self, alice, use_suggest):
        place = 'до "медси"'
        response = alice('Как доехать до аптеки медси')
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert_find_route(response, place)

        on_taxi_suggest = response.suggest('На такси')
        assert on_taxi_suggest

        response = alice.click(on_taxi_suggest) if use_suggest else alice('А на такси?')

        if surface.is_yabro_win(alice):
            assert response.intent == intent.TaxiNewDisabled
            assert response.text == 'Здесь я не справлюсь. Лучше приложение Яндекс или Яндекс.Станция, давайте там.'
        elif surface.is_searchapp(alice):
            assert response.intent == intent.TaxiNewOrder
            assert not response.directive
            resp_list = response.text.split('\n')
            assert resp_list[0].startswith('Поищем такси в Яндекс Go.')
            assert 'Тариф' in resp_list[0] and 'оплата наличными' in resp_list[0]
            assert place in resp_list[0].lower()
            assert resp_list[1].startswith('Машина будет через') and resp_list[1].endswith('мин.')
            assert resp_list[2].startswith('Ехать') and resp_list[2].endswith('мин.')
            assert resp_list[-1].endswith(('Подтвердите, пожалуйста.', 'Всё ли верно?'))

    @pytest.mark.parametrize('route_command, route_type', [
        ('На транспорте', 'mt'),
        ('Пешком', 'pd'),
        ('На авто', 'auto'),
    ])
    def test_open_route_on_map(self, alice, route_command, route_type):
        place = 'до "медси"'
        response = alice('Как доехать до аптеки медси')
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert_find_route(response, place)

        response = alice(route_command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRouteOnMap
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.text in {'Сейчас открою маршрут на карте.', 'Открываю маршрут на карте.'}
        uri = unquote(response.directive.payload.uri)
        if surface.is_searchapp(alice) and route_type == 'auto':
            assert 'intent://build_route_on_map?' in uri
            assert f'rtt%2525253D{route_type}' in uri
        else:
            protocol = 'https' if surface.is_yabro_win(alice) else 'intent'
            assert f'{protocol}://yandex.ru/maps?' in uri
            assert f'rtt={route_type}' in uri


@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestOpenRouteOnMap(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2200
    """

    owners = ('isiv',)

    @pytest.mark.parametrize('command', [
        'построй маршрут',
        'построй маршрут на карте',
        'открой маршрут на карте',
        'проложи маршрут на карте',
    ])
    @pytest.mark.parametrize('details_command, route_type', [
        ('построй маршрут пешком', 'pd'),
        ('построй маршрут на машине', 'auto'),
        ('построй маршрут на общественном транспорте', 'mt'),
    ])
    def test_alice_2200(self, alice, command, details_command, route_type):
        goal = 'до "пушкинъ"'
        goal_point = 55.763738, 37.604936
        response = alice('построй маршрут в кафе Пушкин')
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRoute
        assert_find_route(response, goal)

        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.intent == intent.ShowRouteOnMap
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'intent://yandex.ru/maps?' in response.directive.payload.uri

        response = alice(details_command)
        assert response.scenario in {scenario.Vins, scenario.Route, scenario.HollywoodRoute}
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.intent == intent.ShowRouteOnMap
        uri = unquote(response.directive.payload.uri)
        if route_type == 'auto':
            assert 'intent://build_route_on_map?' in uri
            assert f'rtt%2525253D{route_type}%3Bend' in uri
            assert math.hypot(goal_point[0] - get_point(uri, '&lat_to'),
                              goal_point[1] - get_point(uri, '&lon_to')) < CoordEps
        else:
            assert 'intent://yandex.ru/maps?' in uri
            assert f'rtt={route_type}' in uri
            m = re.search(r'rtext=(\d+.\d+),(\d+.\d+)~(\d+.\d+),(\d+.\d+)', uri)
            assert len(m.groups()) == 4
            assert math.hypot(goal_point[0]-float(m[3]), goal_point[1]-float(m[4])) < CoordEps
