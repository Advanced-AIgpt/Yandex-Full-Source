#include "change_track_version_proxy_prepare.h"
#include "music_common.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/experiments/flags.h>

namespace NAlice::NHollywood::NMusic {

void TTrackFullInfoProxyPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    auto mCtx = GetMusicContext(ctx.ServiceCtx);
    auto& scState = *mCtx.MutableScenarioState();
    TryInitPlaybackContextBiometryOptions(logger, scState);
    const auto& biometryOpts = scState.GetQueue().GetPlaybackContext().GetBiometryOptions();

    const bool enableCrossDc = request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);
    auto musicRequestModeInfo = MakeMusicRequestModeInfo(EAuthMethod::UserId, mCtx.GetAccountStatus().GetUid(), scState);

    auto req = TMusicRequestBuilder(NApiPath::SingleTrack(mCtx.GetOnDemandRequest().GetTrackId(), biometryOpts.GetUserId(), true),
                                    ctx.RequestMeta, request.ClientInfo(),
                                    logger, enableCrossDc, musicRequestModeInfo).BuildAndMove();

    AddMusicProxyRequest(ctx, req, MUSIC_TRACK_FULL_INFO_REQUEST_ITEM);
}

void TTrackSearchProxyPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    auto mCtx = GetMusicContext(ctx.ServiceCtx);
    auto& scState = *mCtx.MutableScenarioState();
    TryInitPlaybackContextBiometryOptions(logger, scState);
    const auto& biometryOpts = scState.GetQueue().GetPlaybackContext().GetBiometryOptions();

    const bool enableCrossDc = request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);
    auto musicRequestModeInfo = MakeMusicRequestModeInfo(EAuthMethod::UserId, mCtx.GetAccountStatus().GetUid(), scState);

    auto req = TMusicRequestBuilder(NApiPath::TrackSearch(mCtx.GetOnDemandRequest().GetSearchText(), biometryOpts.GetUserId()),
                                    ctx.RequestMeta, request.ClientInfo(),
                                    logger, enableCrossDc, musicRequestModeInfo).BuildAndMove();

    AddMusicProxyRequest(ctx, req, MUSIC_TRACK_SEARCH_REQUEST_ITEM);
}

} // NAlice::NHollywood::NMusic
