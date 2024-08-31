#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

class TMusicSdkContinuePrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "sdk/continue/prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic::NMusicSdk