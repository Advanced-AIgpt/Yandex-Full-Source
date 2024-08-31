#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NImage {

class TImageWhatIsThisIntHandle: public TScenario::THandleBase {
public:
    TString Name() const override {
        return "int_handle";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood
