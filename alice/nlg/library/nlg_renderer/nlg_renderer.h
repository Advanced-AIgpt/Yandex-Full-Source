#pragma once

#include "fwd.h"

#include <library/cpp/json/json_value.h>
#include <library/cpp/langs/langs.h>

namespace NAlice {
struct IRng;
} // namespace NAlice

namespace NAlice::NNlg {

struct TRenderContextData {
    NJson::TJsonValue Context = NJson::TJsonValue(NJson::EJsonValueType::JSON_MAP);
    NJson::TJsonValue Form = NJson::TJsonValue(NJson::EJsonValueType::JSON_MAP);
    NJson::TJsonValue ReqInfo = NJson::TJsonValue(NJson::EJsonValueType::JSON_MAP);
};

struct TRenderPhraseResult {
    TString Text;
    TString Voice;
};

struct TRenderCardResult {
    NJson::TJsonValue Card;
};

struct INlgRenderer {
    virtual ~INlgRenderer() = default;

    virtual bool HasPhrase(TStringBuf templateId, TStringBuf phraseId, ELanguage lang) const = 0;
    virtual TRenderPhraseResult RenderPhrase(TStringBuf templateId, TStringBuf phraseId, ELanguage lang,
                                             IRng& rng, TRenderContextData renderContextData) const = 0;

    virtual bool HasCard(TStringBuf templateId, TStringBuf cardId, ELanguage lang) const = 0;
    virtual TRenderCardResult RenderCard(TStringBuf templateId, TStringBuf cardId, ELanguage lang,
                                         IRng& rng, TRenderContextData renderContextData, bool reduceWhitespace = false) const = 0;
};

}
