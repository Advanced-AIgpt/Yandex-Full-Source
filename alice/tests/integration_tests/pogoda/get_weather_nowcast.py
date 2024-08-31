import alice.tests.library.directives as directives
import alice.tests.library.surface as surface
import pytest

from .common import assert_is_nowcast_case, assert_is_not_nowcast_case, assert_is_common_weather_case


_smoke_requests = [
    'осадки в москве',
]

_basic_requests = [
    'будет ли дождь сегодня',
    'зонт брать',
    'когда будет снег',
    'осадки в городе москва',
    'дождь прямо сейчас есть',
    'будет снег вечером',
]

_positives = [f'какие осадки {city}'.strip() for city in ['', 'в москве', 'в чикаго', 'в городе гусь-хрустальный', 'в поселке березовка']]

_negatives = [
    'какой ветер',
    'дождь',
    'песня про дождик',
    'покажи белый снег ',
]

_ellipsis_positives = [
    'а в красноярске',
    'а в питере',
]

_ellipsis_negatives = [
    'дождь идет',
]


class TestNowcast(object):

    owners = ('abc:weatherbackendvteam', )

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _smoke_requests)
    def test_nowcast_smoke(self, alice, command):
        response = alice(command)
        assert_is_nowcast_case(response)
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == \
               'https://yandex.ru/pogoda/maps/nowcast?appsearch_header=1&from=alice_raincard&lat=55.753215&lon=37.622504&utm_campaign=card&utm_content=fullscreen&utm_medium=nowcast&utm_source=alice'

    @pytest.mark.parametrize('surface', surface.actual_surfaces)
    @pytest.mark.parametrize('command', _basic_requests)
    def test_all_surfaces(self, alice, command):
        response = alice(command)
        assert_is_nowcast_case(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _positives)
    def test_nowcast_positive(self, alice, command):
        response = alice(command)
        assert_is_nowcast_case(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _negatives)
    def test_nowcast_negative(self, alice, command):
        response = alice(command)
        assert_is_not_nowcast_case(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _ellipsis_positives)
    def test_nowcast_ellipsis_positive(self, alice, command):
        alice('осадки в москве')
        response = alice(command)
        assert_is_nowcast_case(response, ellipsis=True)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _ellipsis_negatives)
    def test_nowcast_ellipsis_negative(self, alice, command):
        alice('осадки в москве')
        response = alice(command)
        assert_is_not_nowcast_case(response, ellipsis=True)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', ['будет дождь завтра', 'какие осадки в красноярском крае'])
    def test_nowcast_fallback_on_weather_scenario(self, alice, command):
        response = alice(command)
        assert_is_common_weather_case(response)
