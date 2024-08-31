#pragma once

#include "bow.h"

#include <library/cpp/langs/langs.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/singleton.h>
#include <util/generic/vector.h>


namespace NAlice::NIot {

using TVariations = THashMap<TString, TVector<TString>>;

struct ICategorizedVariations {
    virtual const TVariations& GetVariations(const TStringBuf category = CATEGORY_UNKNOWN) const = 0;
    virtual ~ICategorizedVariations() = default;

    static constexpr TStringBuf CATEGORY_UNKNOWN = "unknown";
};


struct TPreprocessor {
    THashMap<TString, THashSet<TString>> TypeToBowIndexTokens;
    TBowIndex BOWIndex;
    NSc::TValue Entities;
    THolder<ICategorizedVariations> SpellingVariations;
    THolder<ICategorizedVariations> Synonyms;
    THashSet<TString> SubSynonyms;
    THashSet<TString> Prepositions;
    THashSet<TString> UnitWords;
    TVector<TString> TypesSuitableForSubSynonyms;
    size_t SubSynonymsMaxNumTokens;
};

const TPreprocessor& GetPreprocessor(ELanguage language);

} // namespace NAlice::NIot
