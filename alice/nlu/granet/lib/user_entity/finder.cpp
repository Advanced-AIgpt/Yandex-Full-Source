#include "finder.h"
#include "finder_impl.h"
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

namespace NGranet::NUserEntity {

TVector<TEntity> FindEntitiesInTexts(const TEntityDicts& dicts, const TVector<TString>& requestVariants,
    TStringBuf destAlignTokens, const TLangMask& languages, IOutputStream* log)
{
    TVector<TEntity> entities;
    TEntityFinder(requestVariants, destAlignTokens, languages, log).Find(dicts, &entities);
    return entities;
}

TVector<TEntity> FindEntitiesInSample(const TEntityDicts& dicts, const TSample::TConstRef& sample,
    ELanguage lang, IOutputStream* log)
{
    Y_ENSURE(sample);
    if (dicts.Dicts.empty()) {
        return {};
    }
    const TString fstText = NNlu::TRequestNormalizer::Normalize(lang, sample->GetText());
    return FindEntitiesInSample(dicts, sample, fstText, {lang, LANG_ENG}, log);
}

TVector<TEntity> FindEntitiesInSample(const TEntityDicts& dicts, const TSample::TConstRef& sample,
    const TString& fstText, const TLangMask& languages, IOutputStream* log)
{
    Y_ENSURE(sample);
    const TString& mainText = sample->GetJoinedTokens();
    TVector<TEntity> entities;
    TEntityFinder({mainText, fstText}, mainText, languages, log).Find(dicts, &entities);
    return entities;
}

} // namespace NGranet::NUserEntity
