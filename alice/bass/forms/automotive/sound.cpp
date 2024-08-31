#include "sound.h"
#include "url_build.h"
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/sound.h>

namespace  NBASS {
namespace  NAutomotive {

namespace {
const TStringBuf CLIENT_ACTION_VOLUME_UP = "volume_up";
const TStringBuf CLIENT_ACTION_VOLUME_DOWN = "volume_down";
const TStringBuf CLIENT_ACTION_MUTE = "mute";
const TStringBuf CLIENT_ACTION_UNMUTE = "unmute";
} // namespace

TResultValue HandleSound(TContext& ctx) {

    TString data;

    if (ctx.FormName() == NSound::LOUDER || ctx.FormName() == NSound::LOUDER_ELLIPSIS) {
        data = CLIENT_ACTION_VOLUME_UP;
    } else if (ctx.FormName() == NSound::QUITER || ctx.FormName() == NSound::QUITER_ELLIPSIS) {
        data = CLIENT_ACTION_VOLUME_DOWN;
    } else if (ctx.FormName() == NSound::MUTE) {
        data = CLIENT_ACTION_MUTE;
    } else if (ctx.FormName() == NSound::UNMUTE) {
        data = CLIENT_ACTION_UNMUTE;
    } else {
        ctx.AddErrorBlock(TError(TError::EType::NOTSUPPORTED));
        return TResultValue();
    }

    TDirectiveBuilder<TAutoSoundDirective> builder(TStringBuf("sound"));
    builder.InsertParam(ToString("action"), data);

    return builder.AddDirective(ctx);
}
} // NAutomotive
} // NBASS
