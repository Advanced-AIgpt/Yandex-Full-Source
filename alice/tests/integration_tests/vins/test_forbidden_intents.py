import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.searchapp,
    surface.station,
])
@pytest.mark.experiments('mm_scenario=Vins', forbidden_intents=intent.GetWeather)
class TestVinsForbiddenIntents(object):

    owners = ('alkapov',)

    def test_forbidden_weather(self, alice):
        response = alice('какая погода?')
        assert response.intent != intent.GetWeather
