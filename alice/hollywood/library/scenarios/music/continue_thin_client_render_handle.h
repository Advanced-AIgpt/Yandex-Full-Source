#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywood::NMusic {

namespace NImpl {


} // namespace NImpl

class TContinueThinClientRenderHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return TString{TStringBuf("continue_thin_client_render")};
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic
