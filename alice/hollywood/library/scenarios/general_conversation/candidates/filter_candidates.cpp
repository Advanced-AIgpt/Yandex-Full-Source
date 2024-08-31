#include "filter_candidates.h"

#include <alice/hollywood/library/scenarios/general_conversation/common/aggregated_reply_wrapper.h>

#include <library/cpp/string_utils/quote/quote.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/string/builder.h>

namespace NAlice::NHollywood::NGeneralConversation {

void FilterCandidates(const NJson::TJsonValue& response, TVector<TAggregatedReplyCandidate>* candidates) {
    const auto& passedNode = response["rules"]["AliceFixlistFilter"]["Passed"];
    THashSet<TStringBuf> passedSet;
    if (passedNode.IsArray()) {
        const auto& passedArray = passedNode.GetArray();
        passedSet.reserve(passedArray.size());
        for (const auto& passed : passedArray) {
            passedSet.insert(passed.GetString());
        }
    } else if (passedNode.IsString()) {
        passedSet.insert(passedNode.GetString());
    }

    EraseIf(*candidates, [&passedSet] (const auto& candidate) { return !passedSet.contains(GetAggregatedReplyText(candidate)); });
}

void FilterRepeatsByReplyHash(const TSessionState& sessionState, TVector<TAggregatedReplyCandidate>* candidates) {
    THashSet<size_t> usedReplies(sessionState.GetUsedRepliesInfo().size());
    for (const auto& replyInfo : sessionState.GetUsedRepliesInfo()) {
        usedReplies.insert(replyInfo.GetHash());
    }
    EraseIf(*candidates, [&usedReplies] (const auto& candidate) { return usedReplies.contains((THash<TString>{}(GetAggregatedReplyText(candidate)))); });
}

} // namespace NAlice::NHollywood::NGeneralConversation
