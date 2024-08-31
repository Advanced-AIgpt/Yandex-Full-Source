#pragma once

#include "transform_face_impl.h"

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood {

class TTransformFaceContinueHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "continue";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood
