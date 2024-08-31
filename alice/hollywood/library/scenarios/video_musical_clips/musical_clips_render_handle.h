#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NMusicalClips {

class TMusicalClipsRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "continue_render";
    }
    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusicalClips
