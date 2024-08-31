#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/response_builders/reply_source_render_strategy.h>

namespace NAlice::NHollywood::NGeneralConversation {

class TErrorRenderStrategy : public IReplySourceRenderStrategy {
public:
    TErrorRenderStrategy(TGeneralConversationRunContextWrapper& contextWrapper, TStringBuf errorType);

public:
    virtual void AddResponse(TGeneralConversationResponseWrapper* responseWrapper) override;

private:
    TGeneralConversationRunContextWrapper& ContextWrapper_;
    TString ErrorType_;
};

} // namespace NAlice::NHollywood::NGeneralConversation
