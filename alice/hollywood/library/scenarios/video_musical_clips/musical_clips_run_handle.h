#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NMusicalClips {

class TMusicalClipsRunHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "run";
    }
    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusicalClips