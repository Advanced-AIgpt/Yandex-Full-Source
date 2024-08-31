#include "video_how_long.h"
#include "utils.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/tools/video/kinopoisk_svod_downloader/kp_genres.h>
#include <alice/library/response/defs.h>

#include <util/generic/ymath.h>

namespace NBASS::NVideo {

IContinuation::TPtr VideoHowLong(TContext& ctx) {
    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_VIDEO_HOW_LONG)) {
        const auto errorMsg = TString::Join(
            "Experimental flag \"", EXPERIMENTAL_FLAG_DISABLE_VIDEO_HOW_LONG, "\" is not enabled");
        const auto error = TError{TError::EType::PROTOCOL_IRRELEVANT, errorMsg};
        return TCompletedContinuation::Make(ctx, error);
    }

    if (GetCurrentScreen(ctx) != EScreenId::VideoPlayer && !ctx.ClientFeatures().IsLegatus()) {
        const TStringBuf errorMsg = "Screen is not supported";
        const auto error = TError{TError::EType::PROTOCOL_IRRELEVANT, errorMsg};
        return TCompletedContinuation::Make(ctx, error);
    }

    AddAnalyticsInfoFromVideoCommand(ctx);

    if (ctx.ClientFeatures().IsLegatus()) {
        return TCompletedContinuation::Make(ctx, AddAttention(ctx, ATTENTION_VIDEO_HOW_LONG_IS_NOT_SUPPORTED));
    }

    using TState = NBassApi::TVideoCurrentlyPlaying<TSchemeTraits>::TConst;
    TState state(ctx.Meta().DeviceState().Video().CurrentlyPlaying().GetRawValue());

    if (state.Item().Type() == ToString(EContentType::TvStream)) {
        const TStringBuf errorMsg = "TvStream video type is not supported in this scenario";
        const auto error = TError{TError::EType::PROTOCOL_IRRELEVANT, errorMsg};
        return TCompletedContinuation::Make(ctx, error);
    }

    if (!state.Progress().HasDuration()) {
        const TStringBuf errorMsg = "Duration type is not defined";
        const auto error = TError{TError::EType::PROTOCOL_IRRELEVANT, errorMsg};
        return TCompletedContinuation::Make(ctx, error);
    }

    NSc::TValue data;
    ui64 duration = static_cast<ui64>(state.Progress().Duration());
    ui64 played = static_cast<ui64>(state.Progress().Played());

    const auto& skippableFragments = state.Item().SkippableFragments();
    // Taking duration till the credits, if possible
    for (const auto& fragment : skippableFragments) {
        if (duration - NAlice::NVideoCommon::SKIP_FRAGMENT_MAX_TIME_DELAY <= fragment.EndTime() &&
            played < fragment.StartTime())
        {
            duration = static_cast<ui64>(fragment.StartTime());
            data["hours"] = (duration - played) / 3600;
            data["minutes"] = (duration - played) / 60 % 60;
            ctx.AddAttention("has_credits", data);
            return TCompletedContinuation::Make(ctx);
        }
    }

    if (duration < played) {
        duration = played;
    }

    data["hours"] = (duration - played) / 3600;
    data["minutes"] = (duration - played) / 60 % 60;

    if (state.Item().Genre() == "мультфильм") {
        ctx.AddAttention("cartoon", data);
    } else if (state.Item().Type() == ToString(EContentType::TvShowEpisode)) {
        ctx.AddAttention("tv_show_episode", data);
    } else if (state.Item().Type() == ToString(EContentType::Movie)) {
        ctx.AddAttention("movie", data);
    } else if (state.Item().Type() == ToString(EContentType::Video)) {
        ctx.AddAttention("video", data);
    } else {
        ctx.AddAttention("unknown_type", data);
    }

    return TCompletedContinuation::Make(ctx);
}

} // namespace NBASS::NVideo
