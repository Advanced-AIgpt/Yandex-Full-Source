#include "skip_fragment.h"
#include "utils.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/libs/video_common/parsers/video_item.h>
#include <alice/library/analytics/common/product_scenarios.h>

#include <util/generic/ymath.h>

namespace NBASS::NVideo {

IContinuation::TPtr SkipFragment(TContext& ctx) {
    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_SKIP_VIDEO_FRAGMENT)) {
        const auto errorMsg = TString::Join(
            "Experimental flag \"", EXPERIMENTAL_FLAG_DISABLE_SKIP_VIDEO_FRAGMENT, "\" is enabled");
        TResultValue error = TError(TError::EType::PROTOCOL_IRRELEVANT, errorMsg);
        return TCompletedContinuation::Make(ctx, error);
    }

    if (GetCurrentScreen(ctx) != EScreenId::VideoPlayer && !ctx.ClientFeatures().IsLegatus()) {
        TResultValue error = TError(TError::EType::PROTOCOL_IRRELEVANT, "Screen is not supported");
        return TCompletedContinuation::Make(ctx, error);
    }

    AddAnalyticsInfoFromVideoCommand(ctx);

    if (ctx.ClientFeatures().IsLegatus()) {
        return TCompletedContinuation::Make(ctx, AddAttention(ctx, ATTENTION_VIDEO_SKIP_FRAGMENT_IS_NOT_SUPPORTED));
    }

    using TState = NBassApi::TVideoCurrentlyPlaying<TSchemeTraits>::TConst;
    TState state(ctx.Meta().DeviceState().Video().CurrentlyPlaying().GetRawValue());
    double current = state.Progress().Played();
    double duration = state.Progress().Duration();

    const auto& skippableFragments = state.Item().SkippableFragments();
    const auto& skippableFragmentsDepr = state.Item().SkippableFragmentsDepr();

    NPlayerCommand::TPlayerRewindCommand command;
    command->Type() = "Absolute";

    if (!skippableFragments.IsNull()) {
        for (const auto& fragment : skippableFragments) {
            const double fragmentEndTime = fragment.EndTime();
            if (IsPositionInsideTheFragment(current,
                    fragment.StartTime(),
                    fragmentEndTime))
            {

                if (fragmentEndTime >= duration - NAlice::NVideoCommon::SKIP_LAST_FRAGMENT_MAX_ERROR) {
                    return PreparePlayNextVideo(ctx);
                }

                command->Amount() = fragmentEndTime;
                ctx.AddCommand<TVideoPlayerRewindDirective>(NPlayerCommand::PLAYER_REWIND, command.Value());
                return TCompletedContinuation::Make(ctx);
            }
        }
    } else if (!skippableFragmentsDepr.IsNull()) {
        if (IsPositionInsideTheFragment(current,
                skippableFragmentsDepr.IntroStart(),
                skippableFragmentsDepr.IntroEnd()))
        {
            command->Amount() = skippableFragmentsDepr.IntroEnd();
            ctx.AddCommand<TVideoPlayerRewindDirective>(NPlayerCommand::PLAYER_REWIND, command.Value());
            return TCompletedContinuation::Make(ctx);
        } else if (skippableFragmentsDepr.CreditsStart() <= current) {
            return PreparePlayNextVideo(ctx);
        }
    }

    LOG(INFO) << "Current position " << ToString(current) << " is outside the skippable fragments" << Endl;

    // If (fragment) slot is presented then SkipFragment scenario will be applied
    if (const auto* fragment = ctx.GetSlot("fragment"); IsSlotEmpty(fragment)) {
        const TString errorMsg = "No slot for SkipFragment scenario, probably NextTrack scenario will be applied";
        TResultValue error = TError(TError::EType::PROTOCOL_IRRELEVANT, errorMsg);
        return TCompletedContinuation::Make(ctx, error);
    }

    return TCompletedContinuation::Make(ctx, AddAttention(ctx, ATTENTION_VIDEO_NOT_SKIPPABLE_FRAGMENT));
}

bool IsPositionInsideTheFragment(
    double position,
    double start,
    double end)
{
    return position >= start - NAlice::NVideoCommon::SKIP_FRAGMENT_MAX_TIME_DELAY &&
           position <= end - NAlice::NVideoCommon::SKIP_FRAGMENT_MAX_TIME_DELAY;
}

} // namespace NBASS::NVideo
