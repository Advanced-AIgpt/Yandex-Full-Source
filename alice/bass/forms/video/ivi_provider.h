#pragma once

#include "defs.h"
#include "utils.h"
#include "video_provider.h"

#include <alice/bass/libs/source_request/handle.h>
#include <alice/bass/libs/video_common/utils.h>

namespace NBASS {
namespace NVideo {

enum class EIviRequestPath {
    Catalogue,
    CollectionCatalog,
    Compilations,
    Search,
    Videos
};

class TIviSourceRequestFactory : public TProviderSourceRequestFactory {
public:
    explicit TIviSourceRequestFactory(TSourcesRequestFactory sources);
    explicit TIviSourceRequestFactory(TContext& ctx);
};

class TIviClipsProvider : public TVideoClipsHttpProviderBase {
public:
    explicit TIviClipsProvider(TContext& context)
        : TVideoClipsHttpProviderBase(context)
    {
    }

    // IVideoClipsProvider overrides:
    std::unique_ptr<IVideoClipsHandle> MakeSearchRequest(const TVideoClipsRequest& request) const override;

    std::unique_ptr<IWebSearchByProviderHandle>
    MakeWebSearchRequest(const TVideoClipsRequest& /*request*/) const override;

    std::unique_ptr<IVideoClipsHandle> MakeNewVideosRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeTopVideosRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeRecommendationsRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeVideosByGenreRequest(const TVideoClipsRequest& request) const override;

    TStringBuf GetProviderName() const override {
        return NVideoCommon::PROVIDER_IVI;
    }

    bool IsUnauthorized() const override {
        return GetAuthToken().empty();
    }

public:
    // TVideoClipsHttpProviderBase overrides:
    std::unique_ptr<IRequestHandle<TString>>
    FetchAuthToken(TStringBuf passportUid, NHttpFetcher::IMultiRequest::TRef /*multiRequest*/) const override;

protected:
    // IVideoClipsProvider overrides:
    TPlayResult GetPlayCommandDataImpl(TVideoItemConstScheme item,
                                       TPlayVideoCommandDataScheme commandData) const override;

protected:
    // TVideoClipsHttpProviderBase overrides:
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
} // namespace NBASS
