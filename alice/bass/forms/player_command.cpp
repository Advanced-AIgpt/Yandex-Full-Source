#include "player_command.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/forms/player_command/defs.h>
#include <alice/bass/forms/player_command/player_command.sc.h>
#include <alice/bass/forms/video/player_command.h>
#include <alice/bass/forms/video/defs.h>
#include <alice/bass/forms/video/utils.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

namespace {

const TSet<TStringBuf> UNSUPPORTED_IN_RADIO_PLAYER_COMMANDS = {
    TStringBuf("player_dislike"),
    TStringBuf("player_like"),
    TStringBuf("player_shuffle"),
    TStringBuf("player_repeat"),
    TStringBuf("player_replay"),
    NPlayerCommand::PLAYER_REWIND,
};

const TSet<TStringBuf> UNSUPPORTED_IN_BLUETOOTH_PLAYER_COMMANDS = {
    TStringBuf("player_dislike"),
    TStringBuf("player_like"),
    TStringBuf("player_shuffle"),
    TStringBuf("player_repeat"),
    TStringBuf("player_replay"),
    NPlayerCommand::PLAYER_REWIND,
};

NSc::TValue ConstructErrorData(TStringBuf command, TStringBuf code) {
    NSc::TValue errorData;
    errorData["command"].SetString(command);
    errorData["code"].SetString(code);
    return errorData;
}

} // namespace

namespace NPlayerCommand {

bool AssertPlayerCommandIsSupported(TStringBuf command, TContext& ctx) {
    NSc::TValue lastWatchedVideo;
    auto player = SelectPlayer(ctx, &lastWatchedVideo);
    if (player == RADIO_PLAYER && UNSUPPORTED_IN_RADIO_PLAYER_COMMANDS.contains(command)) {
        NSc::TValue errorData = ConstructErrorData(command, "radio_unsupported");
        const auto& currentlyPlaying = ctx.Meta().DeviceState().Radio().CurrentlyPlaying();
        errorData["radio"] = currentlyPlaying.IsNull() ? NSc::Null() : *currentlyPlaying.GetRawValue();
        ctx.AddErrorBlock(TError{TError::EType::PLAYERERROR}, errorData);
        ctx.AddStopListeningBlock();
        return false;
    }
    if (player == BLUETOOTH_PLAYER && (UNSUPPORTED_IN_BLUETOOTH_PLAYER_COMMANDS.contains(command) ||
        !ctx.ClientFeatures().SupportsBluetoothPlayer()))
    {
        NSc::TValue errorData = ConstructErrorData(command, "bluetooth_unsupported");
        ctx.AddErrorBlock(TError{TError::EType::PLAYERERROR}, errorData);
        ctx.AddStopListeningBlock();
        return false;
    }
    return true;
}

void SetPlayerCommandProductScenario(TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::PLAYER_COMMANDS);
}

} // namespace NPlayerCommand

using namespace NPlayerCommand;

void TPlayerSimpleCommandHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TPlayerSimpleCommandHandler>();
    };
    handlers->emplace(TStringBuf("personal_assistant.scenarios.player_pause"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.player_order"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.player_shuffle"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.player_repeat"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.player_replay"), handler);
    // old forms (for compatibility)
    handlers->emplace(TStringBuf("personal_assistant.scenarios.music_pause"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.music_continue"), handler);
}

void TPlayerAuthorizedCommandHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TPlayerAuthorizedCommandHandler>(MakeHolder<TBlackBoxAPI>(), MakeHolder<TDataSyncAPI>());
    };
    handlers->emplace(TStringBuf("personal_assistant.scenarios.player_like"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.player_dislike"), handler);
}

void TPlayerContinueCommandHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TPlayerContinueCommandHandler>();
    };
    handlers->emplace(TStringBuf("personal_assistant.scenarios.player_continue"), handler);
}

void TPlayerNextPrevCommandHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormAndContinuableHandler<TPlayerNextPrevCommandHandler>(
                TStringBuf("personal_assistant.scenarios.player_next_track"));
    handlers->RegisterFormAndContinuableHandler<TPlayerNextPrevCommandHandler>(
                TStringBuf("personal_assistant.scenarios.player_previous_track"));
}

void TPlayerRewindCommandHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TPlayerRewindCommandHandler>();
    };
    handlers->emplace(TStringBuf("personal_assistant.scenarios.player_rewind"), handler);
}


} // namespace NBASS
