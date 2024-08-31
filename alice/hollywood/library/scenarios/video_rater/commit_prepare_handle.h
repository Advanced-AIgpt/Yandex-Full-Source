#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NVideoRater {

class TVideoRaterCommitPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "commit_prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

}  // namespace NAlice::NHollywood::NVideoRater
