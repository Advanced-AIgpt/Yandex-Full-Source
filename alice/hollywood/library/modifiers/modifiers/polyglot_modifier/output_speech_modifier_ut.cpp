#include "output_speech_modifier.h"
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NHollywood::NModifiers;

namespace {

Y_UNIT_TEST_SUITE(OutputSpeechModifier) {
    Y_UNIT_TEST(ThrowsOnWrongInput) {
        UNIT_ASSERT_EXCEPTION(TOutputSpeechModifier("<speaker lang=\"en>"), yexception);
        const auto outputSpeechModifier = TOutputSpeechModifier("<speaker lang=\"en\">");
        auto outputSpeechWithError = TString("my text <speaker lang=\"en\" without closing angle brackets");
        UNIT_ASSERT_EXCEPTION(outputSpeechModifier.ModifyOutputSpeech(outputSpeechWithError), yexception);
    }

    Y_UNIT_TEST(ModifyPlainSpeechTest) {
        const auto outputSpeechModifier = TOutputSpeechModifier("<speaker voice=\"oksana\" lang=\"en\">");
        auto outputSpeech = TString("simple phrase without tags");
        outputSpeechModifier.ModifyOutputSpeech(outputSpeech);
        UNIT_ASSERT_EQUAL(outputSpeech, TString("<speaker voice=\"oksana\" lang=\"en\">simple phrase without tags"));
    }

    Y_UNIT_TEST(ReplacementTest) {
        const auto outputSpeechModifier = TOutputSpeechModifier("<speaker voice=\"oksana\" lang=\"en\">");
        auto outputSpeech = TString(
            "First part"
            "<speaker voice=\"shitova\" emotion=\"phlegmatic\">Second part");
        outputSpeechModifier.ModifyOutputSpeech(outputSpeech);
        UNIT_ASSERT_EQUAL(outputSpeech, TString(
            "<speaker voice=\"oksana\" lang=\"en\">First part"
            "<speaker voice=\"oksana\" lang=\"en\" emotion=\"phlegmatic\">Second part"));
    }

    Y_UNIT_TEST(NonVoiceSpeakerTest) {
        const auto outputSpeechModifier = TOutputSpeechModifier("<speaker voice=\"oksana\" lang=\"en\">");
        auto outputSpeech = TString(
            "<speaker background=\"test_background\">"
            "First part"
            "<speaker audio=\"test_audio.opus\">Second part");
        outputSpeechModifier.ModifyOutputSpeech(outputSpeech);
        UNIT_ASSERT_EQUAL(outputSpeech, TString(
            "<speaker background=\"test_background\">"
            "<speaker voice=\"oksana\" lang=\"en\">First part"
            "<speaker audio=\"test_audio.opus\">Second part"));
    }
}

} // namespace
