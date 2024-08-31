import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['random_number']


@pytest.mark.scenario(name='RandomNumber', handle='random_number')
@pytest.mark.parametrize('surface', surface.actual_surfaces)
class Tests:

    @pytest.mark.parametrize('command', [
        pytest.param('загадай случайное число', id='default_range'),
        pytest.param('назови случайное число от 2 до 15', id='from_2_to_15'),
    ])
    def test_make_random_number(self, alice, command):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run'}
        return str(r)

    @pytest.mark.parametrize('command', [
        pytest.param('брось один восьмигранный кубик', id='throw1_8edges'),
        pytest.param('брось три кубика', id='throw3_6edges'),
    ])
    def test_make_throw_dice(self, alice, command):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run'}
        return str(r)
