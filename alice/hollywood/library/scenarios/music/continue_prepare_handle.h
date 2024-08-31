#pragma once

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>

#include <alice/library/logger/fwd.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

namespace NAlice::NHollywood::NMusic {

class TBassMusicContinuePrepareHandle final : public TScenario::THandleBase {
public:
    void Do(TScenarioHandleContext& ctx) const override;

    TString Name() const override {
        return "continue_prepare";
    }
};

} // namespace NAlice::NHollywood::NMusic
