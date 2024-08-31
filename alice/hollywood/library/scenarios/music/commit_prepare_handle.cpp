#include "commit_prepare_handle.h"

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/scenarios/music/biometry/process_biometry.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/like_dislike_handlers.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/report_handlers.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/shots.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/experiments/flags.h>

namespace NAlice::NHollywood::NMusic {

namespace {

TScenarioState GetScenarioState(TRTLogger& logger, const NHollywood::TScenarioApplyRequestWrapper& request) {
    const auto& applyArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
    LOG_DEBUG(logger) << "Getting scenario state";

    TScenarioState scState;
    ReadScenarioState(request.BaseRequestProto(), scState);
    TryInitPlaybackContextBiometryOptions(logger, scState);
    auto& biometryOpts = *scState.MutableQueue()->MutablePlaybackContext()->MutableBiometryOptions();

    Y_ENSURE(applyArgs.HasAccountStatus());
    // Fill scState user id
    const auto biometryData = ProcessBiometryOrFallback(logger, request, TStringBuf{applyArgs.GetAccountStatus().GetUid()});
    Y_ENSURE(!biometryData.UserId.Empty());
    
    biometryOpts.SetUserId(biometryData.UserId);
    scState.SetBiometryUserId(biometryData.UserId);
    if (biometryData.IsIncognitoUser) {
        biometryOpts.SetPlaybackMode(TBiometryOptions_EPlaybackMode_IncognitoMode);
        scState.SetPlaybackMode(TScenarioState_EPlaybackMode_IncognitoMode);
    } else if (biometryData.IsGuestUser) {
        biometryOpts.SetPlaybackMode(TBiometryOptions_EPlaybackMode_GuestMode);
        scState.SetPlaybackMode(TScenarioState_EPlaybackMode_GuestMode);
    } else {
        biometryOpts.SetPlaybackMode(TBiometryOptions_EPlaybackMode_OwnerMode);
        scState.SetPlaybackMode(TScenarioState_EPlaybackMode_OwnerMode);
    }

    return scState;
}

}

void TCommitPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    std::variant<THttpProxyRequestItemPairs, NScenarios::TScenarioCommitResponse> result;

    const auto& applyArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
    LOG_INFO(logger) << "TMusicArguments are "
                     << SerializeProtoText(applyArgs, /* singleLineMode= */ true, /* expandAny= */ true);

