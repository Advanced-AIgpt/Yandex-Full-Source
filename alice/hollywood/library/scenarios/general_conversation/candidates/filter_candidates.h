#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>
#include <alice/hollywood/library/base_scenario/fwd.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <alice/library/json/json.h>

namespace NAlice::NHollywood::NGeneralConversation {

void FilterCandidates(const NJson::TJsonValue& response, TVector<TAggregatedReplyCandidate>* candidates);
void FilterRepeatsByReplyHash(const TSessionState& sessionState, TVector<TAggregatedReplyCandidate>* candidates);

} // namespace NAlice::NHollywood::NGeneralConversation
