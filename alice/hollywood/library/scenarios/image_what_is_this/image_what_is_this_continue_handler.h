#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NImage {

class TImageWhatIsThisContinueHandle: public TScenario::THandleBase {
public:
    TString Name() const override {
        return "continue";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood
