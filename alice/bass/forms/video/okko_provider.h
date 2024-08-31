#pragma once

#include "video_provider.h"

namespace NBASS {
namespace NVideo {

class TOkkoClipsProvider : public TVideoClipsHttpProviderBase {
public:
    explicit TOkkoClipsProvider(TContext& context)
        : TVideoClipsHttpProviderBase(context) {
    }

    // IVideoClipsProvider overrides:
    std::unique_ptr<IVideoItemHandle> DoMakeContentInfoRequest(TVideoItemConstScheme item,
                                                               NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                               bool /* forceUnfiltered */) const override;
    std::unique_ptr<IVideoClipsHandle> MakeSearchRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeNewVideosRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeRecommendationsRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeTopVideosRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeVideosByGenreRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IWebSearchByProviderHandle> MakeWebSearchRequest(const TVideoClipsRequest& request) const override;

    TStringBuf GetProviderName() const override {
        return NVideoCommon::PROVIDER_OKKO;
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
