import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return 'automotive_hvac'


@pytest.fixture(scope='function')
def srcrwr_params(kronstadt_grpc_port):
    return {
        'AUTOMOTIVE_HVAC': f'localhost:{kronstadt_grpc_port}'
    }


@pytest.mark.scenario(name='AutomotiveHvac', handle='automotive_hvac')
@pytest.mark.parametrize('surface', [surface.automotive])
@pytest.mark.experiments(
    'bg_fresh_alice'
)
class TestAutomotiveHvac():

    def test_front_defroster_off(self, alice):
        response = alice(voice('выключи обогрев переднего стекла'))
        assert response.scenario_stages() == {'run'}

        return str(response)

    def test_front_defroster_on(self, alice):
        response = alice(voice('включи обогрев переднего стекла'))
        assert response.scenario_stages() == {'run'}

        return str(response)

    def test_rear_defroster_off(self, alice):
        response = alice(voice('выключи обогрев заднего стекла'))
        assert response.scenario_stages() == {'run'}

        return str(response)

    def test_rear_defroster_on(self, alice):
        response = alice(voice('включи обогрев заднего стекла'))
        assert response.scenario_stages() == {'run'}

        return str(response)
