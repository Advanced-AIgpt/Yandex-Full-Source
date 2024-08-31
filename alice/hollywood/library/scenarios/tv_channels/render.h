#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

using namespace NAlice::NScenarios;
using namespace NAlice::NHollywood;

namespace NAlice::NHollywood::NTvChannels {

class TSwitchTvChannelRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};


std::unique_ptr<TScenarioRunResponse> RenderSwitchTvChannel(TRTLogger& logger, const TFrame& frame, const TString& url,
                                                            const TString& title, const TString& number,
                                                            const TScenarioHandleContext& ctx,
                                                            const TScenarioRunRequestWrapper& request);

} // namespace NAlice::NHollywood::NTvChannels
