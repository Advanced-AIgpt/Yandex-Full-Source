#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/entity.h>
#include <alice/hollywood/library/scenarios/general_conversation/containers/general_conversation_resources.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/base_scenario/fwd.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/library/logger/logger.h>
#include <alice/library/util/rng.h>

namespace NAlice::NHollywood::NGeneralConversation {

void AddEntityCandidatesRequest(const TScenarioRunRequestWrapper& requestWrapper, TScenarioHandleContext* ctx);
TMaybe<TEntitySearchCache> RetireEntityCandidatesResponse(const TScenarioRunRequestWrapper& requestWrapper, const TScenarioHandleContext& ctx);

void FillEntity(TGeneralConversationRunContextWrapper& contextWrapper, const TSessionState& sessionState, TReplyInfo* replyInfo);
void FillEntityType(TGeneralConversationRunContextWrapper& contextWrapper, bool contentForChild, TReplyInfo* replyInfo);
bool TrySetEntity(const TString& entityKey, const TScenarioRunRequestWrapper& requestWrapper, const TGeneralConversationResources& resources, IRng& rng, TReplyInfo* reply);

template <typename TRequestWrapper>
bool IsMovieOpenSupportedDevice(const TRequestWrapper& requestWrapper);

template <typename TContextWrapper>
TMaybe<TString> GetQuestionAboutEntity(TContextWrapper& contextWrapper, const TSessionState& sessionState, const TReplyInfo& replyInfo);

template <typename TContextWrapper>
TString RenderMovieOpenUtterance(const TEntity& entity, TContextWrapper& contextWrapper);

void UpdateLastDiscussion(const TClassificationResult& classificationResult, TSessionState* sessionState);

} // namespace NAlice::NHollywood::NGeneralConversation
