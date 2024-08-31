#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

class TMusicSdkRunSearchContentPostHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "sdk/run/search_content_post";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic::NMusicSdk
