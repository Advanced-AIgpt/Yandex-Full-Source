import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.experiments('disable_mm_skill_discovery_on_gc')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestSkillDiscovery(object):

    owners = ('kuptservol',)

    @pytest.mark.xfail(reason='PASKILLS-6818')
    def test_vins_games_onboarding_must_win(self, alice):
        response = alice('давай поиграем')
        assert response.scenario == scenario.Vins

    def test_skill_discovery_gc_show(self, alice):
        response = alice('математика алиса')
        assert response.scenario == scenario.SkillDiscoveryGc
