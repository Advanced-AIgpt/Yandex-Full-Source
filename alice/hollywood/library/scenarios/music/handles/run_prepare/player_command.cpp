#include "impl.h"

#include <alice/hollywood/library/scenarios/music/audio_player_commands.h>
#include <alice/hollywood/library/scenarios/music/music_player_commands.h>
#include <alice/hollywood/library/scenarios/music/commands.h>

namespace NAlice::NHollywood::NMusic::NImpl {

TMaybe<TRunPrepareHandleImpl::TResult> TRunPrepareHandleImpl::HandlePlayerCommand() {
    const auto playerCommand = FindPlayerCommand(Request_);
    const bool isThinClient = Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT) ||
                              Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_GENERATIVE) ||
                              Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_FM_RADIO);

    // if there is no player command, clear ProductScenarioName,
    // otherwise we will use the old ProductScenarioName
    if (ScenarioState_ && playerCommand == TMusicArguments_EPlayerCommand_None && isThinClient) {
        ScenarioState_->ClearProductScenarioName();
    }

    // there is no player commands
    if (playerCommand == TMusicArguments_EPlayerCommand_None) {
        return Nothing();
    }

    const auto playerFrame = FindPlayerFrame(Request_);
    Y_ENSURE(playerFrame); // it is not "nullptr", because playerCommand is not "None"

    if (Request_.Interfaces().GetHasAudioClient() && isThinClient &&
        IsAudioPlayerVsMusicAndBluetoothTheLatest(Request_.BaseRequestProto().GetDeviceState()))
    {
        // the thin audio player is the newest, handle it
        LOG_INFO(Logger_) << "Handling audio player command";
        return *HandleThinClientPlayerCommand(Ctx_, Request_, playerCommand, Nlg_, playerFrame->GetName(), ScenarioState_);
    } else if (const auto musicPlayerFrame = GetMusicPlayerFrame(Request_); musicPlayerFrame.Defined()) {
        // the legacy music player is the newest, handle it
        LOG_INFO(Logger_) << "Handling music player command";
        return HandleMusicPlayerCommand(Logger_, Request_, Meta_, Nlg_, *musicPlayerFrame, AppHostParams_);
    } else {
        LOG_INFO(Logger_) << "Could not handle player command...";
    }

    return Nothing();
}

} // namespace NAlice::NHollywood::NMusic::NImpl
