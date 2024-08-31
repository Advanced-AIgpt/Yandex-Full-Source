#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NGeneralConversation {

class TGeneralConversationInitHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "init";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

}  // namespace NAlice::NHollywood::NGeneralConversation
