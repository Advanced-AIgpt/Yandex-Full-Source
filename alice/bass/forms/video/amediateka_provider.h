#pragma once

#include "defs.h"
#include "utils.h"
#include "video_provider.h"

#include <alice/bass/libs/socialism/socialism.h>
#include <alice/bass/libs/source_request/handle.h>

#include <util/generic/maybe.h>

namespace NBASS {
namespace NVideo {

class TAmediatekaSourceRequestFactory : public TProviderSourceRequestFactory {
public:
    explicit TAmediatekaSourceRequestFactory(TSourcesRequestFactory sources);
    explicit TAmediatekaSourceRequestFactory(TContext& ctx);
};

class TAmediatekaClipsProvider : public TVideoClipsHttpProviderBase {
public:
    explicit TAmediatekaClipsProvider(TContext& context)
        : TVideoClipsHttpProviderBase(context)
    {
    }

    // IVideoClipsProvider overrides:
    std::unique_ptr<IVideoClipsHandle> MakeSearchRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IWebSearchByProviderHandle> MakeWebSearchRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeNewVideosRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeTopVideosRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeRecommendationsRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeVideosByGenreRequest(const TVideoClipsRequest& request) const override;

    TStringBuf GetProviderName() const override {
        return NVideoCommon::PROVIDER_AMEDIATEKA;
    }

    bool IsUnauthorized() const override {
        return GetAuthToken().empty();
    }

public:
    // TVideoClipsHttpProviderBase overrides:
    std::unique_ptr<IRequestHandle<TString>>
    FetchAuthToken(TStringBuf passportUid, NHttpFetcher::IMultiRequest::TRef multiRequest) const override;

protected:
    // IVideoClipsProvider overrides:
    TPlayResult GetPlayCommandDataImpl(TVideoItemConstScheme item,
                                       TPlayVideoCommandDataScheme commandData) const override;

protected:
    // TVideoClipsHttpProviderBase:
    std::unique_ptr<IVideoItemHandle> DoMakeContentInfoRequest(TVideoItemConstScheme item,
                                                               NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                               bool /* forceUnfiltered */) const override;
    TResultValue DoGetSerialDescriptor(TVideoItemConstScheme tvShowItem,
                                       TSerialDescriptor* serialDescr) const override;
    std::unique_ptr<ISeasonDescriptorHandle>
    DoMakeSeasonDescriptorRequest(const TSerialDescriptor& serialDescr, const TSeasonDescriptor& seasonDescr,
                                  NHttpFetcher::IMultiRequest::TRef multiRequest) const override;
};

} // namespace NVideo
} // namespace NBass
