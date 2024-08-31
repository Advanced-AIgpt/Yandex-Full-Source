#include "helpers.h"

namespace NAlice::NNlg {

namespace NPrivate {

void FormatLocalizedTemplate(
    const TStringBuf localizedTemplate,
    const THashMap<TStringBuf, TRenderLocalizedNlgPlaceholder>& placeholders,
    IOutputStream& out)
{
    auto currentPartOfTemplate = localizedTemplate;
    while (true) {
        TStringBuf prefix, placeholderKeyAndSuffix;
        if (!currentPartOfTemplate.TrySplit('{', prefix, placeholderKeyAndSuffix)) {
            break;
        }
        out << prefix;

        TStringBuf placeholderKey;
        Y_ENSURE_EX(placeholderKeyAndSuffix.TrySplit('}', placeholderKey, currentPartOfTemplate), TTranslationError() <<
            "Localized template has unmatched placeholder starting symbol '{': " << localizedTemplate);

        const auto* placeholderCallback = placeholders.FindPtr(placeholderKey);
        Y_ENSURE_EX(placeholderCallback, TTranslationError() <<
            "Localized template contains unknown placeholder '" << placeholderKey << "': " << localizedTemplate);
        (*placeholderCallback)();
    }
    out << currentPartOfTemplate;
}

} // namespace NPrivate

void LocalizeTemplateWithPlaceholders(
    const TCallCtx& callCtx,
    const TStringBuf localizedTemplateKey,
    const THashMap<TStringBuf, TRenderLocalizedNlgPlaceholder>& placeholders,
    IOutputStream& out)
{
    const auto language = LanguageByName(callCtx.Language);
    Y_ENSURE(language != ELanguage::LANG_UNK, "Failed to extract nlg language: " << callCtx.Language);

    const auto& localizedTemplate = callCtx.Env.GetTranslation(language, localizedTemplateKey);

    NPrivate::FormatLocalizedTemplate(localizedTemplate, placeholders, out);
}

} // namespace NAlice::NNlg
