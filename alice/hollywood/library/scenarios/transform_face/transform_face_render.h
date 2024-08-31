#pragma once

#include "transform_face_impl.h"

#include <alice/hollywood/library/base_scenario/scenario.h>

namespace NAlice::NHollywood {

class TTransformFaceRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
    void Fail(TTransformFaceContinueImpl& impl) const;
};

}
