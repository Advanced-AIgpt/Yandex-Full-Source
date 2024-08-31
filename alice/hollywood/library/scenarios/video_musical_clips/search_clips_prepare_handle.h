#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NMusicalClips {

class TSearchClipsPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "continue_search_clips";
    }
    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusicalClips
