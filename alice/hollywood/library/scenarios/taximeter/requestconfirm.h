#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/hollywood/library/scenarios/taximeter/nlg/register.h>

namespace NAlice::NHollywood {

class TTaximeterRequestConfirmRunHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "run";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

}  // namespace NAlice::NHollywood

