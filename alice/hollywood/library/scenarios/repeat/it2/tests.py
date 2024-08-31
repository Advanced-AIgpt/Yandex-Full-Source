import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice, Scenario


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['repeat', 'random_number']


@pytest.mark.scenario(name='Repeat', handle='repeat')
@pytest.mark.parametrize('surface', [surface.station])
class Tests:

    def test_player_features(self, alice):
        r = alice(voice('загадай случайное число от ста до двухсот',
                        scenario=Scenario('RandomNumber', 'random_number')))
        assert r.scenario_stages() == {'run'}

        alice.skip(seconds=10)

        r = alice(voice('загадай случайное число от одного до двух',
                        scenario=Scenario('RandomNumber', 'random_number')))
        assert r.scenario_stages() == {'run'}

        alice.skip(seconds=10)

        r = alice(voice('повтори'))
        assert r.scenario_stages() == {'run'}
        return str(r)
