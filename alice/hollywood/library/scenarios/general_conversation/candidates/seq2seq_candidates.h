#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/base_scenario/fwd.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/boltalka/generative/service/proto/generative_request.pb.h>

namespace NAlice::NHollywood::NGeneralConversation {

template <typename TContextWrapper>
void AddReplySeq2SeqCandidatesRequest(const TString& url, TContextWrapper& contextWrapper, NGenerativeBoltalka::Proto::TGenerativeRequest& bodyProto, int numHypos=1);

void AddReplySeq2SeqCandidatesRequest(const TString& url, TGeneralConversationRunContextWrapper& contextWrapper, size_t contextLength, bool addSearchInfo=false);

template <typename TContextWrapper>
void AddReplySeq2SeqCandidatesRequest(const TString& url, TContextWrapper& contextWrapper, const TString& request, int numHypos=1);

TMaybe<TVector<TSeq2SeqReplyCandidate>> RetireReplySeq2SeqCandidatesResponse(const TScenarioHandleContext& ctx);

} // namespace NAlice::NHollywood::NGeneralConversation
