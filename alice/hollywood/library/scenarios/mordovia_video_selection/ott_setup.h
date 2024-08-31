#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NMordovia {

class TOttSetup final : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "ott_setup";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMordovia
