#pragma once

#include "video_provider.h"

/*
 * It is stub provider for serving server_action request with yavideo_proxy provider. Such a request
 * should by design contain all necessary data for launching video on device (showing preview, description etc.)
 * This provider acts only as a proxy for data came with server_action. Data retrieve is not responsobility
 * of this class
 * see https://st.yandex-team.ru/QUASAR-2964 for more info
 */

namespace NBASS {
namespace NVideo {

class TYaVideoProxyClipsProvider : public TVideoClipsHttpProviderBase {
public:
    explicit TYaVideoProxyClipsProvider(TContext& context)
        : TVideoClipsHttpProviderBase(context)
    {
    }

    TStringBuf GetProviderName() const override {
        return NVideoCommon::PROVIDER_YAVIDEO_PROXY;
    }

    bool IsUnauthorized() const override {
        return false;
    }

protected:
    // IVideoClipsProvider overrides:
    TPlayResult GetPlayCommandDataImpl(TVideoItemConstScheme item,
                                       TPlayVideoCommandDataScheme commandData) const override;
};

} // namespace NVideo
} // namespace NBASS
