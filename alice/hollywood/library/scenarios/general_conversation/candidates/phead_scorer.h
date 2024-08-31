#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>

#include <alice/boltalka/generative/service/proto/phead.pb.h>

namespace NAlice::NHollywood::NGeneralConversation {

void AddPheadRequest(const TString& url, TGeneralConversationRunContextWrapper& contextWrapper, const TString& pheadPath);

struct TPHeadResponse {
	TVector<float> Scores;
};

TMaybe<TPHeadResponse> RetirePheadResponse(const TScenarioHandleContext& ctx);

} // namespace NAlice::NHollywood::NGeneralConversation
