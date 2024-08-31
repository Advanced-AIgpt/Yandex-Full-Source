#include "nlg.h"
#include <alice/nlg/library/nlg_renderer/create_nlg_renderer_from_register_function.h>

#include <alice/library/logger/logger.h>

#include <util/string/builder.h>

using namespace NAlice::NNlg;

namespace NAlice::NHollywood {

namespace {

void LogContext(const TNlgData& nlgData) {
    LOG_DEBUG(nlgData.Logger) << "Context: " << nlgData.Context << ", "
                                << "Form: " << nlgData.Form << ", "
                                << "ReqInfo: " << nlgData.ReqInfo;
}

NNlg::TRenderContextData ConvertToRenderContextData(const TNlgData& nlgData) {
    return NNlg::TRenderContextData {
        .Context = nlgData.Context,
        .Form = nlgData.Form,
        .ReqInfo = nlgData.ReqInfo,
    };
}

} // namespace

TCompiledNlgComponent::TCompiledNlgComponent(IRng& rng, NNlg::ITranslationsContainerPtr nlgTranslations, const TCompiledNlgComponent::TRegisterFunction& regFunc)
    : NlgRenderer_(CreateLocalizedNlgRendererFromRegisterFunction(regFunc, std::move(nlgTranslations), rng))
{
}

NNlg::TRenderPhraseResult TCompiledNlgComponent::RenderPhrase(const TStringBuf templateName, const TStringBuf phraseName,
                                                  const ELanguage lang, IRng& rng, const TNlgData& nlgData) const {
    if (nlgData.ShouldLogNlg) {
        LOG_DEBUG(nlgData.Logger) << "Rendering template = " << templateName << ", phrase = " << phraseName << ", langId: " << static_cast<int>(lang);
        LogContext(nlgData);
    }

    const auto result = NlgRenderer_->RenderPhrase(
        templateName, phraseName, lang, rng, ConvertToRenderContextData(nlgData));

    if (nlgData.ShouldLogNlg) {
        LOG_DEBUG(nlgData.Logger) << "NLG Text: " << result.Text << ", "
                                  << "NLG Voice: " << result.Voice;
    }

    return result;
}

TRenderCardResult TCompiledNlgComponent::RenderCard(const TStringBuf templateName, const TStringBuf cardName,
                                                    const ELanguage lang, IRng& rng, const TNlgData& nlgData,
                                                    const bool reduceWhitespace) const
{
    if (nlgData.ShouldLogNlg) {
        LOG_DEBUG(nlgData.Logger) << "Rendering template = " << templateName << ", card = " << cardName;
        LogContext(nlgData);
    }

    const auto result = NlgRenderer_->RenderCard(
        templateName, cardName, lang, rng, ConvertToRenderContextData(nlgData), reduceWhitespace);

    if (nlgData.ShouldLogNlg) {
        LOG_DEBUG(nlgData.Logger) << "Card: " << result.Card;
    }

    return result;
}

bool TCompiledNlgComponent::HasPhrase(const TStringBuf templateId, const TStringBuf phraseId, const ELanguage lang) const {
    return NlgRenderer_->HasPhrase(templateId, phraseId, lang);
}

bool TCompiledNlgComponent::HasCard(const TStringBuf templateId, const TStringBuf cardId, const ELanguage lang) const {
    return NlgRenderer_->HasCard(templateId, cardId, lang);
}

} // namespace NAlice::NHollywood
