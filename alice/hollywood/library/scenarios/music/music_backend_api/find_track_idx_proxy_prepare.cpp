#include "find_track_idx_proxy_prepare.h"
#include "music_common.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/library/experiments/flags.h>

namespace NAlice::NHollywood::NMusic {

namespace NImpl {

std::pair<NAppHostHttp::THttpRequest, TStringBuf> FindTrackIdxPrepareProxyImpl(const TMusicQueueWrapper& mq, const TFindTrackIdxRequest& request,
                                                                      const NScenarios::TRequestMeta& meta, const TClientInfo& clientInfo,
                                                                      TRTLogger& logger, const TStringBuf userId,
                                                                      const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo)
{
    TString path;
    if (request.GetContentType() == TFindTrackIdxRequest_EContentType_Album) {
        path = NApiPath::AlbumTracks(mq.ContentId().GetId(),
                                     NApiPath::TApiPathRequestParams(/* pageIdx = */ 0, mq.Config().FindTrackIdxPageSize,
                                                                     mq.GetShuffle(), mq.GetShuffleSeed()),
                                     /* richTracks = */ false, userId, request.GetShouldUseResumeFrom());
    } else if (request.GetContentType() == TFindTrackIdxRequest_EContentType_Artist) {
        path = NApiPath::TrackIdsOfArtist(mq.ContentId().GetId(),
                                          NApiPath::TApiPathRequestParams(/* pageIdx = */ 0, mq.Config().FindTrackIdxPageSize,
                                                                          mq.GetShuffle(), mq.GetShuffleSeed()),
                                          userId);
    } else if (request.GetContentType() == TFindTrackIdxRequest_EContentType_Playlist) {
        auto playlistId = TPlaylistId::FromString(mq.ContentId().GetId());
        path = NApiPath::PlaylistTracks(playlistId->Owner, playlistId->Kind,
                                        NApiPath::TApiPathRequestParams(/* pageIdx = */ 0, mq.Config().FindTrackIdxPageSize,
                                                                        mq.GetShuffle(), mq.GetShuffleSeed()),
                                        /* richTracks = */ false, userId);
    } else {
        ythrow yexception() << "Unexpected ContentId.Type="
                            << TFindTrackIdxRequest_EContentType_Name(request.GetContentType())
                            << "for find_track_idx request";
    }
    auto req = TMusicRequestBuilder(path, meta, clientInfo, logger, enableCrossDc, musicRequestModeInfo, "FindTrackIdx")
        .BuildAndMove();
    return {std::move(req), MUSIC_FIND_TRACK_IDX_REQUEST_ITEM};
}

} // NImpl

void TFindTrackIdxProxyPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    auto mCtx = GetMusicContext(ctx.ServiceCtx);
    auto& scState = *mCtx.MutableScenarioState();
    TryInitPlaybackContextBiometryOptions(logger, scState);
    const auto& biometryOpts = scState.GetQueue().GetPlaybackContext().GetBiometryOptions();

    TMusicQueueWrapper mq(logger, *scState.MutableQueue());
    const bool enableCrossDc = request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);
    auto musicRequestModeInfo = MakeMusicRequestModeInfo(EAuthMethod::UserId, mCtx.GetAccountStatus().GetUid(), scState);
    auto[req, item] = NImpl::FindTrackIdxPrepareProxyImpl(mq, mCtx.GetFindTrackIdxRequest(), ctx.RequestMeta,
                                                          request.ClientInfo(), logger,
                                                          biometryOpts.GetUserId(), enableCrossDc, musicRequestModeInfo);
    AddMusicProxyRequest(ctx, req, item);
}

} // NAlice::NHollywood::NMusic