    auto isClientBiometryModeApplyRequest = IsClientBiometryModeApplyRequest(logger, applyArgs);
    const auto& playerCommand = applyArgs.GetPlayerCommand();
    const bool enableCrossDc = request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);
    if (applyArgs.GetExecutionFlowType() == TMusicArguments_EExecutionFlowType_ComplexLikeDislike) {
        LOG_DEBUG(logger) << "Handling complex like/dislike";

        Y_ENSURE(applyArgs.HasComplexLikeDislikeRequest());
        const auto& likeDislike = applyArgs.GetComplexLikeDislikeRequest();

        auto scState = GetScenarioState(logger, request);
        auto musicRequestModeInfo = MakeMusicRequestModeInfoFromMusicArgs(applyArgs, scState, EAuthMethod::UserId, isClientBiometryModeApplyRequest);
        auto userId = musicRequestModeInfo.RequesterUserId;
        
        THttpProxyRequestItemPairs requests;
        if (likeDislike.HasArtistTarget()) {
            LOG_INFO(logger) << "CommitPrepareHandle: proceeding Complex Like/Dislike Artist command";
            requests.emplace_back(NMusic::WrapMusicLikeDislikeRequest(NMusic::MakeMusicLikeDislikeArtistRequest(
                ctx.RequestMeta, request.ClientInfo(), ctx.Ctx.Logger(),
                userId, /* isLikeCommand= */ likeDislike.GetIsLike(),
                likeDislike.GetArtistTarget().GetId(), enableCrossDc, musicRequestModeInfo)));
        } else if (likeDislike.HasTrackTarget()) {
            LOG_INFO(logger) << "CommitPrepareHandle: proceeding Complex Like/Dislike Track command";
            requests.push_back(NMusic::WrapMusicLikeDislikeRequest(NMusic::MakeMusicLikeDislikeTrackRequest(
                ctx.RequestMeta, request.ClientInfo(), ctx.Ctx.Logger(),
                userId, /* isLikeCommand= */ likeDislike.GetIsLike(),
                likeDislike.GetTrackTarget().GetId(), likeDislike.GetTrackTarget().GetAlbumId(), enableCrossDc,
                musicRequestModeInfo)));
        } else if (likeDislike.HasAlbumTarget()) {
            LOG_INFO(logger) << "CommitPrepareHandle: proceeding Complex Like/Dislike Album command";
            Y_ENSURE(likeDislike.GetIsLike()); // Album dislike is unsupported
            requests.emplace_back(NMusic::WrapMusicLikeDislikeRequest(NMusic::MakeMusicLikeAlbumRequest(
                ctx.RequestMeta, request.ClientInfo(), ctx.Ctx.Logger(),
                userId, likeDislike.GetAlbumTarget().GetId(), enableCrossDc, musicRequestModeInfo)));
        } else if (likeDislike.HasGenreTarget()) {
            LOG_INFO(logger) << "CommitPrepareHandle: proceeding Complex Like/Dislike Genre command";
            Y_ENSURE(likeDislike.GetIsLike()); // Genre dislike is unsupported
            requests.emplace_back(NMusic::WrapMusicLikeDislikeRequest(NMusic::MakeMusicLikeGenreRequest(
                ctx.RequestMeta, request.ClientInfo(), ctx.Ctx.Logger(),
                userId, likeDislike.GetGenreTarget().GetId(), enableCrossDc, musicRequestModeInfo)));
        } else {
            ythrow yexception() << "No target in Complex Like/Dislike request";
        }

        result = requests;
    } else if (playerCommand == TMusicArguments_EPlayerCommand_None) {
        result = NMusic::MakeMusicReportRequest(ctx.RequestMeta, request.ClientInfo(),
                                                ctx.Ctx.Logger(), request);
    } else if (playerCommand == TMusicArguments_EPlayerCommand_Like) {
        THttpProxyRequestItemPairs requests = {};

        auto scState = GetScenarioState(logger, request);
        const TMusicQueueWrapper mq(logger, *scState.MutableQueue());
        
        if (mq.IsGenerative()) {
            LOG_INFO(logger) << "CommitPrepareHandle: proceeding Like command for generative stream";

            auto musicRequestModeInfo = MakeMusicRequestModeInfoFromMusicArgs(applyArgs, scState, EAuthMethod::OAuth, isClientBiometryModeApplyRequest);
            auto metaProvider = MakeRequestMetaProviderFromMusicArgs(ctx.RequestMeta, applyArgs, isClientBiometryModeApplyRequest);

            requests.push_back(MakeTimestampGenerativeFeedbackProxyRequest(
                mq, metaProvider, request.ClientInfo(),
                ctx.Ctx.Logger(), request, GENERATIVE_FEEDBACK_TYPE_TIMESTAMP_LIKE,
                musicRequestModeInfo));
        } else if (applyArgs.GetIsShotPlaying()) {
            LOG_INFO(logger) << "TCommitPrepareHandle: proceeding Like command for shot";
            auto musicRequestModeInfo = MakeMusicRequestModeInfoFromMusicArgs(applyArgs, scState, EAuthMethod::UserId, isClientBiometryModeApplyRequest);
            requests.push_back(MakeShotsLikeDislikeFeedbackProxyRequest(
                mq, ctx.RequestMeta, request.ClientInfo(),
                logger, musicRequestModeInfo.RequesterUserId, /* isLike = */ true,
                enableCrossDc, musicRequestModeInfo));
        } else {
            LOG_INFO(logger) << "CommitPrepareHandle: proceeding Like command for usual track";

            auto musicRequestModeInfo = MakeMusicRequestModeInfoFromMusicArgs(applyArgs, scState, EAuthMethod::UserId, isClientBiometryModeApplyRequest);
            requests.push_back(NMusic::WrapMusicLikeDislikeRequest(NMusic::MakeMusicLikeDislikeTrackFromQueueRequest(
                request, ctx.RequestMeta, request.ClientInfo(),
                ctx.Ctx.Logger(), musicRequestModeInfo.RequesterUserId,
                /* isLikeCommand= */ true, enableCrossDc, musicRequestModeInfo)));

            if (mq.IsRadio()) {
                requests.push_back({
                    PrepareLikeDislikeRadioFeedbackProxyRequest(mq, ctx.RequestMeta, request.ClientInfo(),
                                                                ctx.Ctx.Logger(), request, scState, /* isLike= */ true,
                                                                enableCrossDc),
                    TString(NMusic::MUSIC_RADIO_FEEDBACK_LIKE_REQUEST_ITEM)
                });
            }
        }

        result = requests;
    } else {
        ythrow yexception() << "Unexpected playerCommand=" << TMusicArguments_EPlayerCommand_Name(playerCommand);
    }

    struct {
        TScenarioHandleContext& Ctx;
        const TMusicArguments& ApplyArgs;

        // make proxy request to music back
        void operator()(const THttpProxyRequestItemPairs& result) {
            Ctx.ServiceCtx.AddProtobufItem(ApplyArgs, MUSIC_ARGUMENTS_ITEM);
            for (const auto& [proxyRequest, item] : result) {
                AddMusicProxyRequest(Ctx, proxyRequest, TStringBuf(item));
            }
        }

        // return commit success immediately if request is not needed
        void operator()(const NScenarios::TScenarioCommitResponse& response) {
            Ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        }
    } visitor{ctx, applyArgs};

    std::visit(visitor, result);
}

} // namespace NAlice::NHollywood::NMusic
