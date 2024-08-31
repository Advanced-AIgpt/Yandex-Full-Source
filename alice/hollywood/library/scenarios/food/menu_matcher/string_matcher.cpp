#include "string_matcher.h"
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <library/cpp/json/json_reader.h>
#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/subst.h>

template <typename TContainer>
inline TUtf16String JoinTokens(const TContainer& tokens) {
    TUtf16String result;
    for (const TWtringBuf& token : tokens) {
        if (!result.empty()) {
            result.append(u' ');
        }
        result.append(token);
    }
    return result;
}

namespace NAlice::NHollywood::NFood {

TStringMatcher::TStringMatcher() {
}

void TStringMatcher::ReadOptionsJson(const NJson::TJsonValue& options) {
    Delimiters = UTF8ToWide(options["delimiters"].GetStringSafe(" "));

    for (const NJson::TJsonValue& synonymListJson : options["synonyms"].GetArray()) {
        const TString synonymList = synonymListJson.GetString();
        TStringBuf dstSynonym;
        TStringBuf srcSynonyms;
        TStringBuf(synonymList).Split(':', dstSynonym, srcSynonyms);
        const TUtf16String dstSynonymWide = PreNormalizeForMatch(dstSynonym) + u' ';

        for (const TStringBuf srcSynonym : StringSplitter(srcSynonyms).SplitBySet(",;").SkipEmpty()) {
            const TUtf16String srcSynonymWide = PreNormalizeForMatch(srcSynonym) + u' ';
            SynonymsTrie.Add(srcSynonymWide, dstSynonymWide);
            SynonymsMap[srcSynonymWide] = dstSynonymWide;
        }
    }
}

void TStringMatcher::AddToNormalizationCache(const TString& text) {
    const auto [it, isNew] = NormalizationCache.try_emplace(text);
    if (isNew) {
        it->second = NormalizeForMatchImpl(text);
    }
}

bool TStringMatcher::MatchNames(const TUtf16String& normalizedFirst, TStringBuf second) const {
    if (normalizedFirst.empty()) {
        return false;
    }
    const TUtf16String* fromCache = NormalizationCache.FindPtr(second);
    if (fromCache != nullptr) {
        return normalizedFirst == *fromCache;
    }
    return normalizedFirst == NormalizeForMatchImpl(second);
}

TUtf16String TStringMatcher::NormalizeForMatch(TStringBuf text) const {
    const TUtf16String* fromCache = NormalizationCache.FindPtr(text);
    if (fromCache != nullptr) {
        return *fromCache;
    }
    return NormalizeForMatchImpl(text);
}

TUtf16String TStringMatcher::NormalizeForMatchImpl(TStringBuf text) const {
    return SortTokens(SubstituteSynonyms(PreNormalizeForMatch(text)));
}

TUtf16String TStringMatcher::PreNormalizeForMatch(TStringBuf text) const {
    TUtf16String normalized = UTF8ToWide<true>(text);
    ToLower(normalized);
    SubstGlobal(normalized, u'ё', u'е');
    normalized = JoinTokens(StringSplitter(normalized).SplitBySet(Delimiters.Data()).SkipEmpty());
    return normalized;
}

TUtf16String TStringMatcher::SubstituteSynonyms(TUtf16String text) const {
    if (text.empty()) {
        return text;
    }
    text += u' ';
    TWtringBuf textBuf = text;
    TUtf16String result;
    while (!textBuf.empty()) {
        size_t prefixLen = 0;
        TUtf16String synonym;
        if (SynonymsTrie.FindLongestPrefix(textBuf, &prefixLen, &synonym)) {
            result += synonym;
            textBuf.Skip(prefixLen);
            continue;
        }
        result += textBuf.NextTok(u' ');
        result += u' ';
    }
    Y_ENSURE(!result.empty());
    return result.substr(0, result.size() - 1);
}

// static
TUtf16String TStringMatcher::SortTokens(TWtringBuf text) {
    TVector<TWtringBuf> tokens = StringSplitter(text).Split(u' ').SkipEmpty().ToList<TWtringBuf>();
    Sort(tokens);
    return JoinTokens(tokens);
}

void TStringMatcher::Dump(IOutputStream* log, bool verbose, const TString& indent) const {
    Y_ENSURE(log);
    *log << indent << "TStringMatcher:" << Endl;
    *log << indent << "  Delimiters: " << Delimiters << Endl;
    *log << indent << "  SynonymsMap:" << Endl;
    for (const TUtf16String& src : NGranet::OrderedSetOfKeys(SynonymsMap)) {
        *log << indent << "    " << src << " -> " << SynonymsMap.at(src) << Endl;
    }
    if (!verbose) {
        return;
    }
    *log << indent << "  NormalizationCache:" << Endl;
    for (const TString& src : NGranet::OrderedSetOfKeys(NormalizationCache)) {
        *log << indent << "    " << src << " -> " << WideToUTF8(NormalizationCache.at(src)) << Endl;
    }
}

} // namespace NAlice::NHollywood::NFood
