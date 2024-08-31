#pragma once

#include <alice/nlg/library/nlg_renderer/fwd.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/langs/langs.h>
#include <util/folder/path.h>

namespace NAlice {
struct IRng;
} // namespace NAlice

namespace NAlice::NNlg::NTesting {

INlgRendererPtr CreateTestingNlgRenderer(TRegisterFunction registerFunction, IRng* rng = nullptr);
INlgRendererPtr CreateTestingLocalizedNlgRenderer(TRegisterFunction registerFunction, const TFsPath& translationsJsonPath, IRng* rng = nullptr);

TRenderPhraseResult GetRenderPhraseResult(INlgRenderer& nlgRenderer,
                                          TStringBuf templateId,
                                          TStringBuf phrase,
                                          IRng* rng = nullptr,
                                          NJson::TJsonValue context = NJson::TJsonMap(),
                                          NJson::TJsonValue form = NJson::TJsonMap(),
                                          NJson::TJsonValue reqInfo = NJson::TJsonMap(),
                                          ELanguage language = ELanguage::LANG_RUS);

void TestPhraseTextVoice(INlgRenderer& nlgRenderer,
                         TStringBuf templateId,
                         TStringBuf phrase,
                         TStringBuf expectedText,
                         TStringBuf expectedVoice,
                         NJson::TJsonValue context = NJson::TJsonMap(),
                         NJson::TJsonValue form = NJson::TJsonMap(),
                         NJson::TJsonValue reqInfo = NJson::TJsonMap(),
                         ELanguage language = ELanguage::LANG_RUS);

void TestPhraseTextVoice(TRegisterFunction reg,
                         TStringBuf templateId,
                         TStringBuf phrase,
                         TStringBuf expectedText,
                         TStringBuf expectedVoice,
                         NJson::TJsonValue context = NJson::TJsonMap(),
                         NJson::TJsonValue form = NJson::TJsonMap(),
                         NJson::TJsonValue reqInfo = NJson::TJsonMap(),
                         ELanguage language = ELanguage::LANG_RUS);

void TestPhrase(INlgRenderer& nlgRenderer,
                TStringBuf templateId,
                TStringBuf phrase,
                TStringBuf expected,
                NJson::TJsonValue context = NJson::TJsonMap(),
                NJson::TJsonValue form = NJson::TJsonMap(),
                NJson::TJsonValue reqInfo = NJson::TJsonMap(),
                ELanguage language = ELanguage::LANG_RUS);

void TestPhrase(TRegisterFunction reg,
                TStringBuf templateId,
                TStringBuf phrase,
                TStringBuf expected,
                NJson::TJsonValue context = NJson::TJsonMap(),
                NJson::TJsonValue form = NJson::TJsonMap(),
                NJson::TJsonValue reqInfo = NJson::TJsonMap(),
                ELanguage language = ELanguage::LANG_RUS);

void CheckChoiceFreqs(TRegisterFunction reg,
                      TStringBuf templateId,
                      TStringBuf phrase,
                      const TVector<std::pair<TString, double>>& expectedProb,
                      NJson::TJsonValue context = NJson::TJsonMap(),
                      NJson::TJsonValue form = NJson::TJsonMap(),
                      NJson::TJsonValue reqInfo = NJson::TJsonMap(),
                      ELanguage language = ELanguage::LANG_RUS);

}  // namespace NAlice::NNlg
