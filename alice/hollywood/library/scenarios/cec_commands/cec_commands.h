#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/scenarios/cec_commands/nlg/register.h>

namespace NAlice::NHollywood {
    class TCecCommandsHandle: public TScenario::THandleBase {
    public:
        TString Name() const override {
            return "run";
        }

        void Do(TScenarioHandleContext& ctx) const override;
    };

}
