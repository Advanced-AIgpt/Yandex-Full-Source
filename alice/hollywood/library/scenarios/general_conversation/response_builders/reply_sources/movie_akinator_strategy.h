#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/response_builders/reply_source_render_strategy.h>
#include <alice/hollywood/library/scenarios/suggesters/movie_akinator/response_body_builder.h>

namespace NAlice::NHollywood::NGeneralConversation {

class TMovieAkinatorRenderStrategy : public IReplySourceRenderStrategy {
public:
    TMovieAkinatorRenderStrategy(TGeneralConversationRunContextWrapper& contextWrapper, const TClassificationResult& classificationResult, const TReplyInfo& replyInfo);

public:
    virtual void AddResponse(TGeneralConversationResponseWrapper* responseWrapper) override;
    virtual void AddSuggests(TGeneralConversationResponseWrapper* responseWrapper) override;

private:
    TGeneralConversationRunContextWrapper& ContextWrapper_;
    const TClassificationResult& ClassificationResult_;
    TAkinatorSuggestsState AkinatorSuggestsState_;
};

} // namespace NAlice::NHollywood::NGeneralConversation
