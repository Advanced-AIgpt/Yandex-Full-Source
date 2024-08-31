#pragma once

#include <alice/hollywood/library/scenarios/how_to_spell/nlg/register.h>

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood {

class THowToSpellRunHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "run";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

}  // namespace NAlice::NHollywood
