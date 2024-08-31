#pragma once

#include "fast_command_scenario_run_context.h"
#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/hollywood/library/scenarios/fast_command/nlg/register.h>

namespace NAlice::NHollywood {

class TFastCommandRunHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "run";
    }

    void Do(TScenarioHandleContext& ctx) const override;
private:
    void DoImpl(TFastCommandScenarioRunContext& fastCommandScenarioRunContext) const;
};

}  // namespace NAlice::NHollywood
