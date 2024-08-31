#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

class TMusicSdkContinueRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "sdk/continue/render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic::NMusicSdk
