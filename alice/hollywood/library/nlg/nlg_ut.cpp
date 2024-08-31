#include "nlg.h"

#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/nlg/templates/register.h>
#include <alice/library/logger/logger.h>
#include <alice/library/util/rng.h>
#include <library/cpp/testing/unittest/registar.h>

#include <apphost/lib/service_testing/service_testing.h>

using namespace NAlice;
using namespace NAlice::NHollywood;

namespace {

void CheckPhraseTextVoice(const TStringBuf expectedText, const TStringBuf expectedVoice, const NAlice::NNlg::TRenderPhraseResult& actual) {
    UNIT_ASSERT_VALUES_EQUAL(expectedText, actual.Text);
    UNIT_ASSERT_VALUES_EQUAL(expectedVoice, actual.Voice);
}

void CheckPhrase(const TStringBuf expected, const NAlice::NNlg::TRenderPhraseResult& actual) {
    CheckPhraseTextVoice(expected, expected, actual);
}

void Register(NAlice::NNlg::TEnvironment& env) {
    NAlice::NHollywood::NLibrary::NNlg::NTemplates::RegisterAll(env);
}

NAlice::TFakeRng RNG;
TCompiledNlgComponent NLG(RNG, nullptr, &Register);

} // namespace

Y_UNIT_TEST_SUITE(Nlg) {
    Y_UNIT_TEST(RespectsLanguage) {
        NAlice::NScenarios::TScenarioBaseRequest request;
        NAppHost::NService::TTestContext ctx;
        TScenarioBaseRequestWrapper requestWrapper(request, ctx);
        TNlgData nlgData(TRTLogger::NullLogger(), requestWrapper);

        nlgData.Context["hello"] = 123;

        CheckPhrase(
            "123, got it!",
            NLG.RenderPhrase("hw_test", "render_result", ELanguage::LANG_ENG, RNG, nlgData)
        );
        CheckPhrase(
            "123, принято!",
            NLG.RenderPhrase("hw_test", "render_result", ELanguage::LANG_RUS, RNG, nlgData)
        );
    }

    Y_UNIT_TEST(ArabicAddSpeaker) {
        NAlice::NScenarios::TScenarioBaseRequest request;
        NAppHost::NService::TTestContext ctx;
        TScenarioBaseRequestWrapper requestWrapper(request, ctx);

        auto nlgWrapper = TNlgWrapper::Create(NLG, requestWrapper, RNG, ELanguage::LANG_ARA);

        TNlgData nlgData(TRTLogger::NullLogger());
        nlgData.Context["hello"] = 123;

        CheckPhraseTextVoice(
            "123, نص عادي. "
            "123, النص مع الصوت.",

            "<speaker voice=\"arabic.gpu\" lang=\"ar\">123, نص عادي. "
            "<speaker audio=\"valtz\">123, النص مع الصوت.",

            nlgWrapper.RenderPhrase("hw_test", "render_result", nlgData)
        );

    }
}
