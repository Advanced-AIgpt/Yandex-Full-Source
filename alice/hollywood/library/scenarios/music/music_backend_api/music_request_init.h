#pragma once

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>

namespace NAlice::NHollywood::NMusic {

void AddMusicContentRequest(TScenarioHandleContext& ctx, const NHollywood::TScenarioApplyRequestWrapper& applyRequest);

class TContentProxyPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return TString{TStringBuf("thin_content_proxy_prepare")};
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic
