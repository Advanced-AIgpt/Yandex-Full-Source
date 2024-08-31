#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/base_scenario/fwd.h>
#include <alice/hollywood/library/request/request.h>

namespace NAlice::NHollywood::NGeneralConversation {

void AddReplyCandidatesRequest(TGeneralConversationRunContextWrapper& contextWrapper, const TSessionState& sessionState,
        const TClassificationResult& classificationResult, size_t contextLength);
TVector<TNlgSearchReplyCandidate> RetireReplyCandidatesResponse(const TScenarioHandleContext& ctx, bool preferChildReply);

void AddSuggestCandidatesRequest(const TScenarioRunRequestWrapper& requestWrapper, const TString& reply, size_t contextLength, TScenarioHandleContext* ctx);
TMaybe<TVector<TNlgSearchReplyCandidate>> RetireSuggestCandidatesResponse(const TScenarioHandleContext& ctx, bool preferChildSuggests);

void UpdateUsedRepliesState(const TScenarioRunRequestWrapper& requestWrapper, const TString& text, TSessionState* sessionState);

} // namespace NAlice::NHollywood::NGeneralConversation
