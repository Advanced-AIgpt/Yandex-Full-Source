#include "parsed_sequence.h"

#include "stopwords.h"

#include <quality/trailer/trailer_common/normalize.h>

#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/system/yassert.h>

#include <cctype>

namespace NParsedUserPhrase {

namespace {
bool IsDigitOnly(TWtringBuf buf) {
    for (const auto& c : buf) {
        if (!isdigit(c))
            return false;
    }
    return true;
}
} // namespace anonymous

// static
const float TParsedSequence::INVALID_SCORE = -1;

TParsedSequence::TParsedSequence(TStringBuf sequence)
    : TParsedSequence(UTF8ToWide(sequence)) {
}

TParsedSequence::TParsedSequence(TUtf16String sequence) {
    ForEachToken(sequence, &Original, [&](TWtringBuf w) {
        Words.emplace_back();

        auto& synonyms = Words.back();

        auto emplace = [&synonyms, this](TWtringBuf w, EType type) {
            synonyms.emplace_back(w, type);
            Data[w] |= static_cast<int>(type);
        };

        if (IsDigitOnly(w)) {
            emplace(w, WORD);
            return;
        }

        const auto lemma = TLemma::FromWord(w);

        if (!lemma) {
            emplace(w, WORD);
            emplace(w, LEMMA);
        } else if (lemma->Micro) {
            emplace(w, MICRO);
        } else {
            emplace(w, WORD);
            emplace(lemma->Text, LEMMA);
        }
    });
}

float TParsedSequence::MatchPrefix(const TParsedSequence& etalon, size_t prefixLength) const {
    return MatchPrefix(etalon, prefixLength, NO_STOP_WORDS);
}

float TParsedSequence::Match(const TParsedSequence& etalon) const {
    return MatchPrefix(etalon, Words.size());
}

float TParsedSequence::Match(const TParsedSequence& etalon, const TStopWordsHolder& stopWords) const {
    return MatchPrefix(etalon, Words.size(), stopWords);
}

float TParsedSequence::MatchPrefix(const TParsedSequence& etalon, size_t prefixLength, const TStopWordsHolder& stopWords) const {
    if (prefixLength == 0) {
        return 1.0;
    }

    const float EPS = 1e-6;

    float total = 0;
    float matched = 0;

    if (Original == etalon.Original)
        return 1.0;

    for (size_t i = 0; i < std::min(Words.size(), prefixLength); ++i) {
        const auto& synonyms = Words[i];
        Y_ASSERT(!synonyms.empty());

        float maxWeight = -1;
        float matchWeight = -1;

        for (const auto& st : synonyms) {
            const auto& word = st.first;
            const auto type = st.second;
            const auto etalonTypes = etalon.Types(word);

            float weight = TypeWeight(type);
            if (stopWords.Has(word))
                weight *= stopWords.GetWeight(word);

            maxWeight = std::max(maxWeight, weight);

            if ((etalonTypes & type) && weight > matchWeight)
                matchWeight = weight;

            if (type == LEMMA && (etalonTypes & WORD) && weight > matchWeight)
                matchWeight = weight;
        }

        if (matchWeight >= 0) {
            total += matchWeight;
            matched += matchWeight;
        } else {
            total += maxWeight;
        }
    }

    if (matched < EPS)
        return INVALID_SCORE;

    return matched / total;
}

size_t TParsedSequence::Size() const {
    return Words.size();
}

int TParsedSequence::Types(const TUtf16String& w) const {
    const auto it = Data.find(w);
    return it == Data.end() ? 0 : it->second;
}

bool MatchSequences(const TParsedSequence& userPhrase, const TParsedSequence& predefinedPhrase,
                    float userMatchThreshold, float predefinedMatchThreshold) {
    static const TStopWordsHolder NO_STOP_WORDS;
    return MatchSequences(userPhrase, predefinedPhrase, NO_STOP_WORDS, userMatchThreshold, predefinedMatchThreshold);
}

bool MatchSequences(const TParsedSequence& userPhrase, const TParsedSequence& predefinedPhrase,
                    const TStopWordsHolder& stopWords, float userMatchThreshold, float predefinedMatchThreshold) {
    const bool userMatch = userPhrase.Match(predefinedPhrase, stopWords) >= userMatchThreshold;
    const bool predefinedMatch = predefinedPhrase.Match(userPhrase, stopWords) >= predefinedMatchThreshold;

    return userMatch && predefinedMatch;
}

float ComputeIntersectionScore(const TParsedSequence& request, const TParsedSequence& phrase,
                               const TStopWordsHolder& stopWords) {
    const double score1 = request.Match(phrase, stopWords);
    const double score2 = phrase.Match(request, stopWords);
    return (score1 + score2) / 2;
}

} // namespace NParsedUserPhrase
