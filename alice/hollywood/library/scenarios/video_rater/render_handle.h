#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NVideoRater {

class TVideoRaterRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

enum class EScenarioPhase {
    Init,
    Rate,
    Quit,
    DontUnderstand,
    Irrelevant,
};

}  // namespace NAlice::NHollywood::NVideoRater
