import base64

import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope="module")
def enabled_scenarios():
    return ['general_conversation']


def get_gc_response_info(response):
    return response.run_response.ResponseBody.AnalyticsInfo.Objects[0].GCResponseInfo


@pytest.mark.scenario(name='GeneralConversation', handle='general_conversation')
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestGeneralConversationBase:
    @pytest.mark.experiments('hw_gc_classifier_score_threshold_speaker=1.0')
    @pytest.mark.experiments('hw_gc_render_general_conversation')
    def test_default(self, alice):
        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert not get_gc_response_info(r).IsAggregatedRequest

        directives = r.run_response.ResponseBody.Layout.Directives
        for directive in directives:
            assert not directive.HasField('ShowViewDirective')

    @pytest.mark.experiments('hw_gc_classifier_score_threshold_speaker=0.0')
    def test_aggregated_reply(self, alice):
        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).IsAggregatedRequest

        r = alice(voice('давай поболтаем'))
        assert r.scenario_stages() == {'run'}
        assert not get_gc_response_info(r).IsAggregatedRequest

        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).IsAggregatedRequest

    @pytest.mark.experiments('hw_gc_disable_aggregated_reply_in_modal_mode')
    @pytest.mark.experiments('hw_gc_classifier_score_threshold_speaker=1.0')
    def test_disable_aggregated_reply_in_modal_mode(self, alice):
        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert not get_gc_response_info(r).IsAggregatedRequest

        r = alice(voice('давай поболтаем'))
        assert r.scenario_stages() == {'run'}
        assert not get_gc_response_info(r).IsAggregatedRequest

        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert not get_gc_response_info(r).IsAggregatedRequest

    @pytest.mark.experiments('hw_gc_disable_seq2seq_reply')
    @pytest.mark.experiments('hw_gc_classifier_score_threshold_speaker=1.0')
    def test_aggregated_reply_nlgsearch_only(self, alice):
        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert not get_gc_response_info(r).IsAggregatedRequest

        r = alice(voice('давай поболтаем'))
        assert r.scenario_stages() == {'run'}
        assert not get_gc_response_info(r).IsAggregatedRequest

        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).IsAggregatedRequest
        assert get_gc_response_info(r).Source != 'seq2seq'

    @pytest.mark.experiments('hw_gc_disable_nlgsearch_reply')
    @pytest.mark.experiments('hw_gc_classifier_score_threshold_speaker=1.0')
    def test_aggregated_reply_seq2seq_only(self, alice):
        r = alice(voice('давай поболтаем'))
        assert r.scenario_stages() == {'run'}
        assert not get_gc_response_info(r).IsAggregatedRequest

        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).IsAggregatedRequest
        assert get_gc_response_info(r).Source == 'seq2seq'

    @pytest.mark.experiments('hw_gc_disable_nlgsearch_reply', 'hw_gc_seq2seq_url=/generative')
    @pytest.mark.experiments('hw_gc_classifier_score_threshold_speaker=1.0')
    def test_aggregated_reply_seq2seq_only_with_seq2seq_url(self, alice):
        r = alice(voice('давай поболтаем'))
        assert r.scenario_stages() == {'run'}
        assert not get_gc_response_info(r).IsAggregatedRequest

        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).IsAggregatedRequest
        assert get_gc_response_info(r).Source == 'seq2seq'

    @pytest.mark.experiments('hw_gc_disable_fraud_microintents')
    def test_fraud_microintents(self, alice):
        r = alice(voice('когда будешь уметь'))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).Intent != "personal_assistant.handcrafted.fraud_request_do_not_know"
        r = alice(voice('давай поболтаем'))
        assert r.scenario_stages() == {'run'}
        r = alice(voice('когда будешь уметь'))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).Intent != "personal_assistant.handcrafted.fraud_request_do_not_know"

    def test_fraud_microintents_without_flag(self, alice):
        r = alice(voice('когда будешь уметь'))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).Intent == "personal_assistant.handcrafted.fraud_request_do_not_know"

        r = alice(voice('давай поболтаем'))
        assert r.scenario_stages() == {'run'}
        r = alice(voice('когда будешь уметь'))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).Intent != "personal_assistant.handcrafted.fraud_request_do_not_know"

    def test_microintent_suggests(self, alice):
        r = alice(voice('как тебя зовут'))
        assert len(r.run_response.ResponseBody.Layout.SuggestButtons) == 10
        assert r.scenario_stages() == {'run'}
        r = alice(voice('давай поболтаем'))
        assert r.scenario_stages() == {'run'}
        r = alice(voice('как тебя зовут'))
        assert len(r.run_response.ResponseBody.Layout.SuggestButtons) == 2
        assert r.scenario_stages() == {'run'}

    def test_microintents_should_listen(self, alice):
        r = alice(voice('как тебя зовут'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.ShouldListen
        r = alice(voice('спокойной ночи'))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.ResponseBody.Layout.ShouldListen

    @pytest.mark.experiments('hw_gc_force_pure_gc')
    def test_force_pure_gc(self, alice):
        r = alice(voice('ты китик'))
        assert get_gc_response_info(r).Intent == "personal_assistant.scenarios.external_skill_gc"

    @pytest.mark.experiments('hw_gc_disable_banlist')
    def test_local_banlist(self, alice):
        r = alice(voice('Путин'))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).OriginalIntent == "alice.fixlist.gc_request_banlist.general_conversation_dummy"
        r = alice(voice('девушки'))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).OriginalIntent != "alice.fixlist.gc_request_banlist.general_conversation_dummy"

    @pytest.mark.experiments("hw_gc_enable_birthday_microintents")
    def test_birthday_microintents(self, alice):
        assert get_gc_response_info(alice(voice("Загадай желание"))).Intent == "personal_assistant.handcrafted.make_a_wish"

    def test_birthday_microintents_without_flag(self, alice):
        assert get_gc_response_info(alice(voice("Загадай желание"))).Intent != "personal_assistant.handcrafted.make_a_wish"

    def test_emotional_tts_in_microintents(self, alice):
        r = alice(voice("Привет"))
        assert r.run_response.ResponseBody.Layout.OutputSpeech.startswith("<speaker voice=\"shitova.gpu\" emotion=\"energetic\">")
        assert not r.run_response.ResponseBody.Layout.Cards[0].Text.startswith("<speaker voice=\"shitova.gpu\" emotion=\"energetic\">")

    def test_emotional_tts_classifier(self, alice):
        alice(voice("давай поболтаем"))
        r = alice(voice("я тебя очень люблю. a ты меня?"))
        assert r.run_response.ResponseBody.Layout.OutputSpeech.startswith("<speaker voice=\"shitova.gpu\" emotion=\"energetic\">")
        assert not r.run_response.ResponseBody.Layout.Cards[0].Text.startswith("<speaker voice=\"shitova.gpu\" emotion=\"energetic\">")

    @pytest.mark.experiments("hw_gc_set_tts_speed[pure]=1.5")
    def test_tts_speed_flag(self, alice):
        alice(voice("давай поболтаем"))
        r = alice(voice("как дела?"))
        assert r.run_response.ResponseBody.Layout.OutputSpeech.startswith("<speaker speed=\"1.5\">")
        assert not r.run_response.ResponseBody.Layout.Cards[0].Text.startswith("<speaker speed=\"1.5\">")
        r = alice(voice("как тебе котики?"))
        assert r.run_response.ResponseBody.Layout.OutputSpeech.startswith("<speaker speed=\"1.5\">")
        assert not r.run_response.ResponseBody.Layout.Cards[0].Text.startswith("<speaker speed=\"1.5\">")

    @pytest.mark.experiments("hw_gc_disable_emotional_tts_classifier")
    def test_no_emotional_tts_classifier(self, alice):
        alice(voice("давай поболтаем"))
        r = alice(voice("я тебя люблю"))
        assert not r.run_response.ResponseBody.Layout.OutputSpeech.startswith("<speaker voice=\"shitova.gpu\" emotion=\"energetic\">")

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_mocked_reply[pure]={}".format(
            base64.b64encode("ответ".encode("utf-8")).decode("utf-8")
        ),
    )
    def test_mock_answer_pure(self, alice):
        r = alice(voice("что ты думаешь о котиках"))
        assert r.run_response.ResponseBody.Layout.Cards[0].Text != "Ответ"
        alice(voice("давай поболтаем"))
        r = alice(voice("что ты думаешь о котиках"))
        assert r.run_response.ResponseBody.Layout.Cards[0].Text == "Ответ"

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_mocked_reply[general]={}".format(
            base64.b64encode("ответ".encode("utf-8")).decode("utf-8")
        ),
    )
    def test_mock_answer_general(self, alice):
        r = alice(voice("что ты думаешь о котиках"))
        assert r.run_response.ResponseBody.Layout.Cards[0].Text == "Ответ"
        alice(voice("давай поболтаем"))
        r = alice(voice("что ты думаешь о котиках"))
        assert r.run_response.ResponseBody.Layout.Cards[0].Text != "Ответ"

    @pytest.mark.experiments(
        "hw_gc_disable_movie_discussions_by_default",
        "hw_gc_mocked_reply[all]={}".format(
            base64.b64encode("ответ".encode("utf-8")).decode("utf-8")
        ),
    )
    def test_mock_answer_all(self, alice):
        r = alice(voice("что ты думаешь о котиках"))
        assert r.run_response.ResponseBody.Layout.Cards[0].Text == "Ответ"
        alice(voice("давай поболтаем"))
        r = alice(voice("что ты думаешь о котиках"))
        assert r.run_response.ResponseBody.Layout.Cards[0].Text == "Ответ"


