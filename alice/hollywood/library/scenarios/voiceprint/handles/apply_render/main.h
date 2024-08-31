#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NVoiceprint {

class TApplyRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "apply_render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NVoiceprint
