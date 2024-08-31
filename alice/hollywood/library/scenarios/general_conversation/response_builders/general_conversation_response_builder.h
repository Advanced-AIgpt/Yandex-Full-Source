#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/response_builders/reply_source_render_strategy.h>

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/scenarios/suggesters/movie_akinator/response_body_builder.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/analytics/scenarios/general_conversation/general_conversation.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/generic/ptr.h>

namespace NAlice::NHollywood::NGeneralConversation {


template <typename TContextWrapper>
class TGeneralConversationResponseBuilder {
public:
    TGeneralConversationResponseBuilder(TContextWrapper& contextWrapper,
            const TClassificationResult& classificationResult, const TSessionState& sessionState,
            const TMaybe<TReplyState>& replyState, const TMaybe<TVector<TNlgSearchReplyCandidate>>& suggestsState);

    std::unique_ptr<NScenarios::TScenarioRunResponse> BuildResponse(bool shouldContinue) &&;
    std::unique_ptr<NScenarios::TScenarioContinueResponse> BuildContinueResponse() &&;
    TResponseBodyBuilder* GetRunResponseBodyBuilder();
    TResponseBodyBuilder* GetContinueResponseBodyBuilder();

private:
    void AddFeatures();
    void AddResponseAfter();
    void AddSuggestsBefore();
    void AddSuggestsAfter();
    void AddShowViewDirective();
    void FinalizeSessionState();
    void FinalizeAnalytics();
    void FinalizeBuilder();
    void FinalizeContinueBuilder();

private:
    TContextWrapper& ContextWrapper_;
    const TClassificationResult& ClassificationResult_;
    const TMaybe<TReplyState>& ReplyState_;
    const TMaybe<TVector<TNlgSearchReplyCandidate>>& SuggestsState_;
    THolder<IReplySourceRenderStrategy> ReplySourceRenderStrategy_;
    TGeneralConversationResponseWrapper ResponseWrapper_;
    TSessionState sessionState_;
};

using TGeneralConversationRunResponseBuilder = TGeneralConversationResponseBuilder<TGeneralConversationRunContextWrapper>;
using TGeneralConversationContinueResponseBuilder = TGeneralConversationResponseBuilder<TGeneralConversationApplyContextWrapper>;

}  // namespace NAlice::NHollywood::NGeneralConversation
