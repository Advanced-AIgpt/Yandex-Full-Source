#pragma once

#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

namespace NAlice::NHollywood::NMusic {

class TTrackFullInfoProxyPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "thin_track_full_info_proxy_prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TTrackSearchProxyPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "thin_track_search_proxy_prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic
