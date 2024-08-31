import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


def _get_related_query(response):
    return response.scenario_analytics_info.objects['factoid_related_query']['human_readable']


def _get_search_query(response):
    return response.scenario_analytics_info.objects['tagger_query']['human_readable']


@pytest.mark.parametrize('surface', [surface.station])
class TestSearchRelatedFacts(object):

    owners = ('gserge', 'svetlana-yu')

    def test_related_fact_agree(self, alice):
        response = alice('сколько ног у паука')
        assert response.scenario == scenario.Search
        assert response.intent == intent.Factoid
        related_query = _get_related_query(response)
        assert related_query

        response = alice('да')
        assert response.scenario == scenario.Search
        assert response.intent == intent.Factoid
        assert _get_search_query(response) == related_query

    def test_related_fact_disagree(self, alice):
        response = alice('сколько ног у паука')
        assert response.scenario == scenario.Search
        assert response.intent == intent.Factoid
        assert _get_related_query(response)

        response = alice('нет')
        assert response.scenario == scenario.DoNothing
        assert not response.text

    def test_related_fact_ignore(self, alice):
        response = alice('сколько ног у паука')
        assert response.scenario == scenario.Search
        assert response.intent == intent.Factoid
        assert _get_related_query(response)

        response = alice('расскажи какая калорийность у яблока')
        assert response.scenario == scenario.Search
        assert response.intent == intent.Factoid
        assert _get_search_query(response) == 'какая калорийность у яблока'
