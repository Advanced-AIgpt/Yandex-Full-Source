#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywood::NMusic {

namespace NImpl {

std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse> MusicApplyRenderDoImpl(
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
    const NJson::TJsonValue& bassResponse,
    NHollywood::TContext& ctx,
    TNlgWrapper& nlgWrapper,
    IRng& rng);

} // namespace NImpl

class TBassMusicContinueRenderHandle final : public TScenario::THandleBase {
public:
    void Do(TScenarioHandleContext& ctx) const override;

    TString Name() const override {
        return "continue_render";
    }
};

} // namespace NAlice::NHollywood::NMusic
