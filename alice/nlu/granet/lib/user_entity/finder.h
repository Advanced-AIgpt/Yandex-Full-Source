#pragma once

#include "dictionary.h"
#include <alice/nlu/granet/lib/lang/simple_tokenizer.h>
#include <alice/nlu/granet/lib/sample/entity.h>
#include <alice/nlu/granet/lib/sample/sample.h>
#include <library/cpp/langmask/langmask.h>
#include <util/generic/array_ref.h>
#include <util/generic/set.h>
#include <util/generic/string.h>

namespace NGranet::NUserEntity {

// Find user entities in request.
// Params:
//   requestVariants - different normalizations of single request.
//   destAlignTokens - found entities are aligned to that tokens (joined by space).
[[nodiscard]] TVector<TEntity> FindEntitiesInTexts(const TEntityDicts& dicts, const TVector<TString>& requestVariants,
    TStringBuf destAlignTokens, const TLangMask& languages = GRANET_LANGUAGES, IOutputStream* log = nullptr);

// Find user entities in sample.
// Entities are searched in original and normalized (by NNlu::TRequestNormalizer) texts of sample.
[[nodiscard]] TVector<TEntity> FindEntitiesInSample(const TEntityDicts& dicts, const TSample::TConstRef& sample,
    ELanguage lang, IOutputStream* log = nullptr);

// Find user entities in sample.
// Normalized text provided via 'fstText' parameter.
[[nodiscard]] TVector<TEntity> FindEntitiesInSample(const TEntityDicts& dicts, const TSample::TConstRef& sample,
    const TString& fstText, const TLangMask& languages = GRANET_LANGUAGES, IOutputStream* log = nullptr);

} // namespace NGranet::NUserEntity
