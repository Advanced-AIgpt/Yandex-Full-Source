import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.navi])
class TestSpeechkit(object):
    owners = ('g-kostin',)

    @pytest.mark.region(
        lat=47.222078,
        lon=39.720349,
        timezone='Europe/Moscow',
        region_id=26,
        client_ip='31.13.145.2',
        accuracy=15000,
    )
    def test_user_timezone(self, alice):
        response = alice('проспект солженицына 21/106')
        assert 'error' not in response.raw.get('meta', [])

    @pytest.mark.region(
        timezone='Europe/Moscow',
        region_id=26,
        client_ip='31.13.145.2',
    )
    def test_user_timezone_empty_region(self, alice):
        response = alice('проспект солженицына 21/106')
        assert 'error' not in response.raw.get('meta', [])
