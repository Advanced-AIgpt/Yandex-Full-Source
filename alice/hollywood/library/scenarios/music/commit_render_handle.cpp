#include "commit_render_handle.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/like_dislike_handlers.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/report_handlers.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood::NMusic {

void TCommitRenderHandle::Do(TScenarioHandleContext& ctx) const {
    const auto applyArgs = GetMaybeOnlyProto<TMusicArguments>(ctx.ServiceCtx, MUSIC_ARGUMENTS_ITEM);
    if (!applyArgs) {
        return;
    }
    const auto& playerCommand = applyArgs->GetPlayerCommand();
    auto& logger = ctx.Ctx.Logger();

    if (applyArgs->GetExecutionFlowType() == TMusicArguments_EExecutionFlowType_ComplexLikeDislike) {
        Y_ENSURE(applyArgs->HasComplexLikeDislikeRequest());
        const auto& likeDislike = applyArgs->GetComplexLikeDislikeRequest();

        LOG_INFO(logger) << "CommitRenderHandle: got usual feedback response for Complex Like/Dislike command";
        auto rawResponse = GetRawHttpResponse(ctx, MUSIC_LIKE_RESPONSE_ITEM);
        if (likeDislike.HasArtistTarget()) {
            MusicLikeDislikeArtistEnsureResponse(logger, JsonFromString(rawResponse));
        } else if (likeDislike.HasTrackTarget()) {
            MusicLikeDislikeTrackEnsureResponse(logger, JsonFromString(rawResponse));
        } else if (likeDislike.HasAlbumTarget()) {
            MusicLikeDislikeAlbumEnsureResponse(logger, JsonFromString(rawResponse));
        } else if (likeDislike.HasGenreTarget()) {
            MusicLikeDislikeGenreEnsureResponse(logger, JsonFromString(rawResponse));
        } else {
            ythrow yexception() << "No target in Complex Like/Dislike request";
        }
    } else if (playerCommand == TMusicArguments_EPlayerCommand_None) {
        if (auto rawGenerativeFeedbackResponse = GetRawHttpResponseMaybe(ctx, MUSIC_GENERATIVE_FEEDBACK_RESPONSE_ITEM)) {
            auto generativeFeedbackResponse = JsonFromString(*rawGenerativeFeedbackResponse);
            Y_ENSURE(generativeFeedbackResponse["result"].Has("reload_stream") &&
                         !generativeFeedbackResponse["result"]["reload_stream"].GetBooleanSafe(),
                     "Response from generative feedback is NOT OK");
            LOG_INFO(logger) << "Response from generative feedback is OK";
        }

        if (auto rawShotsFeedbackResponse = GetRawHttpResponseMaybe(ctx, MUSIC_SHOTS_FEEDBACK_RESPONSE_ITEM)) {
            auto shotsFeedbackResponse = JsonFromString(*rawShotsFeedbackResponse);
            Y_ENSURE(shotsFeedbackResponse["result"].GetStringSafe() == "ok", "Response from shots feedback is NOT OK");
            LOG_INFO(logger) << "Response from shots feedback is OK";
        }
    } else if (playerCommand == TMusicArguments_EPlayerCommand_Like) {

        if (auto rawGenerativeFeedbackResponse = GetRawHttpResponseMaybe(ctx, MUSIC_GENERATIVE_FEEDBACK_RESPONSE_ITEM);
            rawGenerativeFeedbackResponse.Defined())
        {
            LOG_INFO(logger) << "CommitRenderHandle: got generative feedback response for player command Like";
            auto generativeFeedbackResponse = JsonFromString(rawGenerativeFeedbackResponse.GetRef());
            Y_ENSURE(generativeFeedbackResponse["result"].Has("reload_stream") &&
                         !generativeFeedbackResponse["result"]["reload_stream"].GetBooleanSafe(),
                     "Response from generative feedback is NOT OK");
            LOG_INFO(logger) << "Response from generative feedback is OK";
        } else if (auto rawShotsFeedbackResponse = GetRawHttpResponseMaybe(ctx, MUSIC_SHOTS_FEEDBACK_RESPONSE_ITEM)) {
            LOG_INFO(logger) << "CommitRenderHandle: got shot feedback response for player command Like";
            auto shotsFeedbackResponse = JsonFromString(*rawShotsFeedbackResponse);
            LOG_INFO(logger) << "Shot Like feedback response is " << shotsFeedbackResponse;
            Y_ENSURE(shotsFeedbackResponse["result"].GetStringSafe() == "ok", "Response from shots Like feedback is NOT OK");
            LOG_INFO(logger) << "Response from shots Like feedback is OK";
        } else {
            LOG_INFO(logger) << "CommitRenderHandle: got usual feedback response for player command Like";
            auto rawResponse = GetRawHttpResponse(ctx, MUSIC_LIKE_RESPONSE_ITEM);
            MusicLikeDislikeTrackEnsureResponse(logger, JsonFromString(rawResponse));

            auto rawRadioFeedbackResponse = GetRawHttpResponseMaybe(ctx, MUSIC_RADIO_FEEDBACK_RESPONSE_ITEM);
            if (rawRadioFeedbackResponse) {
                LOG_INFO(logger) << "CommitRenderHandle: got succeded radio feedback response for player command Like";
            }
        }
    } else {
        ythrow yexception() << "Unsupported playerCommand=" << TMusicArguments_EPlayerCommand_Name(playerCommand)
                            << " in scenario commit stage";
    }

    auto resp = MakeCommitSuccessResponse();
    ctx.ServiceCtx.AddProtobufItem(*resp, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood::NMusic
