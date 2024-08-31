#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NGeneralConversation {

class TGeneralConversationCandidatesAggregatorHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "candidates_aggregator";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

}  // namespace NAlice::NHollywood::NGeneralConversation
