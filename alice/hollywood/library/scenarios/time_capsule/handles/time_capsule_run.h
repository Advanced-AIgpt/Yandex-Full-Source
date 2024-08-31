#pragma once

#include <alice/hollywood/library/scenarios/time_capsule/scene/scene_creators.h>

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NTimeCapsule {

class TTimeCapsuleRunHandle : public TScenario::THandleBase {
public:
    TTimeCapsuleRunHandle();

    TString Name() const override {
        return "run";
    }

    void Do(TScenarioHandleContext& ctx) const override;

private:
    std::array<TSceneCreatorPtr, 13> TimeCapsuleSceneCreators_;
    TSceneCreatorPtr TimeCapsuleIrrelevantSceneCreator_;
};

} // namespace NAlice::NHollywood::NTimeCapsule
