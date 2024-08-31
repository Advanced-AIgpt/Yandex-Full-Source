#include "novelty_album_search_prepare.h"
#include "music_common.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

namespace NAlice::NHollywood::NMusic {

void TNoveltyAlbumSearchProxyPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    auto mCtx = GetMusicContext(ctx.ServiceCtx);
    auto& scState = *mCtx.MutableScenarioState();
    TryInitPlaybackContextBiometryOptions(logger, scState);
    const auto& applyArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
    const auto& biometryOpts = scState.GetQueue().GetPlaybackContext().GetBiometryOptions();

    TString artistId = mCtx.GetOnDemandRequest().GetArtistId();
    LOG_INFO(logger) << "ArtistId=" << artistId;
    if (artistId.empty()) {
        LOG_INFO(logger) << "Unexpectedly got empty ArtistId, skipping preparation of request for novelty album";
        return;
    }
    const bool enableCrossDc = request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);
    auto musicRequestModeInfo = MakeMusicRequestModeInfo(EAuthMethod::UserId, applyArgs.GetAccountStatus().GetUid(), scState);
    auto req = TMusicRequestBuilder(NApiPath::LatestAlbumOfArtist(artistId, biometryOpts.GetUserId()),
                                    ctx.RequestMeta, request.ClientInfo(), logger,
                                    enableCrossDc, musicRequestModeInfo, TString(MUSIC_NOVELTY_ALBUM_SEARCH_REQUEST_ITEM))
        .BuildAndMove();

    AddMusicProxyRequest(ctx, req, MUSIC_NOVELTY_ALBUM_SEARCH_REQUEST_ITEM);
}

} // NAlice::NHollywood::NMusic
