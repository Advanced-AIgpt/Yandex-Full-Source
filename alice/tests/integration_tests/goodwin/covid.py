import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.station])
class TestCovid(object):

    owners = ('the0', )

    @pytest.mark.parametrize('command', [
        'коронавирус',
        'короновирус',
        'корона вирус',
        'covid 19',
        'ковид 19',
        'расскажи про коронавирус',
        'когда карантин закончится'
    ])
    def test_wizard_one_step(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Covid19
        assert 'covid' in response.intent

    @pytest.mark.parametrize('command', [
        'симптомы',
        'рекомендации',
    ])
    def test_wizard_two_steps(self, alice, command):
        response = alice('коронавирус')
        assert response.scenario == scenario.Covid19
        assert 'covid' in response.intent

        response = alice(command)
        assert response.scenario == scenario.Covid19
        assert 'covid' in response.intent
