#pragma once

#include "renderer.h"

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywood {

namespace NImpl {

std::unique_ptr<NAlice::NScenarios::TScenarioRunResponse> BassShowTrafficRenderDoImpl(
    const NHollywood::TScenarioRunRequestWrapper& runRequest,
    NJson::TJsonValue& bassResponse,
    TContext& ctx,
    NShowTrafficBass::TRenderer& renderer);

} // namespace NImpl

class TBassShowTrafficRenderHandle : public TScenario::THandleBase {
    TString Name() const override {
        return "render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood
