import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from voice import nlg


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', surface.actual_surfaces)
@pytest.mark.voice
class TestWhisper(object):
    owners = ('flimsywhimsy',)

    def test_turn_on(self, alice):
        response = alice('а давай шепотом поговорим')
        assert response.scenario == scenario.Voice
        assert response.intent == intent.WhisperTurnOn
        assert response.text == nlg.TURN_ON_TEXT
        assert response.output_speech_text == nlg.TURN_ON_VOICE

        # Check that whisper mode is enabled
        response = alice('что такое режим шепота')
        assert response.text == nlg.WHAT_IS_IT_TEXT
        assert response.output_speech_text == nlg.WHAT_IS_IT_VOICE

    def test_turn_off(self, alice):
        response = alice('перестань говорить шепотом')
        assert response.scenario == scenario.Voice
        assert response.intent == intent.WhisperTurnOff
        assert response.text == nlg.TURN_OFF
        assert response.output_speech_text == nlg.TURN_OFF

        # Check that whisper mode is disabled
        response = alice('что такое режим шепота')
        assert response.text == nlg.WHAT_IS_IT_WITH_PROACTIVITY_TEXT
        assert response.output_speech_text == nlg.WHAT_IS_IT_WITH_PROACTIVITY_VOICE

    def test_say_something(self, alice):
        response = alice('а скажи что-нибудь шепотом')
        assert response.scenario == scenario.Voice
        assert response.intent == intent.WhisperSaySomething
        assert response.text in nlg.SAY_SOMETHING_TEXT
        assert response.output_speech_text in nlg.SAY_SOMETHING_VOICE

    def test_what_is_it_no_proactivity(self, alice):
        response = alice('а давай шепотом поговорим')
        response = alice('что такое режим шепота')
        assert response.scenario == scenario.Voice
        assert response.intent == intent.WhisperWhatIsIt
        assert response.text == nlg.WHAT_IS_IT_TEXT
        assert response.output_speech_text == nlg.WHAT_IS_IT_VOICE

    def test_what_is_it_proactivity_agree(self, alice):
        response = alice('перестань говорить шепотом')
        response = alice('что такое режим шепота')
        assert response.scenario == scenario.Voice
        assert response.intent == intent.WhisperWhatIsIt
        assert response.text == nlg.WHAT_IS_IT_WITH_PROACTIVITY_TEXT
        assert response.output_speech_text == nlg.WHAT_IS_IT_WITH_PROACTIVITY_VOICE

        response = alice('давай')
        assert response.scenario == scenario.Voice
        assert response.intent == intent.WhisperTurnOn
        assert response.text == nlg.TURN_ON_TEXT
        assert response.output_speech_text == nlg.TURN_ON_VOICE

    def test_what_is_it_proactivity_disagree(self, alice):
        response = alice('перестань говорить шепотом')
        response = alice('что такое режим шепота')
        assert response.scenario == scenario.Voice
        assert response.intent == intent.WhisperWhatIsIt
        assert response.text == nlg.WHAT_IS_IT_WITH_PROACTIVITY_TEXT
        assert response.output_speech_text == nlg.WHAT_IS_IT_WITH_PROACTIVITY_VOICE

        response = alice('не надо')
        assert response.scenario == scenario.DoNothing
        assert not response.text
