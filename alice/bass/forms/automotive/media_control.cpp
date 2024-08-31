#include "media_control.h"
#include "url_build.h"
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/sound.h>

#include <alice/bass/libs/logging_v2/logger.h>
#include <util/string/cast.h>

namespace  NBASS {
namespace  NAutomotive {

namespace {
THashMap <TStringBuf, TStringBuf> SupportedCommandRenames = {
    { TStringBuf("player_pause"), TStringBuf("pause") },
    { TStringBuf("player_continue"), TStringBuf("play") },
    { TStringBuf("player_next_track"), TStringBuf("next") },
    { TStringBuf("player_previous_track"), TStringBuf("prev") },
};

const TStringBuf MEDIA_CONTROL = "media_control";
} // namespace

TResultValue HandleMediaControl(TContext& ctx, const TStringBuf& command) {
    if (SupportedCommandRenames.contains(command)) {
        TDirectiveBuilder<TAutoMediaControlDirective> builder(MEDIA_CONTROL);
        builder.InsertParam(ToString("action"), ToString(SupportedCommandRenames.at(command)));
        return builder.AddDirective(ctx);
    } else {
        ctx.AddErrorBlock(
            TError(TError::EType::NOTSUPPORTED, TStringBuf("unsupported_operation")),
            NSc::Null()
        );
        return TResultValue();
    }
}

TResultValue HandleMediaControlSource(TContext& ctx, const TStringBuf& command) {
    TDirectiveBuilder<TAutoMediaControlDirective> builder(MEDIA_CONTROL);
    builder.InsertParam(ToString("source"), ToString(command));
    return builder.AddDirective(ctx);
}

} // NAutomotive
} // NBASS
