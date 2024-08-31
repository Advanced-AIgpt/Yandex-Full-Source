#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/base_scenario/fwd.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/analytics/scenarios/general_conversation/general_conversation.pb.h>

namespace NAlice::NHollywood::NGeneralConversation {

TString GetProactivityIntentFromSearchAction(const TScenarioRunRequestWrapper& requestWrapper, const TString& action, bool modalModeEnabled, IRng& rng);
void AddProactivityActions(const TReplyInfo& replyInfo, TResponseBodyBuilder* responseBodyBuilder);
void AddProactivitySuggests(const TScenarioRunRequestWrapper& requestWrapper, const TReplyInfo& replyInfo,
        TNlgWrapper& nlgWrapper, TRTLogger& logger, TResponseBodyBuilder* responseBodyBuilder);
void AddProactivityAnalytics(const TReplyInfo& replyInfo, const TString& frameName, NScenarios::NGeneralConversation::TGCResponseInfo* gcResponseInfo);
void PatchReplyWithEntity(TGeneralConversationRunContextWrapper& contextWrapper, const TString& frameName, TReplyInfo* reply);
bool TryDetectFrameProactivity(const TScenarioRunRequestWrapper& requestWrapper, bool modalModeEnabled, IRng& rng, TClassificationResult* classificationResult);

} // namespace NAlice::NHollywood::NGeneralConversation
