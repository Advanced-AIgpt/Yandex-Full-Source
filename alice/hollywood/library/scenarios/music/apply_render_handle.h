#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywood::NMusic {

class TApplyRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return TString{TStringBuf("apply_thin_client_render")};
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic
