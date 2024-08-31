#pragma once

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {

struct TPhraseMatch {
    size_t ItemIndex = -1;
    double Score = 0;
    TString MatchedPhrase;
};

struct TMatchingItem {
    TVector<TString> Synonyms;
};

template <class TTransformer, class TScorer>
TMaybe<TPhraseMatch> GenericChooseBestMatch(const TString& request, const TVector<TMatchingItem>& items,
                                            const double threshold, const TTransformer& transformer,
                                            const TScorer& scorer) {
    const auto transformedRequest = transformer(request);
    TMaybe<TPhraseMatch> bestMatch = Nothing();
    for (size_t index = 0; index < items.size(); ++index) {
        const auto& item = items[index];
        for (const auto& phrase : item.Synonyms) {
            const auto transformedPhrase = transformer(phrase);
            const double score = scorer(transformedRequest, transformedPhrase);
            if (score < threshold) {
                continue;
            }
            if (!bestMatch.Defined() || score > bestMatch->Score) {
                bestMatch = TPhraseMatch{index, score, phrase};
            }
        }
    }
    return bestMatch;
}

TMaybe<TPhraseMatch> ChooseBestMatch(const TString& request, const TVector<TMatchingItem>& items);

} // namespace NAlice
