#pragma once

#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/json/writer/json_value.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywood::NFood {

class TStringMatcher {
public:
    TStringMatcher();

    void ReadOptionsJson(const NJson::TJsonValue& options);

    void AddToNormalizationCache(const TString& text);

    bool MatchNames(const TUtf16String& normalizedFirst, TStringBuf second) const;
    TUtf16String NormalizeForMatch(TStringBuf text) const;

    void Dump(IOutputStream* log, bool verbose = true, const TString& indent = "") const;

private:
    TUtf16String NormalizeForMatchImpl(TStringBuf text) const;
    TUtf16String PreNormalizeForMatch(TStringBuf text) const;
    TUtf16String SubstituteSynonyms(TUtf16String text) const;
    static TUtf16String SortTokens(TWtringBuf text);

private:
    TUtf16String Delimiters = u" ";
    TCompactTrie<wchar16, TUtf16String>::TBuilder SynonymsTrie;
    THashMap<TUtf16String, TUtf16String> SynonymsMap; // for debug
    THashMap<TString, TUtf16String> NormalizationCache;
};

}  // namespace NAlice::NHollywood::NFood
