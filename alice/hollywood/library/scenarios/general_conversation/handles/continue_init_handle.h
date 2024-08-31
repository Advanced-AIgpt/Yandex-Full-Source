#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NGeneralConversation {

class TGeneralConversationContinueInitHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "init_continue";
    }

    void Do(TScenarioHandleContext& ctx_c) const override;
};

}  // namespace NAlice::NHollywood::NGeneralConversation
