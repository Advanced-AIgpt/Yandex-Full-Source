#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/response_builders/reply_source_render_strategy.h>

namespace NAlice::NHollywood::NGeneralConversation {

class TEasterEggRenderStrategy : public IReplySourceRenderStrategy {
public:
    TEasterEggRenderStrategy(TGeneralConversationRunContextWrapper& contextWrapper, const TClassificationResult& classificationResult, const TReplyInfo& replyInfo);

public:
    virtual void AddResponse(TGeneralConversationResponseWrapper* responseWrapper) override;
    virtual void AddSuggests(TGeneralConversationResponseWrapper* responseWrapper) override;
    virtual bool NeedCommonSuggests() const override { return false; }

private:
    TGeneralConversationRunContextWrapper& ContextWrapper_;
    const TClassificationResult& ClassificationResult_;
    const TReplyInfo& ReplyInfo_;
};

} // namespace NAlice::NHollywood::NGeneralConversation