@pytest.mark.scenario(name='GeneralConversation', handle='general_conversation')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestGeneralConversationBaseSearchapp:
    @pytest.mark.experiments('hw_gc_classifier_score_threshold=1.0')
    @pytest.mark.experiments('hw_gc_render_general_conversation')
    def test_default(self, alice):
        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert not get_gc_response_info(r).IsAggregatedRequest

        directives = r.run_response.ResponseBody.Layout.Directives
        for directive in directives:
            assert not directive.HasField('ShowViewDirective')

    @pytest.mark.experiments('hw_gc_classifier_score_threshold=0.0')
    def test_aggregated_reply(self, alice):
        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).IsAggregatedRequest

        r = alice(voice('давай поболтаем'))
        assert r.scenario_stages() == {'run'}
        assert not get_gc_response_info(r).IsAggregatedRequest

        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).IsAggregatedRequest

    def test_fraud_microintents_without_flag(self, alice):
        r = alice(voice('когда будешь уметь'))
        assert get_gc_response_info(r).Intent == "personal_assistant.handcrafted.fraud_request_do_not_know"
        alice(voice('давай поболтаем'))
        r = alice(voice('когда будешь уметь'))
        assert get_gc_response_info(r).Intent != "personal_assistant.handcrafted.fraud_request_do_not_know"

    @pytest.mark.experiments('hw_gc_enable_microintents_outside_modal')
    def test_microintent_suggests(self, alice):
        r = alice(voice('как тебя зовут'))
        assert len(r.run_response.ResponseBody.Layout.SuggestButtons) == 11
        assert r.scenario_stages() == {'run'}
        alice(voice('давай поболтаем'))
        r = alice(voice('как тебя зовут'))
        assert len(r.run_response.ResponseBody.Layout.SuggestButtons) == 2
        assert r.scenario_stages() == {'run'}

    @pytest.mark.experiments('hw_gc_enable_microintents_outside_modal')
    def test_microintents_should_listen(self, alice):
        r = alice(voice('как тебя зовут'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.ShouldListen
        r = alice(voice('спокойной ночи'))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.ResponseBody.Layout.ShouldListen

    @pytest.mark.experiments('hw_gc_enable_microintents_outside_modal')
    def test_ellipsis_microintents(self, alice):
        alice(voice('расскажи анекдот'))
        r = alice(voice('расскажи еще'))
        assert get_gc_response_info(r).Intent == "personal_assistant.handcrafted.tell_me_another"

    @pytest.mark.experiments('hw_gc_enable_microintents_outside_modal')
    def test_pure_ellipsis_microintents(self, alice):
        alice(voice('давай поболтаем'))
        alice(voice('расскажи анекдот'))
        r = alice(voice('расскажи еще'))
        assert get_gc_response_info(r).OriginalIntent == "alice.microintents.tell_me_another"

    @pytest.mark.experiments('hw_gc_disable_pure_gc')
    def test_disable_pure_gc(self, alice):
        r = alice(voice('давай поболтаем'))
        assert get_gc_response_info(r).Intent != "personal_assistant.scenarios.external_skill_gc"
        r = alice(voice('как дела'))
        assert get_gc_response_info(r).Intent != "personal_assistant.scenarios.external_skill_gc"

    @pytest.mark.experiments('hw_gc_enable_microintents_outside_modal', 'hw_gc_disable_pure_gc', 'hw_gc_disable_let_us_talk_microintent')
    def test_disable_let_us_talk(self, alice):
        r = alice(voice('давай поболтаем'))
        assert get_gc_response_info(r).Intent != "personal_assistant.handcrafted.let_us_talk"

    @pytest.mark.experiments("hw_gc_enable_modality_in_pure_gc")
    def test_enable_modality_in_pure_gc(self, alice):
        alice(voice("давай поболтаем"))
        r = alice(voice("что ты думаешь о котиках"))
        assert r.run_response.ResponseBody.ExpectsRequest

    def test_no_modality_in_pure_gc(self, alice):
        alice(voice("давай поболтаем"))
        r = alice(voice("что ты думаешь о котиках"))
        assert not r.run_response.ResponseBody.ExpectsRequest


@pytest.mark.scenario(name='GeneralConversation', handle='general_conversation')
@pytest.mark.parametrize('surface', [surface.smart_display])
@pytest.mark.experiments('hw_gc_render_general_conversation')
class TestGeneralConversationBaseSmartDisplay:
    def test_default(self, alice):
        r = alice(voice('давай поболтаем'))
        assert r.scenario_stages() == {'run'}
        response_body = r.run_response.ResponseBody
        directives = response_body.Layout.Directives
        assert directives[0].HasField('ShowViewDirective')
        directive = directives[0].ShowViewDirective
        assert directive.CardId == 'general_conversation.scenario.div.card'

        assert directives[1].HasField('TtsPlayPlaceholderDirective')
