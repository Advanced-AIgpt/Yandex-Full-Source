import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.experiments(f'mm_scenario={scenario.Vins}')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestVinsResponse(object):

    owners = ('g-kostin',)

    @pytest.mark.text
    def test_should_not_listen_after_text_request_by_default(self, alice):
        response = alice('как дела')
        assert response.voice_response.should_listen is False

    @pytest.mark.voice
    def test_should_listen_after_voice_request_by_default(self, alice):
        response = alice('как дела')
        assert response.voice_response.should_listen is True

    @pytest.mark.region(region.ZeroLocation)
    @pytest.mark.voice
    def test_zero_location(self, alice):
        response = alice('привет')
        assert response.scenario == scenario.Vins
