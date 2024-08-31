#pragma once

#include "alignment.h"
#include <library/cpp/string_utils/levenshtein_diff/levenshtein_diff.h>
#include <util/generic/deque.h>
#include <util/generic/hash.h>
#include <util/digest/sequence.h>

namespace NNlu {

// ~~~~ TNormalizedTokenAligner ~~~~

class TNormalizedTokenAligner : public TMoveOnly {
public:
    TAlignmentData Align(TWtringBuf joinedTokens1, TWtringBuf joinedTokens2);

private:
    // pair<hash, string>
    // Used to speed up the comparizon.
    using TToken = std::pair<size_t, TWtringBuf>;

    static void InitWorkingTokens(TWtringBuf joinedTokens, TDeque<TToken>* workingTokens);
    size_t MatchCommonPrefix();
    size_t MatchCommonSuffix();
    void MatchChunk();
    void ProcessEditChain(const NLevenshtein::TEditChain& chain);

private:
    TDeque<TToken> Tokens1;
    TDeque<TToken> Tokens2;
    TAlignmentData Alignment;
};

// ~~~~ TTokenAligner ~~~~

class TTokenAligner {
public:
    static TAlignment Align(const TVector<TString>& tokens1, const TVector<TString>& tokens2);
    static TAlignment Align(const TVector<TStringBuf>& tokens1, const TVector<TStringBuf>& tokens2);
    static TAlignment Align(TStringBuf joinedTokens1, TStringBuf joinedTokens2);
    static TAlignment Align(TWtringBuf joinedTokens1, TWtringBuf joinedTokens2);

private:
    static TUtf16String NormalizeTokens(TWtringBuf tokensRaw, TVector<size_t>* partitions);
    static TUtf16String NormalizeToken(TWtringBuf tokenRaw);
    static void NormalizeByTable(TUtf16String* token);
    static void SplitNumber(TUtf16String* token);
};

// ~~~~ TTokenCachedAligner ~~~~

class TTokenCachedAligner : public TMoveOnly {
public:
    explicit TTokenCachedAligner(size_t cacheLimit = Max<size_t>());

    const TAlignment& Align(const TVector<TString>& tokens1, const TVector<TString>& tokens2);
    const TAlignment& Align(const TVector<TStringBuf>& tokens1, const TVector<TStringBuf>& tokens2);
    const TAlignment& Align(const TString& joinedTokens1, const TString& joinedTokens2);

private:
    using TAlignerInput = std::pair<TString, TString>;

private:
    size_t CacheLimit = Max<size_t>();
    THashMap<TAlignerInput, TAlignment> Cache;
};

} // namespace NNlu
