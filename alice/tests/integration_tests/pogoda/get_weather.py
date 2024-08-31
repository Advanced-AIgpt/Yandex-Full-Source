import alice.tests.library.surface as surface
import pytest
import re

from .common import assert_is_common_weather_case, assert_is_not_common_weather_case, WeatherDivCard, common_contexts


_smoke_requests = [
    'погода сегодня в москве',
]

_basic_requests = [
    'погода',
    'что сегодня по погоде',
    'на следующих выходных солнечно будет',
    'хочу сейчас погоду узнать',
    'насколько холодно в мухосранске будет вечером',
    'тепло ли сейчас в сочи',
]

_positives = [f'какая погода {context}'.strip() for context in common_contexts]

_negatives = [
    'полгода плохая погода',
    'давление на завтра',
    'что значит одеться по погоде',
]

_ellipsis_positives = [
    'а в красноярске',
    'а завтра',
]

_ellipsis_negatives = [
    'погода',
    'а ветер какой',
    'а в мухосранске',
]


class TestWeather(object):

    owners = ('abc:weatherbackendvteam', )

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _smoke_requests)
    def test_weather_smoke(self, alice, command):
        response = alice(command)
        assert_is_common_weather_case(response)
        weather_card = WeatherDivCard(response.div_card)
        assert re.search(
            r'(сейчас|[в] настоящее время|[в] настоящий момент|[в] данный момент|[в] данную минуту) в москве',
            weather_card.title.lower()
        )
        assert 'Ощущается как' in weather_card.subtitle

    @pytest.mark.parametrize('surface', surface.actual_surfaces)
    @pytest.mark.parametrize('command', _basic_requests)
    def test_all_surfaces(self, alice, command):
        response = alice(command)
        assert_is_common_weather_case(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _positives)
    def test_weather_positive(self, alice, command):
        response = alice(command)
        assert_is_common_weather_case(response)

    @pytest.mark.experiments('weather_use_pressure_scenario')
    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _negatives)
    def test_weather_negative(self, alice, command):
        response = alice(command)
        assert_is_not_common_weather_case(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _ellipsis_positives)
    def test_weather_ellipsis_positive(self, alice, command):
        alice('погода сегодня в москве')
        response = alice(command)
        assert_is_common_weather_case(response, ellipsis=True)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _ellipsis_negatives)
    def test_weather_ellipsis_negative(self, alice, command):
        alice('погода сегодня в москве')
        response = alice(command)
        assert_is_not_common_weather_case(response, ellipsis=True)
