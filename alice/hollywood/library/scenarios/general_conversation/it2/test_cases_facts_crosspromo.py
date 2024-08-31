import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.scenario_responses import Accumulator


@pytest.fixture(scope="module")
def enabled_scenarios():
    return ["general_conversation"]


@pytest.mark.scenario(name="GeneralConversation", handle="general_conversation")
@pytest.mark.parametrize("surface", [surface.loudspeaker, surface.searchapp])
class TestGeneralConversationBaseFactsCrosspromo:
    @pytest.mark.experiments(
        "hw_facts_crosspromo_non_quasar",
        "bg_fresh_granet",
        "hw_gc_disable_movie_discussions_by_default",
        "hw_facts_crosspromo_change_questions",
        "hw_facts_crosspromo_full_dict",
        "hw_facts_crosspromo_scenario_filter_disable",
    )
    def test_facts_crosspromo_discuss(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("у меня теперь есть кошка")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_facts_crosspromo_non_quasar",
        "bg_fresh_granet",
        "hw_gc_disable_movie_discussions_by_default",
        "hw_facts_crosspromo_change_questions",
        "hw_facts_crosspromo_scenario_filter_disable",
    )
    def test_facts_crosspromo_discuss_filtered(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("у меня теперь есть кошка")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_facts_crosspromo_non_quasar",
        "bg_fresh_granet",
        "hw_gc_disable_movie_discussions_by_default",
        "hw_facts_crosspromo_change_questions",
        "hw_facts_crosspromo_scenario_filter_disable",
    )
    def test_facts_crosspromo_discuss_timeout(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("у меня теперь есть кошка")))
        acc.add(alice(voice("а ты любишь собак?")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_facts_crosspromo_non_quasar",
        "bg_fresh_granet",
        "hw_gc_disable_movie_discussions_by_default",
        "hw_facts_crosspromo_change_questions",
    )
    def test_facts_crosspromo_previous_scenario(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("ты любишь розы?")))
        return str(acc)
