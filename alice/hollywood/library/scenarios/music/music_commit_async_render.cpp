#include "music_commit_async_render.h"

#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>

namespace NAlice::NHollywood::NMusic {

void TMusicCommitAsyncRenderHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();

    if (GetRawHttpResponseMaybe(ctx, MUSIC_RADIO_FEEDBACK_RESPONSE_ITEM)) {
        // response has code between 200 and 299, so it is OK (there is no "result" object)
        LOG_INFO(logger) << "Response from radio feedback is OK";
    }

    if (GetRawHttpResponseMaybe(ctx, MUSIC_RADIO_FEEDBACK_SKIP_RESPONSE_ITEM)) {
        // response has code between 200 and 299, so it is OK (there is no "result" object)
        LOG_INFO(logger) << "Response from radio feedback skip is OK";
    }

    if (auto rawPlayAudioResponse = GetRawHttpResponseMaybe(ctx, MUSIC_PLAYS_RESPONSE_ITEM); rawPlayAudioResponse) {
        auto playAudioResponse = JsonFromString(*rawPlayAudioResponse);
        Y_ENSURE(playAudioResponse["result"].GetStringSafe() == "ok", "Response from /plays is NOT OK");
        LOG_INFO(logger) << "Response from /plays is OK";
    }

    if (auto rawSaveProgressResponse = GetRawHttpResponseMaybe(ctx, MUSIC_SAVE_PROGRESS_RESPONSE_ITEM); rawSaveProgressResponse) {
        auto saveProgressResponse = JsonFromString(*rawSaveProgressResponse);
        Y_ENSURE(saveProgressResponse["result"].GetStringSafe() == "ok", "Response from /streams/progress/save-current is NOT OK");
        LOG_INFO(logger) << "Response from /streams/progress/save-current is OK";
    }

    if (GetRawHttpResponseMaybe(ctx, MUSIC_REMOVE_LIKE_RESPONSE_ITEM)) {
        // response has code between 200 and 299, so it is OK (there is no "result" object)
        LOG_INFO(logger) << "Response from remove like is OK";
    }
}

} // NAlice::NHollywood::NMusic
