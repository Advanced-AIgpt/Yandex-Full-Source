import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['hardcoded_response']


@pytest.mark.scenario(name='HardcodedResponse', handle='hardcoded_response')
@pytest.mark.experiments('mm_enable_granet_in_hardcoded_responses')
class TestHardcodedResponse:

    @pytest.mark.parametrize('surface', [surface.loudspeaker])
    def test_vedmak_pesnya(self, alice):
        r = alice(voice('чем заплатить ведьмаку'))
        return str(r)

    @pytest.mark.parametrize('surface', [surface.station_lite_red])
    def test_pomelo(self, alice):
        r = alice(voice('скажи где купить помело ночью алиса?'))
        return str(r)
