#include "continue_thin_client_render_handle.h"

#include <alice/hollywood/library/bass_adapter/bass_stats.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/shots.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/result_renders.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

namespace NAlice::NHollywood::NMusic {

void TContinueThinClientRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};
    auto& logger = ctx.Ctx.Logger();
    auto mCtx = GetMusicContext(ctx.ServiceCtx);
    auto& scState = *mCtx.MutableScenarioState();
    TryInitPlaybackContextBiometryOptions(logger, scState);

    TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    std::unique_ptr<NAlice::NScenarios::TScenarioApplyResponse> resp;

    if (mCtx.HasAccountStatus() && mCtx.GetAccountStatus().GetUid().Empty()) {
        resp = NImpl::ApplyThinClientRenderUnauthorizedHandleImpl(ctx, mCtx, request, nlgWrapper);
        LOG_INFO(logger) << "User is not authorized";
        ctx.ServiceCtx.AddProtobufItem(*resp, RESPONSE_ITEM);
        return;
    }

    TResponseBodyBuilder::TRenderData renderData;
    Y_DEFER {
        for (auto const& [cardId, cardData] : renderData) {
            LOG_INFO(ctx.Ctx.Logger()) << "Adding render_data to context";
            ctx.ServiceCtx.AddProtobufItem(cardData, RENDER_DATA_ITEM);
        }
    };

    TMusicQueueWrapper mq(logger, *scState.MutableQueue());

    if (mCtx.HasAccountStatus() && !mCtx.GetAccountStatus().GetHasMusicSubscription())
    {
        // We should clear any Queue state that could came with the request
        scState.ClearQueue();
        ClearBiometryOptions(scState);

        if (mCtx.GetAccountStatus().HasPromo()) {
            LOG_INFO(logger) << "User has no subscription but has promo to activate";
            resp = NImpl::ApplyThinClientRenderPromoHandleImpl(ctx, mCtx, request, nlgWrapper);
        } else {
            LOG_INFO(logger) << "User has no subscription";
            resp = NImpl::ApplyThinClientRenderNonPremiumHandleImpl(ctx, mCtx, request, nlgWrapper);
        }

        ctx.ServiceCtx.AddProtobufItem(*resp, RESPONSE_ITEM);
        return;
    }

    if (!mCtx.HasAccountStatus() || mq.QueueSize() == 0 || mCtx.GetContentStatus().GetErrorVer2() != NoError) {
        const auto isError = mCtx.GetContentStatus().GetErrorVer2() != NoError;
        LOG_WARN(logger) << "Music context have "
                         << (!mCtx.HasAccountStatus() ? "no account status" : isError ? "error" : "empty queue");
        resp = NImpl::ApplyThinClientRenderErrorHandleImpl(ctx, request, nlgWrapper, mCtx);
    } else {
        if (auto maybeResp = GetRawHttpResponseMaybe(ctx, MUSIC_SHOTS_HTTP_RESPONSE_ITEM)) {
            LOG_INFO(logger) << "Process shots resp:" << *maybeResp;
            ProcessShotsResponse(ctx.Ctx.Logger(), *maybeResp, mq);
        }
        const TAvatarColorsRequestHelper<ERequestPhase::After> avatarColors(ctx);
        const TNeighboringTracksRequestHelper<ERequestPhase::After> neighboringTracks(ctx);
        const TLikesTracksRequestHelper<ERequestPhase::After> likesTracks(ctx);
        const TDislikesTracksRequestHelper<ERequestPhase::After> dislikesTracks(ctx);
        const TShowViewBuilderSources sources{
            .AvatarColors = &avatarColors,
            .NeighboringTracks = &neighboringTracks,
            .LikesTracks = &likesTracks,
            .DislikesTracks = &dislikesTracks,
        };

        const std::shared_ptr<const TMusicFastData> fastData = ctx.Ctx.GlobalContext().FastData().GetFastData<TMusicFastData>();
        const auto response = GetFirstOfRawHttpResponses(ctx, MUSIC_RESPONSE_ITEMS);
        const auto shots = ctx.Ctx.GlobalContext().FastData().GetFastData<TMusicShotsFastData>();
        resp = NImpl::ContinueThinClientRenderHandleImpl(ctx, mCtx, response, request,
                                                         nlgWrapper, ctx.Rng, fastData.get(), renderData,
                                                         sources, *shots);
    }

    ctx.ServiceCtx.AddProtobufItem(*resp, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NMusic
