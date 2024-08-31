#include "phrase_matching.h"

#include <alice/library/parsed_user_phrase/parsed_sequence.h>

namespace NAlice {
namespace {

constexpr double MATCHING_SCORE_THRESHOLD = 0.47;

double ComputeMatchingScore(const NParsedUserPhrase::TParsedSequence& request, const NParsedUserPhrase::TParsedSequence& phrase) {
    double score1 = request.Match(phrase);
    double score2 = phrase.Match(request);
    return (score1 + score2) / 2;
}

} // namespace anonymous

TMaybe<TPhraseMatch> ChooseBestMatch(const TString& request, const TVector<TMatchingItem>& items) {
    return GenericChooseBestMatch(request, items, MATCHING_SCORE_THRESHOLD,
                                  [](const TString& text){return NParsedUserPhrase::TParsedSequence(text);},
                                  ComputeMatchingScore);
}

} // namespace NAlice
