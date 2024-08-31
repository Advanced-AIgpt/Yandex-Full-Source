#include "collect_main_screen_prepare.h"
#include "common.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>

#include <alice/hollywood/library/scenarios/music/proto/centaur.pb.h>

namespace NAlice::NHollywood::NMusic::NCentaur {

namespace {

class TImpl {
public:
    TImpl(THwServiceContext& ctx)
        : Ctx_{ctx}
        , Request_{ctx.GetProtoOrThrow<TCollectMainScreenRequest>(COLLECT_MAIN_SCREEN_REQUEST_ITEM)}
    {
    }

    void Do() {
        // construct infinite feed request
        NAppHostHttp::THttpRequest infiniteFeedRequest = ConstructHttpRequest(
            NApiPath::InfiniteFeed(Request_.GetMusicRequestMeta().GetUserId()),
            "InfiniteFeed"
        );
        Ctx_.AddProtobufItemToApphostContext(infiniteFeedRequest, INFINITE_FEED_HTTP_REQUEST_ITEM);

        // construct children landing catalogue request
        NAppHostHttp::THttpRequest childrenLandingCatalogueRequest = ConstructHttpRequest(
            NApiPath::ChildrenLandingCatalogue(Request_.GetMusicRequestMeta().GetUserId()),
            "ChildrenLandingCatalogue"
        );
        Ctx_.AddProtobufItemToApphostContext(childrenLandingCatalogueRequest, CHILDREN_LANDING_CATALOGUE_HTTP_REQUEST_ITEM);
    }

private:
    NAppHostHttp::THttpRequest ConstructHttpRequest(TStringBuf path, TStringBuf name) {
        auto modeInfo = TMusicRequestModeInfoBuilder()
            .SetAuthMethod(EAuthMethod::OAuth)
            .SetRequestMode(ERequestMode::Owner)
            .BuildAndMove();

        auto builder = TMusicRequestBuilder{path, Request_.GetMusicRequestMeta(), Ctx_.Logger(), modeInfo, TString{name}};
        return builder
            .SetUseOAuth()
            .Build();
    }

private:
    THwServiceContext& Ctx_;
    const TCollectMainScreenRequest Request_;
};

} // namespace

const TString& TCollectMainScreenPrepareHandle::Name() const {
    const static TString name = "music/centaur_collect_main_screen_prepare";
    return name;
}

void TCollectMainScreenPrepareHandle::Do(THwServiceContext& ctx) const {
    TImpl{ctx}.Do();
}

} // namespace NAlice::NHollywood::NMusic::NCentaur
