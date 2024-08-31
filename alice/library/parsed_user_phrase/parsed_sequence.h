#pragma once

#include "stopwords.h"
#include "utils.h"

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <utility>

namespace NParsedUserPhrase {

class TStopWordsHolder;

class TParsedSequence {
public:
    static const float INVALID_SCORE;

public:
    explicit TParsedSequence(TStringBuf sequence);
    explicit TParsedSequence(TUtf16String sequence);

    float Match(const TParsedSequence& etalon) const;
    float Match(const TParsedSequence& etalon, const TStopWordsHolder& stopWords) const;
    float MatchPrefix(const TParsedSequence& etalon, size_t prefixLength) const;
    float MatchPrefix(const TParsedSequence& etalon, size_t prefixLength, const TStopWordsHolder& stopWords) const;

    size_t Size() const;

private:
    int Types(const TUtf16String& w) const;

private:
    TUtf16String Original;

    THashMap<TUtf16String, int> Data;
    TVector<TVector<std::pair<TUtf16String, EType>>> Words;
};

bool MatchSequences(const TParsedSequence& userPhrase, const TParsedSequence& predefinedPhrase,
                    float userMatchThreshold = 0.5, float predefinedMatchThreshold = 0.11);

bool MatchSequences(const TParsedSequence& userPhrase, const TParsedSequence& predefinedPhrase,
                    const TStopWordsHolder& stopWords, float userMatchThreshold = 0.5,
                    float predefinedMatchThreshold = 0.11);

float ComputeIntersectionScore(const TParsedSequence& request, const TParsedSequence& phrase,
                               const TStopWordsHolder& stopWords = NO_STOP_WORDS);

} // namespace NParsedUserPhrase
