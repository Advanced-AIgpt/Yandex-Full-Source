#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/analytics/scenarios/general_conversation/general_conversation.pb.h>

namespace NAlice::NHollywood::NGeneralConversation {

struct TGeneralConversationResponseWrapper {
    TGeneralConversationResponseWrapper(const TSessionState& sessionState, TNlgWrapper& nlgWrapper)
        : SessionState(sessionState)
        , Builder(&nlgWrapper)
        , ContinueBuilder(&nlgWrapper)
        , GcResponseInfo()
    {}

    TSessionState SessionState;
    TRunResponseBuilder Builder;
    TContinueResponseBuilder ContinueBuilder;
    NScenarios::NGeneralConversation::TGCResponseInfo GcResponseInfo;
};

class IReplySourceRenderStrategy {
public:
    virtual ~IReplySourceRenderStrategy() = default;
    virtual void AddResponse(TGeneralConversationResponseWrapper* responseWrapper) = 0;
    virtual void AddSuggests(TGeneralConversationResponseWrapper* responseWrapper) {
        Y_UNUSED(responseWrapper);
    };
    virtual TMaybe<TSemanticFrame> GetSemanticFrame() {
        return Nothing();
    };
    virtual bool NeedCommonSuggests() const { return true; }

    virtual bool ShouldListen() const { return true; }
};

} // namespace NAlice::NHollywood::NGeneralConversation
