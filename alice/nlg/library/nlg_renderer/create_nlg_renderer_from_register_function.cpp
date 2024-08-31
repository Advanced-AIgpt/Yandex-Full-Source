#include "create_nlg_renderer_from_register_function.h"
#include "nlg_renderer.h"

#include <alice/nlg/library/runtime_api/env.h>
#include <alice/nlg/library/voice_prefix/voice_prefix.h>
#include <util/string/join.h>

namespace NAlice::NNlg {

namespace {

TPhraseCardParams ConvertToPhraseCardParams(TRenderContextData&& renderContextData) {
    return TPhraseCardParams {
        .Context = TValue::FromJsonValue(renderContextData.Context),
        .Form = TValue::FromJsonValue(renderContextData.Form),
        .ReqInfo = TValue::FromJsonValue(renderContextData.ReqInfo),
    };
}

class TNlgRenderer final: public INlgRenderer {
public:
    explicit TNlgRenderer(TEnvironment environment);
    bool HasPhrase(TStringBuf templateId, TStringBuf phraseId, ELanguage lang) const override;
    TRenderPhraseResult RenderPhrase(TStringBuf templateId, TStringBuf phraseId, ELanguage lang,
                                     IRng& rng, TRenderContextData renderContextData) const override;

    bool HasCard(TStringBuf templateId, TStringBuf cardId, ELanguage lang) const override;
    TRenderCardResult RenderCard(TStringBuf templateId, TStringBuf cardId, ELanguage lang,
                                 IRng& rng, TRenderContextData renderContextData, bool reduceWhitespace = false) const override;
private:
    TEnvironment Environment_;
};

TNlgRenderer::TNlgRenderer(TEnvironment environment)
    : Environment_(std::move(environment))
{
}

bool TNlgRenderer::HasPhrase(TStringBuf templateId, TStringBuf phraseId, ELanguage lang) const {
    return Environment_.HasPhrase(templateId, phraseId, lang);
}

TRenderPhraseResult TNlgRenderer::RenderPhrase(
    TStringBuf templateId, TStringBuf phraseId, ELanguage lang, IRng& rng, TRenderContextData renderContextData) const
{
    auto result = Environment_.RenderPhrase(
        templateId,
        phraseId,
        lang,
        rng,
        ConvertToPhraseCardParams(std::move(renderContextData))
    );
    if (result.Voice) {
        result.Voice.prepend(GetVoicePrefixForLanguage(lang));
    }
    return TRenderPhraseResult {
        .Text = std::move(result.Text),
        .Voice = std::move(result.Voice),
    };
}

bool TNlgRenderer::HasCard(TStringBuf templateId, TStringBuf cardId, ELanguage lang) const {
    return Environment_.HasCard(templateId, cardId, lang);
}

TRenderCardResult TNlgRenderer::RenderCard(
    TStringBuf templateId, TStringBuf cardId, ELanguage lang, IRng& rng, TRenderContextData renderContextData, bool reduceWhitespace) const
{
    return TRenderCardResult {
        .Card = Environment_.RenderCard(
            templateId,
            cardId,
            lang,
            rng,
            ConvertToPhraseCardParams(std::move(renderContextData)),
            reduceWhitespace
        ),
    };
}

} // namespace

INlgRendererPtr CreateNlgRendererFromRegisterFunction(TRegisterFunction registerFunction, IRng& rng) {
    return CreateLocalizedNlgRendererFromRegisterFunction(registerFunction, nullptr, rng);
}

INlgRendererPtr CreateLocalizedNlgRendererFromRegisterFunction(
    TRegisterFunction registerFunction,
    ITranslationsContainerPtr translationsContainer,
    IRng& rng)
{
    auto environment = TEnvironment();
    registerFunction(environment);
    environment.InitializeAllGlobals(rng);
    environment.SetTranslationsContainer(std::move(translationsContainer));
    return std::make_shared<TNlgRenderer>(std::move(environment));
}

}
