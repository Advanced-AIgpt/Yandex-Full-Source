import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', surface.actual_surfaces)
@pytest.mark.experiments(f'mm_scenario={scenario.ShowTraffic}')
class TestIrrelevantTraffic(object):
    owners = ('ikorobtsev',)

    def test_relevant(self, alice):
        response = alice('Пробки в Санкт-Петербурге')
        assert response.scenario == scenario.ShowTraffic
        assert response.intent == intent.ShowTraffic

        response = alice('В Москве')
        assert response.scenario == scenario.ShowTraffic
        assert response.intent in {intent.ShowTraffic, intent.ShowTrafficEllipsis}

    def test_irrelevant(self, alice):
        response = alice('В Москве')
        assert response.scenario == scenario.ShowTraffic
        assert response.intent not in {intent.ShowTraffic, intent.ShowTrafficEllipsis}
