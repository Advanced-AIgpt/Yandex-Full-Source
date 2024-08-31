import math
import re

import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from . import CoordEps, get_point


@pytest.mark.region(region.Moscow)
class TestBuildRoute(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1194
    https://testpalm.yandex-team.ru/testcase/alice-1599
    Частично https://testpalm.yandex-team.ru/testcase/alice-1802
    """

    owners = ('isiv',)

    @pytest.mark.parametrize('surface', [surface.station(is_tv_plugged_in=False)])
    def test_route_go_home(self, alice):
        response = alice('построй маршрут')
        assert response.scenario in {scenario.Vins}
        assert response.intent == intent.ShowRoute
        response = alice('домой')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.RememberNamedLocation

    @pytest.mark.parametrize('surface', [surface.automotive, surface.navi])
    @pytest.mark.parametrize('command, goal, lat, lon', [(
        'на улицу Ботаническая', 'до адреса Ботаническая улица', 55.834315, 37.593738
    )])
    def test_build_two_steps(self, alice, command, goal, lat, lon):
        response = alice('Поехали')
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert 'Куда' in response.text

        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRouteEllipsis
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://build_route_on_map?confirmation=1' in response.directive.payload.uri
        assert math.hypot(lat - get_point(response.directive.payload.uri, 'lat_to'),
                          lon - get_point(response.directive.payload.uri, 'lon_to')) < CoordEps
        if surface.is_navi(alice):
            assert response.text.startswith((
                'Принято', 'Хорошо', 'В путь!',
            ))
            assert f'Едем {goal}' in response.text

        response = alice('Поехали')
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://external_confirmation?confirmed=1' in response.directive.payload.uri

    @pytest.mark.parametrize('surface', [surface.automotive, surface.navi])
    @pytest.mark.parametrize('command, goal, lat, lon', [
        ('Поехали в Питер', 'до адреса Санкт-Петербург', 59.93895, 30.31564),
        ('Как проехать в Ашан?', 'до "Ашан" по адресу улица Вавилова 3', 55.7068, 37.5924),
        ('Поехали на заправку', 'до "ЕКА" по адресу Саввинская набережная 7А', 55.7376775, 37.5695535),
        ('Поехали на Льва Толстого 16', 'до адреса улица Льва Толстого 16', 55.733937, 37.587046),
    ])
    def test_build_to_goal(self, alice, command, goal, lat, lon):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://build_route_on_map?confirmation=1' in response.directive.payload.uri
        assert math.hypot(lat - get_point(response.directive.payload.uri, 'lat_to'),
                          lon - get_point(response.directive.payload.uri, 'lon_to')) < CoordEps
        if surface.is_navi(alice):
            assert response.text.startswith((
                'Принято', 'Хорошо', 'В путь!',
            ))
            assert f'Едем {goal}' in response.text

        response = alice('Поехали')
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://external_confirmation?confirmed=1' in response.directive.payload.uri

    @pytest.mark.parametrize('surface', [surface.navi])
    @pytest.mark.parametrize('command, route_regexp, goal, additional_suggests, hint_regexp', [
        ('Пешком в ближайший парк',
         r'займет 1[0-9] минут',
         'до "Сквер Тургенева" по адресу улица Остоженка 41',
         'На транспорте',
         'до "Усадьба Трубецких" по адресу Усачёва улица 1А, стр. 2'),
        ('Поехали на трамвае в библиотеку',
         r'займет [12][0-9] минут.?, включая (1 километр )?([1-9]00 метров )?пешком',
         'до "РГБ" по адресу улица Воздвиженка 3/5',
         'Пешком',
         r'до "(.*[Бб]иблиотека.*|Р[Гг][Дд]?[Бб])" по адресу'),
    ])
    def test_build_to_goal_by(self, alice, command, route_regexp, goal, additional_suggests, hint_regexp):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert response.text.startswith((
            'Маршрут займет', 'Путь займет', 'Дорога займет',
        ))
        assert goal in response.text
        assert re.search(route_regexp, response.text)

        suggests = {s.title for s in response.suggests}
        must_suggest = {'На такси', 'На авто', 'Маршрут на карте', 'Что ты умеешь?', additional_suggests}
        assert suggests.issuperset(must_suggest)
        for other in suggests.difference(must_suggest):
            assert re.search(hint_regexp, other)

    @pytest.mark.parametrize('surface', [surface.automotive, surface.navi])
    @pytest.mark.parametrize('command, goal, lat_from, lat_to, lon_from, lon_to', [(
        'Поехали от улицы Тверская до Ломоносовского проспекта',
        'до адреса Ломоносовский проспект',
        55.7639, 55.6972, 37.6064, 37.5292,
    )])
    def test_build_from_to_route(self, alice, command, goal, lat_from, lat_to, lon_from, lon_to):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://build_route_on_map?confirmation=1' in response.directive.payload.uri
        assert re.search(rf'lat_from={lat_from}\d+&lat_to={lat_to}\d+', response.directive.payload.uri)
        assert re.search(rf'lon_from={lon_from}\d+&lon_to={lon_to}\d+', response.directive.payload.uri)
        if surface.is_navi(alice):
            assert response.text.startswith((
                'Принято', 'Хорошо', 'В путь!',
            ))
            assert f'Едем {goal}' in response.text

        response = alice('Поехали')
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://external_confirmation?confirmed=1' in response.directive.payload.uri

    @pytest.mark.parametrize('surface', [surface.old_automotive])
    @pytest.mark.parametrize('command, goal, lat, lon', [
        ('Поехали в Питер', 'до адреса Санкт-Петербург', 59.93895069, 30.31563454),
        ('Как проехать в Ашан?', 'до "Ашан" по адресу улица Вавилова 3', 55.706827, 37.592429),
        ('Поехали на заправку', 'до "ЕКА" по адресу Саввинская набережная 7А', 55.7376775, 37.5695535),
        ('Поехали на Льва Толстого 16', 'до адреса улица Льва Толстого 16', 55.73396898, 37.58709252),
        ('Поехали на Льва Толстого шестнадцать', 'до адреса улица Льва Толстого 16', 55.73396898, 37.58709252),
    ])
    def test_alice_1802(self, alice, command, goal, lat, lon):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.YandexNaviDirective
        assert response.directive.payload.application == 'yandexnavi'
        assert response.directive.payload.intent == 'build_route_on_map'
        assert response.directive.payload.params.confirmation == '1'
        assert math.hypot(response.directive.payload.params.lat_to - lat,
                          response.directive.payload.params.lon_to - lon) < CoordEps

        response = alice('Поехали')
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.AutoAppConfirmationYes
        assert response.directive.name == directives.names.YandexNaviDirective
        assert response.directive.payload.application == 'yandexnavi'
        assert response.directive.payload.intent == 'external_confirmation'
        assert response.directive.payload.params.app == 'yandexnavi'
        assert response.directive.payload.params.confirmed == '1'

    @pytest.mark.parametrize('surface', [surface.automotive, surface.navi])
    @pytest.mark.parametrize('command, goal, lat, lon', [
        ('маршрут коллонтай сорок три', 'до адреса Санкт-Петербург, улица Коллонтай 43', 59.9246, 30.4971),
    ])
    def test_build_route_alphabetic_address(self, alice, command, goal, lat, lon):
        response = alice(command)
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert 'yandexnavi://build_route_on_map?confirmation=1' in response.directive.payload.uri
        if surface.is_navi(alice):
            assert response.text.startswith((
                'Принято', 'Хорошо', 'В путь!',
            ))
            assert f'Едем {goal}' in response.text
        assert math.hypot(lat - get_point(response.directive.payload.uri, 'lat_to'),
                          lon - get_point(response.directive.payload.uri, 'lon_to')) < CoordEps

        response = alice('Поехали')
        assert response.scenario in {scenario.Vins, scenario.Route}
        assert response.intent == intent.ShowRoute
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'yandexnavi://external_confirmation?confirmed=1' in response.directive.payload.uri
