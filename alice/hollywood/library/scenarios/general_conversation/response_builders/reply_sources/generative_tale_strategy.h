#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/response_builders/reply_source_render_strategy.h>

#include <alice/memento/proto/user_configs.pb.h>

namespace NMemento = ru::yandex::alice::memento::proto;

namespace NAlice::NHollywood::NGeneralConversation {

template <typename TContextWrapper>
class TGenerativeTaleRenderStrategy : public IReplySourceRenderStrategy {
public:
    TGenerativeTaleRenderStrategy(TContextWrapper& contextWrapper, const TClassificationResult& classificationResult, const TReplyInfo& replyInfo);

public:
    virtual void AddResponse(TGeneralConversationResponseWrapper* responseWrapper) override;
    virtual void AddSuggests(TGeneralConversationResponseWrapper* responseWrapper) override;
    virtual bool NeedCommonSuggests() const override { return false; }
    virtual bool ShouldListen() const override;
    virtual void UpdateMementoGenerativeTale(TResponseBodyBuilder* bodyBuilder);

private:
    void MakeSharingSuggests(TResponseBodyBuilder& responseBodyBuilder) const;

private:
    TContextWrapper& ContextWrapper_;
    const TClassificationResult& ClassificationResult_;
    const TReplyInfo& ReplyInfo_;
    NMemento::TGenerativeTale MementoGenerativeTale_;
    const NScenarios::TInterfaces Interfaces_;
};

} // namespace NAlice::NHollywood::NGeneralConversation
