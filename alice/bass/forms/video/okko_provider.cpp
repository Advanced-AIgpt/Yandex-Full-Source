#include "okko_provider.h"

#include "defs.h"
#include "utils.h"
#include "web_search.h"

#include <alice/bass/forms/video/universal_api_utils.h>

#include <alice/bass/libs/video_common/okko_utils.h>
#include <alice/bass/libs/video_common/video.sc.h>

#include <alice/bass/setup/setup.h>

using namespace NVideoCommon;

namespace NBASS {
namespace NVideo {

namespace {

TResultValue Unimplemented() {
    return TError{TError::EType::VIDEOERROR, "Unimplemented okko stub"};
}

// FIXME: stub.
class TOkkoSearchHandle : public IVideoClipsHandle {
public:
    TOkkoSearchHandle() {
    }

    TResultValue WaitAndParseResponse(TVideoGalleryScheme* /* response */) override {
        return Unimplemented();
    }
};

// FIXME: stub.
class TContentInfoHandle : public IVideoItemHandle {
public:
    TContentInfoHandle(TContext& /* context */, TVideoItemConstScheme /* item */,
                       NHttpFetcher::IMultiRequest::TRef /* multiRequest */) {
    }

    NVideoCommon::TResult WaitAndParseResponse(TVideoItem& /* item */) override {
        return NVideoCommon::TError{"Unimplemented okko stub"};
    }
};

// FIXME: stub.
class TRecommendationHandle : public IVideoClipsHandle {
public:
    TRecommendationHandle() {
    }

    TResultValue WaitAndParseResponse(TVideoGalleryScheme* /* response */) override {
        return Unimplemented();
    }
};

class TOkkoWebSearchHandle : public TWebSearchByProviderHandle {
public:
    TOkkoWebSearchHandle(const TVideoClipsRequest& request, TContext& context)
        : TWebSearchByProviderHandle(PatchQuery(request.Slots.BuildSearchQueryForWeb()), request, PROVIDER_OKKO,
                                     context) {
    }

    static TString PatchQuery(TStringBuf query) {
        return TStringBuilder() << query << TStringBuf(" host:www.okko.tv");
    }

    bool ParseItemFromDoc(const NSc::TValue doc, TVideoItem* item) override {
        if (!item)
            return false;

        const TStringBuf url = doc["url"].GetString();
        return ParseOkkoItemFromUrl(url, *item);
    }
};

} // namespace

std::unique_ptr<IVideoItemHandle>
TOkkoClipsProvider::DoMakeContentInfoRequest(TVideoItemConstScheme item,
                                             NHttpFetcher::IMultiRequest::TRef multiRequest,
                                             bool /* forceUnfiltered */) const {
    return std::make_unique<TContentInfoHandle>(Context, item, multiRequest);
}

std::unique_ptr<IVideoClipsHandle> TOkkoClipsProvider::MakeSearchRequest(const TVideoClipsRequest& /* request */) const {
    return std::make_unique<TOkkoSearchHandle>();
}

std::unique_ptr<IVideoClipsHandle> TOkkoClipsProvider::MakeNewVideosRequest(const TVideoClipsRequest& /* request */) const {
    return std::make_unique<TOkkoSearchHandle>();
}

std::unique_ptr<IVideoClipsHandle>
TOkkoClipsProvider::MakeRecommendationsRequest(const TVideoClipsRequest& /* request */) const {
    return std::make_unique<TRecommendationHandle>();
}

std::unique_ptr<IVideoClipsHandle> TOkkoClipsProvider::MakeTopVideosRequest(const TVideoClipsRequest& /* request */) const {
    return std::make_unique<TRecommendationHandle>();
}

std::unique_ptr<IVideoClipsHandle> TOkkoClipsProvider::MakeVideosByGenreRequest(const TVideoClipsRequest& /* request */) const {
    return std::make_unique<TOkkoSearchHandle>();
}

std::unique_ptr<IWebSearchByProviderHandle>
TOkkoClipsProvider::MakeWebSearchRequest(const TVideoClipsRequest& request) const {
    return FetchSetupRequest<NVideo::TWebSearchByProviderResponse>(MakeHolder<TOkkoWebSearchHandle>(request, Context),
                                                                   Context, request.MultiRequest);
}

IVideoClipsProvider::TPlayResult
TOkkoClipsProvider::GetPlayCommandDataImpl(TVideoItemConstScheme item, TPlayVideoCommandDataScheme commandData) const {
    auto itemId = TString{*item->ProviderItemId()};
    commandData->Uri() = itemId;
    return ResultSuccess();
}

} // namespace NVideo
} // namespace NBASS
