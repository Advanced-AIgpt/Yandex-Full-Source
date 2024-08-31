import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.surface as surface
import pytest


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['vins']


@pytest.mark.scenario(name='Vins', handle='vins')
@pytest.mark.voice
@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [surface.searchapp])
class Tests:

    def test_run(self, alice):
        response = alice('долорес режим анализа')
        assert response.scenario_stages() == {'run'}
        analytics = response.run_response.ResponseBody.AnalyticsInfo
        assert analytics.Intent == intent.Bugreport
        return str(response)

    def test_apply(self, alice):
        response = alice('Вызови такси')
        assert response.scenario_stages() == {'run', 'apply'}
        analytics = response.apply_response.ResponseBody.AnalyticsInfo
        assert analytics.Intent == intent.TaxiNewDisabled
        return str(response)

    def test_commit(self, alice):
        response = alice('сколько будет стоить привезти тело из турции в киргизию')
        assert response.scenario_stages() == {'run', 'commit'}
        analytics = response.run_response.CommitCandidate.ResponseBody.AnalyticsInfo
        assert analytics.Intent == 'personal_assistant.scenarios.direct_gallery'
        return str(response)

    def test_vins_response_proto(self, alice):
        response = alice('сколько время в нью-йорке')
        assert response.scenario_stages() == {'run'}
        analytics = response.run_response.ResponseBody.AnalyticsInfo
        assert analytics.Intent == intent.GetTime
        return str(response)
