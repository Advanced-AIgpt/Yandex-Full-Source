#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood::NImage {

class TImageWhatIsThisRender: public TScenario::THandleBase {
public:
    void Do(TScenarioHandleContext& ctx) const override;

    TString Name() const override {
        return "render";
    }

};

}
