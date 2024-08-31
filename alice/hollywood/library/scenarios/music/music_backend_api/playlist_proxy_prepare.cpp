#include "playlist_proxy_prepare.h"
#include "music_common.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

namespace NAlice::NHollywood::NMusic {

std::pair<NAppHostHttp::THttpRequest, TStringBuf> PlaylistSearchPrepareProxyImpl(const TPlaylistRequest& request,
                                                                        const NScenarios::TRequestMeta& meta,
                                                                        const TClientInfo& clientInfo,
                                                                        TRTLogger& logger,
                                                                        const TString& userId,
                                                                        const bool enableCrossDc,
                                                                        const TMusicRequestModeInfo& musicRequestMode) {
    TString path;
    TStringBuf item;
    TString name;
    if (request.GetPlaylistType() == TPlaylistRequest_EPlaylistType_Normal) {
        path = NApiPath::PlaylistSearch(userId, request.GetPlaylistName());
        item = MUSIC_PLAYLIST_SEARCH_REQUEST_ITEM;
        name = "PlaylistSearch";
    } else {
        path = NApiPath::SpecialPlaylist(userId, ConvertSpecialPlaylistId(request.GetPlaylistName()));
        item = MUSIC_SPECIAL_PLAYLIST_REQUEST_ITEM;
        name = "SpecialPlaylist";
    }
    auto req = TMusicRequestBuilder(path, meta, clientInfo, logger, enableCrossDc, musicRequestMode, name)
        .BuildAndMove();
    return {std::move(req), item};
}

void TPlaylistSearchProxyPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};
    auto mCtx = GetMusicContext(ctx.ServiceCtx);
    const bool enableCrossDc = request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);
    const auto& applyArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();

    auto isClientBiometryModeRequest = IsClientBiometryModeApplyRequest(logger, applyArgs);
    const auto& userId = isClientBiometryModeRequest ? applyArgs.GetGuestCredentials().GetUid() : mCtx.GetAccountStatus().GetUid();
    auto musicRequestModeInfoBuilder = TMusicRequestModeInfoBuilder()
                            .SetAuthMethod(EAuthMethod::UserId)
                            .SetOwnerUserId(mCtx.GetAccountStatus().GetUid())
                            .SetRequesterUserId(userId);
    if (isClientBiometryModeRequest && userId != mCtx.GetAccountStatus().GetUid()) {
        musicRequestModeInfoBuilder.SetRequestMode(ERequestMode::Guest);
    } else {
        musicRequestModeInfoBuilder.SetRequestMode(ERequestMode::Owner);
    }

    auto[req, item] = PlaylistSearchPrepareProxyImpl(mCtx.GetPlaylistRequest(), ctx.RequestMeta,
                                                     request.ClientInfo(),
                                                     logger, userId,
                                                     enableCrossDc, musicRequestModeInfoBuilder.BuildAndMove());
    AddMusicProxyRequest(ctx, req, item);
}

} // NAlice::NHollywood::NMusic
