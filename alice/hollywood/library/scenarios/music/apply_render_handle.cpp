#include "apply_render_handle.h"

#include <alice/hollywood/library/bass_adapter/bass_stats.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/scenarios/music/analytics_info/analytics_info.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/consts.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/result_renders.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/shots.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/requests_helper/requests_helper.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/experiments/flags.h>

namespace NAlice::NHollywood::NMusic {

void TApplyRenderHandle::Do(TScenarioHandleContext& ctx) const {
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

    if (mCtx.HasAccountStatus() && !mCtx.GetAccountStatus().GetHasMusicSubscription()) {
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

    const auto hasError = mCtx.GetContentStatus().GetErrorVer2() != NoError;
    TMusicQueueWrapper mq(logger, *scState.MutableQueue());

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

    if (!mCtx.HasAccountStatus() || mq.QueueSize() == 0 || hasError) {
        LOG_WARN(logger) << "Music context have "
                         << (!mCtx.HasAccountStatus() ? "no account status" : hasError ? "error" : "empty queue");
        resp = NImpl::ApplyThinClientRenderErrorHandleImpl(ctx, request, nlgWrapper, mCtx);
    } else {
        if (HaveHttpResponse(ctx, MUSIC_GENERATIVE_FEEDBACK_RESPONSE_ITEM)) {
            const auto response = JsonFromString(GetRawHttpResponse(ctx, MUSIC_GENERATIVE_FEEDBACK_RESPONSE_ITEM));
            LOG_INFO(logger) << "Generative feedback response is " << response;
            if (mCtx.GetNeedGenerativeContinue()) {
                LOG_INFO(logger) << "Generative feedback response found (streamPlay)";
                Y_ENSURE(response["result"].Has("reload_stream"), "Response from generative feedback is NOT OK");
                if (request.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_GENERATIVE_FORCE_RELOAD_ON_STREAM_PLAY) ||
                    response["result"]["reload_stream"].GetBooleanSafe())
                {
                    LOG_INFO(logger) << "Reload stream is true";
                    resp = NImpl::ApplyThinClientRenderGenerativeHandleImpl(ctx, mCtx, request, nlgWrapper);
                } else {
                    LOG_INFO(logger) << "Reload stream is false";
                    const auto shots = ctx.Ctx.GlobalContext().FastData().GetFastData<TMusicShotsFastData>();
                    resp = NImpl::ApplyThinClientRenderEmptyResponseHandleImpl(ctx, mCtx, request, nlgWrapper, ctx.Rng,
                                                                               sources, *shots);
                }
            } else {
                LOG_INFO(logger) << "Generative feedback response found (dislike or skip)";
                Y_ENSURE(response["result"].Has("reload_stream") && response["result"]["reload_stream"].GetBooleanSafe(),
                         "Response from generative feedback is NOT OK");
                resp = NImpl::ApplyThinClientRenderGenerativeHandleImpl(ctx, mCtx, request, nlgWrapper);
            }
        } else {
            if (auto maybeResp = GetRawHttpResponseMaybe(ctx, MUSIC_SHOTS_HTTP_RESPONSE_ITEM)) {
                LOG_INFO(logger) << "Process shots resp:" << *maybeResp;
                ProcessShotsResponse(ctx.Ctx.Logger(), *maybeResp, mq);
            }

            const auto response = GetFirstOfRawHttpResponses(ctx, MUSIC_RESPONSE_ITEMS);

            if (HaveHttpResponse(ctx, MUSIC_SHOTS_FEEDBACK_RESPONSE_ITEM)) {
                LOG_INFO(logger) << "ApplyRenderHandle: got shot Dislike feedback response";
                const auto shotsFeedbackResponse = JsonFromString(GetRawHttpResponse(ctx, MUSIC_SHOTS_FEEDBACK_RESPONSE_ITEM));
                LOG_INFO(logger) << "Shot Dislike feedback response is " << shotsFeedbackResponse;
                Y_ENSURE(shotsFeedbackResponse["result"].GetStringSafe() == "ok", "Response from shots Dislike feedback is NOT OK");
                LOG_INFO(logger) << "Response from shots Dislike feedback is OK";
            }

            // TODO(jan-fazli): rename MUSIC_DISLIKE_REQUEST_ITEM, cause it is now also used for likes
            if (HaveHttpResponse(ctx, MUSIC_DISLIKE_RESPONSE_ITEM)) {
                const auto likeDislikeResponse = GetRawHttpResponse(ctx, MUSIC_DISLIKE_RESPONSE_ITEM);
                LOG_INFO(logger) << "likeDislikeResponse is " << likeDislikeResponse;
                Y_ENSURE(!likeDislikeResponse.empty()); // TODO(vitvlkv): validate likeDislikeResponse better
            }
            if (mCtx.GetNeedOnboardingRadioLikeDislike()) {
                const auto radioFeedbackResponse = GetRawHttpResponseMaybe(ctx, MUSIC_RADIO_FEEDBACK_LIKE_DISLIKE_RESPONSE_ITEM);
                if (!radioFeedbackResponse.Defined()) {
                    LOG_ERROR(logger) << "Response from onboarding radio feedback like dislike is NOT OK, probably a guest user";
                } else {
                    // response has code between 200 and 299, so it is OK
                    LOG_INFO(logger) << "Response from onboarding radio feedback like dislike is OK: " << radioFeedbackResponse.GetRef();
                }
            }
            const auto shots = ctx.Ctx.GlobalContext().FastData().GetFastData<TMusicShotsFastData>();
            resp = NImpl::ApplyThinClientRenderHandleImpl(ctx, mCtx, response, request, nlgWrapper, ctx.Rng, renderData,
                                                          sources, *shots);
        }
    }

    ctx.ServiceCtx.AddProtobufItem(*resp, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NMusic
