import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.experiments(
    'enable_protocol_search_everywhere',
    'mm_increase_search_priority_for_searchapp_and_yabromobile',
    'mm_enable_search_activate_by_vins_for_searchapp_and_yabromobile',
)
class TestSearchEllipsisButtons(object):

    owners = ('tolyandex', 'svetlana-yu')

    search_intents = [intent.Factoid, intent.ObjectAnswer, intent.Calculator, intent.Search, intent.Serp]

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', [
        'сколько будет 10 плюс 10',
        'какой счет у спартака',
    ])
    def test_serp(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Search
        assert response.intent in self.search_intents
        response = alice('открой в поиске')
        assert response.intent == intent.Serp

    @pytest.mark.parametrize('surface', [
        surface.searchapp,
        surface.station,
    ])
    @pytest.mark.parametrize('command', [
        'расскажи какая калорийность у яблока',
        'что такое слава',
    ])
    def test_factoid_src(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Search
        assert response.intent in self.search_intents
        response = alice('подробнее')
        assert response.intent == intent.FactoidSrc

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_factoid_call(self, alice):
        response = alice('телефон службы доверия')
        assert response.scenario == scenario.Search
        assert response.intent in self.search_intents
        response = alice('набери им')
        assert response.intent == intent.FactoidCall
