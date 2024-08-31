import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.scenario_responses import Accumulator


@pytest.fixture(scope="module")
def enabled_scenarios():
    return ["general_conversation"]


@pytest.mark.scenario(name="GeneralConversation", handle="general_conversation")
@pytest.mark.parametrize("surface", [surface.searchapp])
class TestGeneralConversationBaseLetsDiscuss:
    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_lets_discuss_movie_frames",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_lets_discuss_movie_frames",
    )
    def test_lets_discuss_movie_specific(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай обсудим фильм матрица")))
        acc.add(alice(voice("мне он понравился")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_lets_discuss_movie_frames",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_lets_discuss_movie_frames",
        "hw_gc_proactivity_movie_discuss_entity_boost=100",
    )
    def test_lets_discuss_movie_specific_boost(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай обсудим фильм матрица")))
        acc.add(alice(voice("песнь моря")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_lets_discuss_movie_frames",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_lets_discuss_movie_frames",
    )
    def test_lets_discuss_movie_specific_modal(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай поболтаем")))
        acc.add(alice(voice("давай обсудим фильм матрица")))
        acc.add(alice(voice("мне он понравился")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_lets_discuss_movie_frames",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_lets_discuss_movie_frames",
    )
    def test_lets_discuss_some_movie(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай обсудим мультфильм")))
        acc.add(alice(voice("о чем он")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_lets_discuss_movie_frames",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_lets_discuss_movie_frames",
    )
    def test_lets_discuss_some_movie_modal(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай поболтаем")))
        acc.add(alice(voice("давай обсудим мультфильм")))
        acc.add(alice(voice("о чем он")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_lets_discuss_movie_frames",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_lets_discuss_movie_frames",
    )
    def test_lets_discuss_movie_change_entity(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай обсудим фильм матрица")))
        acc.add(alice(voice("мне он понравился")))
        acc.add(alice(voice("интересно увидеть продолжение")))
        acc.add(alice(voice("давай обсудим другой фильм")))
        acc.add(alice(voice("о чем он")))
        acc.add(alice(voice("давай обсудим мультфильм")))
        acc.add(alice(voice("а мне нравится рик и морти")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_lets_discuss_movie_frames",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_lets_discuss_movie_frames",
        "hw_gc_entity_discussion_question_prob=1.0",
    )
    def test_lets_discuss_movie_specific_question(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай обсудим фильм матрица")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_lets_discuss_movie_frames",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_lets_discuss_movie_frames",
        "hw_gc_movie_open_suggest_prob=1.0",
    )
    def test_lets_discuss_movie_no_open_suggest_on_wrong_device(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай обсудим фильм матрица")))
        return str(acc)


@pytest.mark.scenario(name="GeneralConversation", handle="general_conversation")
@pytest.mark.parametrize("surface", [surface.loudspeaker])
class TestGeneralConversationBaseLetsDiscussQuasar:
    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_lets_discuss_movie_frames",
        "hw_gc_reply_EntityBoost=15",
        "hw_gc_entity_index",
        "hw_gc_force_entity_soft",
        "hw_gc_lets_discuss_movie_frames",
        "hw_gc_movie_open_suggest_prob=1.0",
    )
    @pytest.mark.device_state({"is_tv_plugged_in": True})
    def test_lets_discuss_movie_open_suggest(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("давай обсудим фильм матрица")))
        return str(acc)
