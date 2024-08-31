#include "player_command.h"

#include "utils.h"

#include <alice/bass/forms/directives.h>

#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS::NVideo {

TResultValue RewindVideo(TContext& ctx, NPlayerCommand::TPlayerRewindCommand& command) {
    if (GetCurrentScreen(ctx) != EScreenId::VideoPlayer) {
        return TResultValue();
    }

    using TState = NBassApi::TVideoCurrentlyPlaying<TSchemeTraits>::TConst;
    TState state(ctx.Meta().DeviceState().Video().CurrentlyPlaying().GetRawValue());

    double current = state.Progress().Played();
    double duration = state.Progress().Duration();
    double amount = command->Amount();

    TMaybe<NPlayerCommand::ERewindType> type = ParseEnumMaybe<NPlayerCommand::ERewindType>(command->Type());
    if (!type) {
        TString err = TStringBuilder() << "Unsupported rewind type: " << command->Type();
        LOG(ERR) << err << Endl;
        return TError(TError::EType::VIDEOERROR, err);
    }

    double newPosition = 0;
    switch (*type) {
    case NPlayerCommand::ERewindType::Absolute:
    case NPlayerCommand::ERewindType::None:
        newPosition = amount;
        break;
    case NPlayerCommand::ERewindType::Backward:
        newPosition = current - amount;
        break;

    case NPlayerCommand::ERewindType::Forward:
        newPosition = current + amount;
        break;
    }
    if (newPosition < 0) {
        if (IsSlotEmpty(ctx.GetSlot(TStringBuf("time")))) {
            command->Type() = ToString(NPlayerCommand::ERewindType::Absolute);
            command->Amount() = 0;
        } else {
            return AddAttention(ctx, ATTENTION_REWIND_BEFORE_BEGIN);
        }
    }
    if (newPosition >= duration) {
        return AddAttention(ctx, ATTENTION_REWIND_AFTER_END);
    }
    ctx.AddCommand<TVideoPlayerRewindDirective>(NPlayerCommand::PLAYER_REWIND, command.Value());
    return TResultValue();
}

} // namespace NBASS::NVideo
