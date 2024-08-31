#pragma once

#include <alice/hollywood/library/nlg/nlg_data.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/library/util/rng.h>
#include <alice/nlg/library/nlg_renderer/nlg_renderer.h>
#include <alice/nlg/library/runtime_api/translations.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/langs/langs.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/string.h>

#include <functional>

namespace NAlice::NHollywood {

namespace NImpl {

struct TMapContainer {
    const NJson::TJsonValue Value{NJson::JSON_MAP};
};

} // namespace NImpl

class TCompiledNlgComponent {
public:
    using TRegisterFunction = NNlg::TRegisterFunction;

public:
    TCompiledNlgComponent(IRng& rng, NNlg::ITranslationsContainerPtr nlgTranslations, const TRegisterFunction& regFunc);

    NNlg::TRenderPhraseResult RenderPhrase(
        const TStringBuf templateName, const TStringBuf phraseName,
        const ELanguage lang, IRng& rng,
        const TNlgData& nlgData) const;

    NNlg::TRenderCardResult RenderCard(
        const TStringBuf templateName, const TStringBuf cardName,
        const ELanguage lang, IRng& rng,
        const TNlgData& nlgData, const bool reduceWhitespace = false) const;

    bool HasPhrase(const TStringBuf templateId, const TStringBuf phraseId, const ELanguage lang) const;
    bool HasCard(const TStringBuf templateId, const TStringBuf cardId, const ELanguage lang) const;

private:
    NNlg::INlgRendererPtr NlgRenderer_;
};

} // namespace NAlice::NHollywood
