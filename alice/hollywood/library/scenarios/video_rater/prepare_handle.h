#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NVideoRater {

class TVideoRaterPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

}  // namespace NAlice::NHollywood::NVideoRater
