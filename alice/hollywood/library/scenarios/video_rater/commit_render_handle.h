#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/scenarios/video_rater/proto/video_rater_state.pb.h>

namespace NAlice::NHollywood::NVideoRater {

class TVideoRaterCommitRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "commit_render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

}  // namespace NAlice::NHollywood::NVideoRater
