#pragma once

#include "video_provider.h"

#include <alice/bass/libs/source_request/handle.h>

namespace NBASS {
namespace NVideo {

class TYouTubeClipsProvider : public TVideoClipsHttpProviderBase {
public:
    explicit TYouTubeClipsProvider(TContext& context)
        : TVideoClipsHttpProviderBase(context)
    {
    }

    // IVideoClipsProvider overrides:
    std::unique_ptr<IVideoClipsHandle> MakeSearchRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeNewVideosRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeRecommendationsRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeTopVideosRequest(const TVideoClipsRequest& request) const override;

    TStringBuf GetProviderName() const override {
        return NVideoCommon::PROVIDER_YAVIDEO;
    }

    bool IsUnauthorized() const override {
        return false;
    }

    std::unique_ptr<IRequestHandle<TString>>
    FetchAuthToken(TStringBuf passportUid, NHttpFetcher::IMultiRequest::TRef /* multiRequest */) const override;

protected:
    // IVideoClipsProvider overrides:
    TPlayResult GetPlayCommandDataImpl(TVideoItemConstScheme item,
                                       TPlayVideoCommandDataScheme commandData) const override;
    std::unique_ptr<IVideoItemHandle> DoMakeContentInfoRequest(TVideoItemConstScheme item,
                                                               NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                               bool /* forceUnfiltered */) const override;
};

} // namespace NVideo
} // namespace NBASS
