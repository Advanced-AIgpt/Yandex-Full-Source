import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope="module")
def enabled_scenarios():
    return ['general_conversation']


def get_gc_response_info(response):
    return response.run_response.ResponseBody.AnalyticsInfo.Objects[0].GCResponseInfo


@pytest.mark.scenario(name='GeneralConversationHeavy', handle='general_conversation')
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestGeneralConversationHeavyBase:
    @pytest.mark.experiments('hw_gc_enable_gc_heavy_scanrio_classification')
    def test_default(self, alice):
        r = alice(voice('давай поболтаем'))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.Features.IsIrrelevant
        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.Features.IsIrrelevant

    @pytest.mark.experiments('hw_gc_disable_nlgsearch_reply', 'hw_gc_seq2seq_url=/generative', 'hw_gc_enable_gc_heavy_scanrio_classification')
    def test_aggregated_reply_seq2seq_only_with_seq2seq_url(self, alice):
        r = alice(voice('давай поболтаем'))
        assert r.scenario_stages() == {'run'}
        assert not get_gc_response_info(r).IsAggregatedRequest
        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert get_gc_response_info(r).IsAggregatedRequest
        assert get_gc_response_info(r).Source == 'seq2seq'

    @pytest.mark.experiments("hw_gc_disable_movie_discussions_by_default", 'hw_gc_enable_gc_heavy_scanrio_classification')
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


@pytest.mark.scenario(name='GeneralConversation', handle='general_conversation')
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestGeneralConversationBase:
    @pytest.mark.experiments('hw_gc_enable_gc_heavy_scanrio_classification')
    def test_pure_without_search(self, alice):
        r = alice(voice('ты китик'))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.Features.IsIrrelevant
        r = alice(voice('давай поболтаем'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.Features.IsIrrelevant
