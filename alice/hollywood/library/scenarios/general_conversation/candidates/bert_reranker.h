#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/base_scenario/fwd.h>
#include <alice/hollywood/library/request/request.h>

namespace NAlice::NHollywood::NGeneralConversation {

void AddBertRerankerRequest(const TAggregatedRepliesState& repliesState, TGeneralConversationRunContextWrapper& contextWrapper, size_t contextLength);
bool RetireBertRerankerResponse(const TScenarioHandleContext& ctx, TVector<TAggregatedReplyCandidate>* candidates);

} // namespace NAlice::NHollywood::NGeneralConversation
