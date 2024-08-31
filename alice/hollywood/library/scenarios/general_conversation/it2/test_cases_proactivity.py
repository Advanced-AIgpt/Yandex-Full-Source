import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.scenario_responses import Accumulator


@pytest.fixture(scope="module")
def enabled_scenarios():
    return ["general_conversation"]


@pytest.mark.scenario(name="GeneralConversation", handle="general_conversation")
@pytest.mark.parametrize("surface", [surface.searchapp])
class TestGeneralConversationBaseProactivity:
    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=1",
        "hw_gc_proactivity_timeout=0",
    )
    def test_proactivity_index_discuss_movies_forbidden_dialog_turn_count(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("мне скучно")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_forbidden_prev_scenario_timeout=0",
        "hw_gc_proactivity_timeout=0",
    )
    def test_proactivity_index_discuss_movies_forbidden_prev_scenario_timeout(
        self, alice
    ):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("мне скучно")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_forbidden_prev_scenarios=GeneralConversation;Video",
        "hw_gc_proactivity_timeout=0",
    )
    def test_proactivity_index_discuss_movies_forbidden_prev_scenarios(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("мне скучно")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_movie_discuss",
        "hw_gc_proactivity_movie_discuss_suggests",
    )
    def test_proactivity_index_discuss_movies_suggests(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("песнь моря")))
        acc.add(alice(voice("норм фильм")))
        acc.add(alice(voice("мне скучно")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_movie_discuss",
    )
    def test_proactivity_index_discuss_movies(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("песнь моря")))
        acc.add(alice(voice("норм фильм")))
        acc.add(alice(voice("мне скучно")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_movie_discuss",
        "bg_fresh_granet_form=alice.general_conversation.i_dont_know",
    )
    def test_proactivity_index_discuss_movies_i_dont_know(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("не помню")))
        acc.add(alice(voice("норм фильм")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_movie_discuss",
    )
    def test_proactivity_index_modal_discuss_movies(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай поболтаем")))
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("песнь моря")))
        acc.add(alice(voice("норм фильм")))
        acc.add(alice(voice("мне скучно")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_movie_discuss_specific",
    )
    def test_proactivity_index_discuss_movies_specific(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("норм фильм")))
        acc.add(alice(voice("мне скучно")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_movie_discuss_specific",
    )
    def test_proactivity_index_discuss_movies_specific_agree(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("давай")))
        acc.add(alice(voice("мне скучно")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_frame_proactivity",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gcp_proactivity_movie_discuss",
    )
    def test_proactivity_frame_discuss_movies(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("песнь моря")))
        acc.add(alice(voice("норм фильм")))
        acc.add(alice(voice("мне скучно")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_frame_proactivity",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gcp_proactivity_movie_discuss",
    )
    def test_proactivity_frame_modal_discuss_movies(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай поболтаем")))
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("песнь моря")))
        acc.add(alice(voice("норм фильм")))
        acc.add(alice(voice("мне скучно")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_frame_proactivity",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gcp_proactivity_movie_discuss_specific",
    )
    def test_proactivity_frame_discuss_movies_specific(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("норм фильм")))
        acc.add(alice(voice("мне скучно")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_frame_proactivity",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gcp_proactivity_movie_discuss_specific",
    )
    def test_proactivity_frame_modal_discuss_movies_specific(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай поболтаем")))
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("норм фильм")))
        acc.add(alice(voice("мне скучно")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_movie_discuss_specific",
        "hw_gc_proactivity_entity_search",
        "hw_gc_proactivity_entity_search_first",
        "hw_gc_proactivity_timeout=0",
    )
    def test_proactivity_index_discuss_movies_specific_entity_search(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("норм фильм")))
        acc.add(alice(voice("мне скучно")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_frame_proactivity",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gcp_proactivity_movie_discuss_specific",
        "hw_gc_proactivity_entity_search",
        "hw_gc_proactivity_entity_search_first",
        "hw_gc_proactivity_timeout=0",
        "hw_gc_proactivity_entity_search_crop=5",
    )
    def test_proactivity_frame_discuss_movies_specific_entity_search(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("норм фильм")))
        acc.add(alice(voice("мне скучно")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_movie_discuss",
        "hw_gc_entity_discussion_question_prob=1.0",
        "hw_gc_entity_discussion_question_suggest",
    )
    def test_proactivity_index_modal_discuss_movies_question(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай поболтаем")))
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("песнь моря")))
        acc.add(alice(voice("норм фильм")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_movie_discuss_specific",
        "hw_gc_entity_discussion_question_prob=1.0",
        "hw_gc_entity_discussion_question_suggest",
    )
    def test_proactivity_index_discuss_movies_specific_question(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("смотрел")))
        acc.add(alice(voice("норм фильм")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_frame_proactivity",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gcp_proactivity_movie_discuss",
        "hw_gc_entity_discussion_question_prob=1.0",
        "hw_gc_entity_discussion_question_suggest",
    )
    def test_proactivity_frame_modal_discuss_movies_question(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай поболтаем")))
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("песнь моря")))
        acc.add(alice(voice("норм фильм")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_frame_proactivity",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gcp_proactivity_movie_discuss_specific",
        "hw_gc_entity_discussion_question_prob=1.0",
        "hw_gc_entity_discussion_question_suggest",
    )
    def test_proactivity_frame_discuss_movies_specific_question(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("смотрел")))
        acc.add(alice(voice("норм фильм")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_frame_proactivity",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gcp_proactivity_movie_discuss_specific",
        "hw_gc_proactivity_movie_discuss_specific_wacthed_entity_boost=100",
    )
    def test_proactivity_frame_discuss_movies_specific_wached(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("смотрел")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_frame_proactivity",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gcp_proactivity_movie_discuss_specific",
        "hw_gc_proactivity_movie_discuss_specific_not_wacthed_entity_boost=100",
    )
    def test_proactivity_frame_discuss_movies_specific_not_wached(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("не смотрел")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_frame_proactivity",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gcp_proactivity_movie_discuss",
        "hw_gc_proactivity_movie_discuss_entity_boost=100",
    )
    def test_proactivity_frame_discuss_movies_boost(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("песнь моря")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_movie_akinator",
        "hw_gc_movie_akinator",
    )
    def test_proactivity_index_movie_akinator(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("давай")))
        acc.add(alice(voice("левые")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_movie_akinator",
        "hw_gc_movie_akinator",
    )
    def test_proactivity_index_modal_movie_akinator(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай поболтаем")))
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("давай")))
        acc.add(alice(voice("левые")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_frame_proactivity",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gcp_proactivity_movie_akinator",
        "hw_gc_movie_akinator",
    )
    def test_proactivity_frame_movie_akinator(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("давай")))
        acc.add(alice(voice("левые")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_frame_proactivity",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gcp_proactivity_movie_akinator",
        "hw_gc_movie_akinator",
    )
    def test_proactivity_frame_modal_movie_akinator(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай поболтаем")))
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("давай")))
        acc.add(alice(voice("левые")))
        return str(acc)


@pytest.mark.scenario(name="GeneralConversation", handle="general_conversation")
@pytest.mark.parametrize("surface", [surface.loudspeaker])
class TestGeneralConversationBaseProactivityQuasar:
    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_proactivity_forbidden_dialog_turn_count_less=0",
        "hw_gc_proactivity",
        "hw_gc_reply_ProactivityBoost=100",
        "hw_gc_force_proactivity_soft",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_proactivity_movie_discuss_specific",
        "hw_gc_movie_open_suggest_prob=1.0",
    )
    @pytest.mark.device_state({"is_tv_plugged_in": True})
    def test_proactivity_index_discuss_movies_specific_open(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("мне скучно")))
        acc.add(alice(voice("норм фильм")))
        return str(acc)
