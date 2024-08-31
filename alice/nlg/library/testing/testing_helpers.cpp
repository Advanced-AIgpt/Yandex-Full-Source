#include "testing_helpers.h"

#include <alice/nlg/library/nlg_renderer/create_nlg_renderer_from_register_function.h>
#include <alice/nlg/library/nlg_renderer/nlg_renderer.h>
#include <alice/nlg/library/runtime_api/translations.h>
#include <alice/library/util/rng.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NNlg::NTesting {

INlgRendererPtr CreateTestingNlgRenderer(TRegisterFunction registerFunction, IRng* rng) {
    TRng fallbackRng(5);
    if (!rng) {
        rng = &fallbackRng;
    }
    return CreateNlgRendererFromRegisterFunction(registerFunction, *rng);
}

INlgRendererPtr CreateTestingLocalizedNlgRenderer(TRegisterFunction registerFunction, const TFsPath& translationsJsonPath, IRng* rng) {
    TRng fallbackRng(5);
    if (!rng) {
        rng = &fallbackRng;
    }
    auto translations = CreateTranslationsContainerFromFile(translationsJsonPath);
    return CreateLocalizedNlgRendererFromRegisterFunction(registerFunction, translations, *rng);
}

TRenderPhraseResult GetRenderPhraseResult(INlgRenderer& nlgRenderer,
                                          TStringBuf templateId,
                                          TStringBuf phrase,
                                          IRng* rng,
                                          NJson::TJsonValue context,
                                          NJson::TJsonValue form,
                                          NJson::TJsonValue reqInfo,
                                          ELanguage language) {
    TRng fallbackRng(4);
    if (!rng) {
        rng = &fallbackRng;
    }
    return nlgRenderer.RenderPhrase(
            templateId,
            phrase,
            language,
            *rng,
            TRenderContextData {
                .Context = std::move(context),
                .Form = std::move(form),
                .ReqInfo = std::move(reqInfo),
            });
}

void TestPhraseTextVoice(INlgRenderer& nlgRenderer,
                         TStringBuf templateId,
                         TStringBuf phrase,
                         TStringBuf expectedText,
                         TStringBuf expectedVoice,
                         NJson::TJsonValue context,
                         NJson::TJsonValue form,
                         NJson::TJsonValue reqInfo,
                         ELanguage language) {
    const auto [text, voice] =
        GetRenderPhraseResult(nlgRenderer, templateId, phrase, nullptr,
                              std::move(context), std::move(form), std::move(reqInfo),
                              language);
    UNIT_ASSERT_VALUES_EQUAL_C(expectedText, text, text);
    UNIT_ASSERT_VALUES_EQUAL_C(expectedVoice, voice, voice);
}

void TestPhraseTextVoice(TRegisterFunction reg,
                         TStringBuf templateId,
                         TStringBuf phrase,
                         TStringBuf expectedText,
                         TStringBuf expectedVoice,
                         NJson::TJsonValue context,
                         NJson::TJsonValue form,
                         NJson::TJsonValue reqInfo,
                         ELanguage language) {
    const auto nlgRenderer = CreateTestingNlgRenderer(reg);
    TestPhraseTextVoice(*nlgRenderer, templateId, phrase, expectedText, expectedVoice, context, form, reqInfo, language);
}

void TestPhrase(INlgRenderer& nlgRenderer,
                TStringBuf templateId,
                TStringBuf phrase,
                TStringBuf expected,
                NJson::TJsonValue context,
                NJson::TJsonValue form,
                NJson::TJsonValue reqInfo,
                ELanguage language) {
    TestPhraseTextVoice(nlgRenderer, templateId, phrase, expected, expected, std::move(context), std::move(form), std::move(reqInfo), language);
}

void TestPhrase(TRegisterFunction reg,
                TStringBuf templateId,
                TStringBuf phrase,
                TStringBuf expected,
                NJson::TJsonValue context,
                NJson::TJsonValue form,
                NJson::TJsonValue reqInfo,
                ELanguage language) {
    TestPhraseTextVoice(reg, templateId, phrase, expected, expected, std::move(context), std::move(form), std::move(reqInfo), language);
}

void CheckChoiceFreqs(TRegisterFunction reg,
                      TStringBuf templateId,
                      TStringBuf phrase,
                      const TVector<std::pair<TString, double>>& expectedProb,
                      NJson::TJsonValue context,
                      NJson::TJsonValue form,
                      NJson::TJsonValue reqInfo,
                      ELanguage language) {
    THashMap<TString, size_t> counts;

    constexpr size_t numSamples = 100;
    size_t intSample = 0;
    size_t doubleSample = 0;
    TFakeRng rng([&doubleSample]() { return doubleSample++ / static_cast<double>(numSamples); },
                 [&intSample]() { return intSample++; });
    const auto nlgRenderer = CreateTestingNlgRenderer(reg);

    for (size_t i = 0; i < numSamples; ++i) {
        const auto out = GetRenderPhraseResult(*nlgRenderer, templateId, phrase, &rng,
                                               context, form, reqInfo, language);
        UNIT_ASSERT_VALUES_EQUAL(out.Text, out.Voice);
        ++counts[out.Text];
    }

    auto getFreq = [&counts](TStringBuf sample) {
        size_t count = 0;
        if (auto* sampleCount = counts.FindPtr(sample)) {
            count = *sampleCount;
        }

        return count / static_cast<double>(numSamples);
    };

    constexpr double epsilon = 0.05; // arbitrary small epsilon
    for (auto& [sample, prob] : expectedProb) {
        auto freq = getFreq(sample);
        UNIT_ASSERT_C(std::abs(prob - freq) < epsilon,
                      "sample = " << sample << ", prob = " << prob << ", freq = " << freq);
    }
}

}  // namespace NAlice::NNlg
