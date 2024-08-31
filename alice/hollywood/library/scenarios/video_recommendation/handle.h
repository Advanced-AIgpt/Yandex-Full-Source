#pragma once

#include "video_database.h"

#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/hollywood/library/scenarios/video_recommendation/nlg/register.h>

namespace NAlice::NHollywood {

class TVideoRecommendationRunHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "run";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

}  // namespace NAlice::NHollywood
