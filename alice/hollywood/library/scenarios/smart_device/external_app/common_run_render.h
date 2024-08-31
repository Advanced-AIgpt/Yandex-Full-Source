#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood {

class TCommonrunRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "common_run_render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood
