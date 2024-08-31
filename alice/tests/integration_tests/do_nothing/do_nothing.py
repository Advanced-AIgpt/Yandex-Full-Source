import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [
    surface.searchapp,
    surface.smart_tv,
    surface.station,
])
class TestDoNothing(object):

    owners = ('olegator', )

    def test(self, alice):
        response = alice('ничего не делай')
        assert response.scenario == scenario.DoNothing
        assert not response.text
        assert not response.output_speech_text
        assert not response.directive
