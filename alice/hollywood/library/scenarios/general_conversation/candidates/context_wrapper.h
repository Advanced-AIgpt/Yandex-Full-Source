#pragma once

#include <alice/hollywood/library/scenarios/general_conversation/containers/general_conversation_fast_data.h>
#include <alice/hollywood/library/scenarios/general_conversation/containers/general_conversation_resources.h>

#include <alice/hollywood/library/base_scenario/fwd.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>

namespace NAlice::NHollywood::NGeneralConversation {

template <typename TRequest, typename TRequestWrapper>
class TGeneralConversationContextWrapper {
public:
    explicit TGeneralConversationContextWrapper(TScenarioHandleContext* ctx);

public:
    TScenarioHandleContext* Ctx();
    const TGeneralConversationFastData* FastData();
    TRTLogger& Logger();
    TNlgWrapper& NlgWrapper();
    const TGeneralConversationResources& Resources() const;
    const TRequestWrapper& RequestWrapper() const;
    IRng& Rng();

private:
    TScenarioHandleContext* Ctx_;
    const TRequest ScenarioRunRequest_;
    const TRequestWrapper RequestWrapper_;
    TNlgWrapper NlgWrapper_;
    const std::shared_ptr<const TGeneralConversationFastData> FastData_;
};

using TGeneralConversationRunContextWrapper = TGeneralConversationContextWrapper<NScenarios::TScenarioRunRequest, TScenarioRunRequestWrapper>;
using TGeneralConversationApplyContextWrapper = TGeneralConversationContextWrapper<NScenarios::TScenarioApplyRequest, TScenarioApplyRequestWrapper>;

} // namespace NAlice::NHollywood::NGeneralConversation
