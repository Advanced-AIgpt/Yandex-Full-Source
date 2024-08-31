#pragma once

#include "youtube_provider.h"
#include "yavideo_provider.h"

#include <alice/bass/libs/source_request/handle.h>

namespace NBASS::NVideo {

class TYouTubeWithYaVideoCInfoClipsProvider : public TYouTubeClipsProvider {
public:
    explicit TYouTubeWithYaVideoCInfoClipsProvider(TContext& context)
        : TYouTubeClipsProvider(context)
        , YaVideoProvider(std::make_unique<TYaVideoClipsProvider>(context))
    {
    }

protected:
    // IVideoClipsProvider overrides:
    std::unique_ptr<IVideoItemHandle> DoMakeContentInfoRequest(TVideoItemConstScheme item,
                                                               NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                               bool /* forceUnfiltered */) const override {
        return YaVideoProvider->MakeContentInfoRequest(item, multiRequest);
    }

    std::unique_ptr<IRequestHandle<TString>>
    FetchAuthToken(TStringBuf /*passportUid*/, NHttpFetcher::IMultiRequest::TRef /*multiRequest*/) const override {
        return std::make_unique<TDummyRequestHandle<TString>>();
    }

private:
    std::unique_ptr<IVideoClipsProvider> YaVideoProvider;
};

} // namespace NBASS::NVideo
