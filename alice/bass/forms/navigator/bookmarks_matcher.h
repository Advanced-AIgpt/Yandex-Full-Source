#pragma once

#include <alice/library/parsed_user_phrase/parsed_sequence.h>
#include <alice/library/parsed_user_phrase/stopwords.h>

#include <util/string/builder.h>

namespace NBASS {

struct TGeoStopWords {
    TGeoStopWords();

    NParsedUserPhrase::TStopWordsHolder Holder;
};

class TBookmarksMatcher {
public:
    static constexpr float DEFAULT_THRESHOLD = 0.8;
    static constexpr float INVALID_SCORE = -1;

public:
    struct TScore {
        bool operator<(const TScore& other) {
            if (ForwardScore == other.ForwardScore)
                return BackwardScore < other.BackwardScore;
            return ForwardScore < other.ForwardScore;
        }

        TString ToString() {
            return TStringBuilder() << ForwardScore << ", " << BackwardScore;
        }

        float ForwardScore = TBookmarksMatcher::INVALID_SCORE;
        float BackwardScore = TBookmarksMatcher::INVALID_SCORE;
    };

public:
    explicit TBookmarksMatcher(TStringBuf query);

    float Match(TStringBuf bookmark, float threshold) const;

private:
    NParsedUserPhrase::TParsedSequence Query;
    TGeoStopWords StopWords;
};

} // namespace NBASS
