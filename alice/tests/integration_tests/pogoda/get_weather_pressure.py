import alice.tests.library.surface as surface
import pytest

from .common import assert_is_pressure_case, assert_is_not_pressure_case, now_phrases, common_contexts


_smoke_requests = [
    'давление сегодня в москве',
]

_basic_requests = [
    'давление',
    'атмосферное давление на выходных',
    'давление воздуха завтра',
    'скажи сколько миллиметров ртутного столба давление сейчас',
    'давление вечерком',
]

_positives = [f'какое давление {context}'.strip() for context in common_contexts]

_negatives = [
    'какое давление считается нормальным',
    'артериальное давление',
    'что делать если высокое давление',
    'давление повысилось',
    'давление внутри шин',
]

_ellipsis_positives = [
    'а в красноярске',
    'а завтра',
]

_ellipsis_negatives = [
    'давление',
    'а в целом погода',
]


@pytest.mark.experiments('weather_use_pressure_scenario')
class TestPressure(object):

    owners = ('abc:weatherbackendvteam', )

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _smoke_requests)
    def test_pressure_smoke(self, alice, command):
        response = alice(command)
        assert_is_pressure_case(response)
        assert any(response.text.startswith(f'{now_phrase} в Москве давление составляет') for now_phrase in now_phrases)

    @pytest.mark.parametrize('surface', surface.actual_surfaces)
    @pytest.mark.parametrize('command', _basic_requests)
    def test_all_surfaces(self, alice, command):
        response = alice(command)
        assert_is_pressure_case(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _positives)
    def test_pressure_positive(self, alice, command):
        response = alice(command)
        assert_is_pressure_case(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _negatives)
    def test_pressure_negative(self, alice, command):
        response = alice(command)
        assert_is_not_pressure_case(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _ellipsis_positives)
    def test_pressure_ellipsis_positive(self, alice, command):
        alice('давление сегодня в москве')
        response = alice(command)
        assert_is_pressure_case(response, ellipsis=True)
        assert 'давление составляет' in response.text or 'давление составит' in response.text

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', _ellipsis_negatives)
    def test_pressure_ellipsis_negative(self, alice, command):
        alice('давление сегодня в москве')
        response = alice(command)
        assert_is_not_pressure_case(response, ellipsis=True)
