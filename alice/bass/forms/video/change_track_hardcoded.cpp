#include "change_track_hardcoded.h"
#include "utils.h"

namespace NBASS::NVideo {

IContinuation::TPtr ChangeTrackHardcoded(TContext& ctx) {
    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_CHANGE_TRACK)) {
        const auto errorMsg = TString::Join(
            "Experimental flag \"", EXPERIMENTAL_FLAG_DISABLE_CHANGE_TRACK, "\" is enabled");
        TResultValue error = TError(TError::EType::PROTOCOL_IRRELEVANT, errorMsg);
        return TCompletedContinuation::Make(ctx, error);
    }

    if (GetCurrentScreen(ctx) == EScreenId::VideoPlayer) {
        const TStringBuf errorMsg = "Intent ChangeTrack should have been selected";
        TResultValue error = TError(TError::EType::PROTOCOL_IRRELEVANT, errorMsg);
        return TCompletedContinuation::Make(ctx, error);
    }

    AddAnalyticsInfoFromVideoCommand(ctx);

    if (ctx.ClientFeatures().IsQuasar()) {
        ctx.AddAttention(ATTENTION_VIDEO_IRRELEVANT_SCREEN_FOR_CHANGE_TRACK);
    } else {
        ctx.AddAttention(ATTENTION_VIDEO_IRRELEVANT_CLIENT_FOR_CHANGE_TRACK);
    }

    return TCompletedContinuation::Make(ctx);
}

} // namespace NBASS::NVideo
