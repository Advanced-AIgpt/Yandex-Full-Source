#include "show_video_settings.h"
#include "utils.h"

#include <alice/bass/forms/directives.h>
#include <alice/library/response/defs.h>

#include <util/generic/ymath.h>

namespace NBASS::NVideo {

IContinuation::TPtr ShowVideoSettings(TContext& ctx) {
    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_SHOW_VIDEO_SETTINGS)) {
        const auto errorMsg = TString::Join(
            "Experimental flag \"", EXPERIMENTAL_FLAG_DISABLE_SHOW_VIDEO_SETTINGS, "\" is enabled");
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
        return TCompletedContinuation::Make(ctx, AddAttention(ctx, ATTENTION_VIDEO_SHOW_VIDEO_SETTINGS_IS_NOT_SUPPORTED));
    }

    using TState = NBassApi::TVideoCurrentlyPlaying<TSchemeTraits>::TConst;
    TState state(ctx.Meta().DeviceState().Video().CurrentlyPlaying().GetRawValue());
    if (state.Item().AudioStreams().Empty() && state.Item().Subtitles().Empty()) {
        ctx.AddAttention(ATTENTION_VIDEO_IRRELEVANT_PROVIDER);
        return TCompletedContinuation::Make(ctx);
    }

    AddShowVideoSettingsCommandAndShouldListen(ctx);

    return TCompletedContinuation::Make(ctx);
}

void AddShowVideoSettingsCommandAndShouldListen(TContext& ctx) {
    NSc::TValue command;
    command[NAlice::NResponse::LISTENING_IS_POSSIBLE].SetBool(true);
    ctx.AddCommand<TShowVideoSettingsDirective>(
        NAlice::NVideoCommon::COMMAND_SHOW_VIDEO_SETTINGS,
        command);
}

} // namespace NBASS::NVideo
