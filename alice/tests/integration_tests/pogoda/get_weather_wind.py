import alice.tests.library.surface as surface
import pytest

from .common import assert_is_wind_case, assert_is_not_wind_case, now_phrases, common_contexts


_smoke_requests = [
    'ветер сегодня в москве',
]

_basic_requests = [
    'ветер',
    'сила ветра на выходных',
    'какой ветер будет дуть завтра',
    'скажи сколько метров в секунду ветер сейчас',
    'какой ветер вчера был',
    'ветер вечерком',
]

_positives = [f'какой ветер {context}'.strip() for context in common_contexts]

_negatives = [
    'какой ветер считается сильным',
    'унесенные ветром',
    'ветер с моря дул',
    'ветреная девушка',
    'как посчитать силу ветра',
]

_ellipsis_positives = [
    'а в красноярске',
    'а завтра',
]

_ellipsis_negatives = [
    'ветер',
    'а в целом погода',
]


@pytest.mark.experiments('weather_use_wind_scenario')
class TestWind(object):

    owners = ('abc:weatherbackendvteam', )

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _smoke_requests)
    def test_pressure_smoke(self, alice, command):
        response = alice(command)
        assert_is_wind_case(response)
        assert any(response.text.startswith(f'{now_phrase} в Москве') for now_phrase in now_phrases)
        assert 'ветер' in response.text or 'штиль' in response.text

    @pytest.mark.parametrize('surface', surface.actual_surfaces)
    @pytest.mark.parametrize('command', _basic_requests)
    def test_all_surfaces(self, alice, command):
        response = alice(command)
        assert_is_wind_case(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _positives)
    def test_pressure_positive(self, alice, command):
        response = alice(command)
        assert_is_wind_case(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _negatives)
    def test_pressure_negative(self, alice, command):
        response = alice(command)
        assert_is_not_wind_case(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _ellipsis_positives)
    def test_pressure_ellipsis_positive(self, alice, command):
        alice('ветер сегодня в москве')
        response = alice(command)
        assert_is_wind_case(response, ellipsis=True)
        assert 'ветер' in response.text or 'штиль' in response.text

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _ellipsis_negatives)
    def test_pressure_ellipsis_negative(self, alice, command):
        alice('ветер сегодня в москве')
        response = alice(command)
        assert_is_not_wind_case(response, ellipsis=True)
