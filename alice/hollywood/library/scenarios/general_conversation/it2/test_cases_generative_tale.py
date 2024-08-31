import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import voice


DEFAULT_EXPERIMENTS = (
    "bg_fresh_granet_form=alice.generative_tale",
)

MEMENTO_NEW_USER = {
    "UserConfigs": {
        "GenerativeTale": {
            "UsageCounter": 0
        }
    }
}

MEMENTO_OLD_USER = {
    "UserConfigs": {
        "GenerativeTale": {
            "UsageCounter": 2
        }
    }
}

MEMENTO_HAS_TALE = {
    'UserConfigs': {
        'GenerativeTale': {
            'TaleName': 'Сказка про Медведя',
            'TaleText': 'Жил был медведь.'
        }
    }
}

MEMENTO_HAS_NO_TALE = MEMENTO_NEW_USER


@pytest.fixture(scope="module")
def enabled_scenarios():
    return ["general_conversation"]


@pytest.mark.scenario(name="GeneralConversation", handle="general_conversation")
@pytest.mark.parametrize("surface", [surface.loudspeaker, surface.searchapp])
class TestGeneralConversationBaseGenerativeTale:
    @pytest.mark.experiments(*DEFAULT_EXPERIMENTS)
    def test_generative_tale_start(self, alice):
        r = alice(voice("придумай сказку"))
        assert r.scenario_stages() == {"run"}
        assert r.run_response.ResponseBody.AnalyticsInfo.Objects[0].GCResponseInfo.OriginalIntent == "alice.generative_tale.activate.generative_tale_ask_character"

    @pytest.mark.memento(MEMENTO_OLD_USER)
    @pytest.mark.experiments(*DEFAULT_EXPERIMENTS)
    @pytest.mark.experiments('hw_gc_render_general_conversation')
    def test_generative_tale_start_with_character(self, alice):
        r = alice(voice("придумай сказку про рыцаря"))
        assert "Начинаем" in r.continue_response.ResponseBody.Layout.OutputSpeech
        assert "Рыцар" in r.continue_response.ResponseBody.Layout.OutputSpeech
        directives = r.continue_response.ResponseBody.Layout.Directives
        for directive in directives:
            assert not directive.HasField('ShowViewDirective')

    @pytest.mark.memento(MEMENTO_OLD_USER)
    @pytest.mark.experiments(*DEFAULT_EXPERIMENTS)
    def test_generative_tale_full(self, alice):
        r = alice(voice("придумай сказку"))
        r = alice(voice("рыцарь"))
        assert r.scenario_stages() == {"run", "continue"}
        r = alice(voice("хватит"))

    @pytest.mark.memento(MEMENTO_OLD_USER)
    @pytest.mark.experiments(*DEFAULT_EXPERIMENTS)
    def test_generative_tale_politics(self, alice):
        r = alice(voice("придумай сказку"))

        r = alice(voice("про путина"))
        assert "не очень подходит" in r.run_response.ResponseBody.Layout.OutputSpeech
        assert "путин" not in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.scenario_stages() == {"run"}

        r = alice(voice("хватит"))

        r = alice(voice("придумай сказку про путина"))
        assert "не очень подходит" in r.run_response.ResponseBody.Layout.OutputSpeech
        assert r.scenario_stages() == {"run"}

        r = alice(voice("про путина"))
        assert "Снова за своё" in r.continue_response.ResponseBody.Layout.OutputSpeech
        assert "путин" not in r.continue_response.ResponseBody.Layout.OutputSpeech.lower()
        assert r.scenario_stages() == {"run", "continue"}

    @pytest.mark.memento(MEMENTO_OLD_USER)
    @pytest.mark.experiments(*DEFAULT_EXPERIMENTS)
    def test_generative_tale_force_exit(self, alice):
        r = alice(voice("придумай сказку про рыцаря"))

        r = alice(voice("включи музыку"))
        assert r.is_run_irrelevant()
        assert r.scenario_stages() == {"run"}

    @pytest.mark.memento(MEMENTO_OLD_USER)
    @pytest.mark.experiments(*DEFAULT_EXPERIMENTS)
    def test_generative_tale_timeout_exit(self, alice):
        r = alice(voice("придумай сказку про рыцаря"))
        assert not r.run_response.Features.IgnoresExpectedRequest
        assert "Рыцар" in r.continue_response.ResponseBody.Layout.OutputSpeech

        alice.skip(seconds=10 * 60 + 1)
        r = alice(voice("а дальше"))
        assert r.run_response.Features.IgnoresExpectedRequest
        assert r.scenario_stages() == {"run"}

    @pytest.mark.memento(MEMENTO_NEW_USER)
    @pytest.mark.experiments(*DEFAULT_EXPERIMENTS)
    def test_generative_tale_onboarding_new_user(self, alice):
        r = alice(voice("придумай сказку про рыцаря"))
        assert "Алиса, хватит" in r.continue_response.ResponseBody.Layout.OutputSpeech

        r = alice(voice("хватит"))

        r = alice(voice("придумай сказку"))
        assert "Алиса, хватит" in r.run_response.ResponseBody.Layout.OutputSpeech

        r = alice(voice("про рыцаря"))
        assert "Алиса, хватит" not in r.continue_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.memento(MEMENTO_OLD_USER)
    @pytest.mark.experiments(*DEFAULT_EXPERIMENTS)
    def test_generative_tale_onboarding_old_user(self, alice):
        r = alice(voice("придумай сказку про рыцаря"))
        assert "Алиса, хватит" not in r.continue_response.ResponseBody.Layout.OutputSpeech

        r = alice(voice("хватит"))

        r = alice(voice("придумай сказку"))
        assert "Алиса, хватит" not in r.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.memento(MEMENTO_OLD_USER)
    @pytest.mark.experiments(*DEFAULT_EXPERIMENTS)
    def test_generative_tale_generate_and_send_tale(self, alice):
        r = alice(voice("придумай сказку про рыцаря"))
        r = alice(voice("дальше"))
        r = alice(voice("дальше"))
        r = alice(voice("дальше"))
        r = alice(voice("отправить на телефон"))
        r = alice(voice("сказка про Рыцаря"))
        assert "Сказка про Рыцаря" in r.continue_response.ResponseBody.Layout.OutputSpeech, r.continue_response.ResponseBody.Layout

    @pytest.mark.memento(MEMENTO_HAS_NO_TALE)
    @pytest.mark.experiments(
        "hw_gc_enable_generative_tale",
        "bg_enable_generative_tale",
        "bg_fresh_granet",
        "bg_enable_gc_force_exit",
        "mm_postclassifier_gc_force_intents=alice.generative_tale",
    )
    def test_send_me_my_tale_without_tale(self, alice):
        response = alice(voice('пришли мне мою сказку'))
        assert response.scenario_stages() == {'run'}
        assert 'Извините, мы не сочиняли с вами сказок.' in response.run_response.ResponseBody.Layout.OutputSpeech

    @pytest.mark.memento(MEMENTO_HAS_TALE)
    @pytest.mark.experiments(
        "hw_gc_enable_generative_tale",
        "bg_enable_generative_tale",
        "bg_fresh_granet",
        "bg_enable_gc_force_exit",
        "mm_postclassifier_gc_force_intents=alice.generative_tale",
        "mm_enable_apphost_continue_scenarios"
    )
    def test_send_me_my_tale_with_tale(self, alice):
        response = alice(voice('пришли мне мою сказку'))
        assert response.scenario_stages() == {'run', 'continue'}
        assert 'Отправила сказку' in response.continue_response.ResponseBody.Layout.OutputSpeech


@pytest.mark.scenario(name="GeneralConversation", handle="general_conversation")
@pytest.mark.parametrize("surface", [surface.smart_display])
class TestGeneralConversationBaseGenerativeTaleSmartDisplay:
    @pytest.mark.memento(MEMENTO_OLD_USER)
    @pytest.mark.experiments(*DEFAULT_EXPERIMENTS)
    @pytest.mark.experiments('hw_gc_render_general_conversation')
    def test_generative_tale_start(self, alice):
        r = alice(voice("придумай сказку про рыцаря"))
        assert r.scenario_stages() == {"run", "continue"}
        response_body = r.continue_response.ResponseBody
        directives = response_body.Layout.Directives
        assert directives[0].HasField('ShowViewDirective')
        directive = directives[0].ShowViewDirective
        assert directive.CardId == 'general_conversation.scenario.div.card'

        assert directives[1].HasField('TtsPlayPlaceholderDirective')
