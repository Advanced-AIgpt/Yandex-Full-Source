#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NImage {

class TImageWhatIsThisHandle: public TScenario::THandleBase {
public:
    TImageWhatIsThisHandle();

    TString Name() const override {
        return "run";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood
