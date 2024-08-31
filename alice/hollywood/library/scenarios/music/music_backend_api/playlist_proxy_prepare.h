#pragma once

#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

namespace NAlice::NHollywood::NMusic {

std::pair<NAppHostHttp::THttpRequest, TStringBuf> PlaylistSearchPrepareProxyImpl(const TPlaylistRequest& request,
                                                                        const NScenarios::TRequestMeta& meta,
                                                                        const TClientInfo& clientInfo,
                                                                        TRTLogger& logger,
                                                                        const TString& userId,
                                                                        const bool enableCrossDc,
                                                                        const TMusicRequestModeInfo& musicRequestMode);

class TPlaylistSearchProxyPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "thin_playlist_search_proxy_prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic
