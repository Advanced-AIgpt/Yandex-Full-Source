import base64

import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.scenario_responses import Accumulator


@pytest.fixture(scope="module")
def enabled_scenarios():
    return ["general_conversation"]


def get_gc_response_info(response):
    return response.run_response.ResponseBody.AnalyticsInfo.Objects[0].GCResponseInfo


@pytest.mark.scenario(name="GeneralConversation", handle="general_conversation")
@pytest.mark.parametrize("surface", surface.actual_surfaces)
class TestGeneralConversationBaseAllApps:
    @pytest.mark.experiments("hw_gc_disable_movie_discussions_by_default")
    def test_ty_kitik(self, alice):
        r = alice(voice("ты китик"))
        assert get_gc_response_info(r).Intent == "personal_assistant.general_conversation.general_conversation"

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default", "alice_birthday"
    )
    def test_dont_understand_alice_birthday(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("что ты думаешь о путине")))
        return str(acc)

    @pytest.mark.experiments("hw_gc_disable_movie_discussions_by_default")
    def test_pure_gc(self, alice):
        r = alice(voice("давай поболтаем"))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).OriginalIntent == "alice.general_conversation.pure_gc_activate"
        r = alice(voice("как тебя зовут"))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).OriginalIntent == "alice.microintents.what_is_your_name"
        r = alice(voice("что ты думаешь о путине"))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).OriginalIntent == "alice.fixlist.gc_request_banlist.general_conversation_dummy"
        r = alice(voice("какая сейчас погода"))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).OriginalIntent == "alice.general_conversation.general_conversation"
        r = alice(voice("хватит болтать"))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).OriginalIntent == "alice.general_conversation.pure_gc_deactivate"
        r = alice(voice("нормально"))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).OriginalIntent == "alice.gc_feedback.neutral"

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default", "hw_gc_force_search_gif_soft"
    )
    def test_no_gif_in_answer(self, alice):
        r = alice(voice("ты китик"))
        assert len(r.run_response.ResponseBody.Layout.Cards) == 1

    @pytest.mark.experiments("hw_gc_debug_disable_search_suggests")
    def test_child_mode(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("как дела")))
        return str(acc)

    @pytest.mark.experiments("hw_gc_disable_child_replies")
    def test_no_child_mode(self, alice):
        acc = Accumulator()
        acc.add(alice(voice("как дела")))
        return str(acc)

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_mocked_reply={}".format(
            base64.b64encode("ответ".encode("utf-8")).decode("utf-8")
        ),
    )
    def test_mock_answer(self, alice):
        r = alice(voice("что ты думаешь о котиках"))
        assert r.run_response.ResponseBody.Layout.Cards[0].Text == "Ответ"
        r = alice(voice("давай поболтаем"))
        assert r.run_response.ResponseBody.Layout.Cards[0].Text != "Ответ"
        r = alice(voice("что ты думаешь о путине"))
        assert r.run_response.ResponseBody.Layout.Cards[0].Text != "Ответ"
        r = alice(voice("какая сейчас погода"))
        assert r.run_response.ResponseBody.Layout.Cards[0].Text == "Ответ"
        r = alice(voice("хватит болтать"))
        assert r.run_response.ResponseBody.Layout.Cards[0].Text != "Ответ"
        r = alice(voice("нормально"))
        assert r.run_response.ResponseBody.Layout.Cards[0].Text != "Ответ"


@pytest.mark.scenario(name="GeneralConversation", handle="general_conversation")
@pytest.mark.parametrize("surface", [surface.station_pro])
class TestGeneralConversationBaseStationPro:
    @pytest.mark.supported_features("led_display")
    def test_led_image(self, alice):
        r = alice(voice("как дела"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 2
        assert directives[0].HasField('ForceDisplayCardsDirective')
        assert directives[1].HasField('DrawLedScreenDirective')
        r = alice(voice("я тебя люблю"))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 2
        assert directives[0].HasField('ForceDisplayCardsDirective')
        assert directives[1].HasField('DrawLedScreenDirective')


@pytest.mark.scenario(name="GeneralConversation", handle="general_conversation")
@pytest.mark.supported_features('scled_display')
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestGeneralConversationBaseScled:
    def test_scled_image(self, alice):
        for request in ["ты китик", "как дела"]:
            r = alice(voice(request))
            directives = r.run_response.ResponseBody.Layout.Directives
            assert len(directives) == 2
            assert directives[0].HasField('DrawScledAnimationsDirective')
            assert directives[1].HasField('TtsPlayPlaceholderDirective')
