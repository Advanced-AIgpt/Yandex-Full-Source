#include "common.h"
#include "multiroom.h"

#include <alice/hollywood/library/multiroom/multiroom.h>

#include <alice/megamind/protos/common/device_state.pb.h>

namespace NAlice::NHollywood::NMultiroom {

namespace {

constexpr TStringBuf PLAYER_TYPE_SLOT_NAME = "player_type";
constexpr TStringBuf ROOM_SLOT_NAME = "room";

constexpr TStringBuf PLAYER_TYPE_MUSIC = "music";
constexpr TStringBuf ROOM_EVERYWHERE = "everywhere";
constexpr TStringBuf ROOM_ID_ALL = "__all__";

constexpr TStringBuf START_MULTIROOM_FRAME_NAME = "alice.multiroom.start_multiroom";

bool ProcessActivateMultiroomCommand(TRTLogger& logger, const TScenarioRunRequestWrapper& request,
                                     TResponseBodyBuilder& bodyBuilder, const TFrame& frame) {
    const auto logError = [&logger] (const TStringBuf msg) {
        LOG_INFO(logger) << "Can not process multiroom activation: " << msg;
        return false;
    };

    if (const auto playerTypeSlot = frame.FindSlot(PLAYER_TYPE_SLOT_NAME);
        playerTypeSlot && playerTypeSlot->Value.AsString() != PLAYER_TYPE_MUSIC)
    {
        return logError("player_type defined and != music");
    }

    const auto& deviceState = request.Proto().GetBaseRequest().GetDeviceState();
    const auto musicPlayerIsPlaying = deviceState.GetMusic().GetPlayer().HasPause() && !deviceState.GetMusic().GetPlayer().GetPause();
    const auto& audioPlayerState = deviceState.GetAudioPlayer().GetPlayerState();
    const auto audioPlayerIsPlaying = audioPlayerState == TDeviceState_TAudioPlayer_TPlayerState_Playing;
    if (!musicPlayerIsPlaying && !audioPlayerIsPlaying) {
        return logError("no currently playing music track");
    }

    NJson::TJsonValue directiveValue;
    if (FrameHasSomeLocationSlot(frame)) {
        AddRedirectToLocation(directiveValue, frame);
    } else if (const auto roomSlot = frame.FindSlot(ROOM_SLOT_NAME)) {
        const auto roomFromSlot = roomSlot->Value.AsString();
        directiveValue["room_id"] = roomFromSlot == ROOM_EVERYWHERE ? ROOM_ID_ALL : roomFromSlot;
    } else {
        return logError("Location slots and room slot are not defined");
    }

    directiveValue[LOCATION_INCLUDE_CURRENT_DEVICE_ID] = true;
    bodyBuilder.AddClientActionDirective(TString{START_MULTIROOM_DIRECTIVE},
                                         TString{START_MULTIROOM_DIRECTIVE}, directiveValue);
    return true;
}

} // namespace

TMaybe<TFrame> GetMultiroomFrame(const TMaybe<TFrame>& frame, const TScenarioInputWrapper& input) {
    if (frame.Defined() && frame->Name() == START_MULTIROOM_FRAME_NAME) {
        return frame;
    }
    if (input.FindSemanticFrame(START_MULTIROOM_FRAME_NAME)) {
        return input.CreateRequestFrame(START_MULTIROOM_FRAME_NAME);
    }
    return Nothing();
}

void ProcessMultiroomCommand(TFastCommandScenarioRunContext& fastCommandScenarioRunContext, const TFrame& frame) {
    const auto& request = fastCommandScenarioRunContext.Request;
    auto& logger = fastCommandScenarioRunContext.Logger;
    auto& builder = fastCommandScenarioRunContext.RunResponseBuilder;
    auto& bodyBuilder = builder.CreateResponseBodyBuilder(&frame);

    if (frame.Name() == START_MULTIROOM_FRAME_NAME &&
        ProcessActivateMultiroomCommand(logger, request, bodyBuilder, frame))
    {
        return;
    }

    TNlgData nlgData{logger, request};
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(SOUND_COMMON_NLG,
                                                   RENDER_SOUND_NOT_SUPPORTED,
                                                   /* buttons = */ {}, nlgData);
    builder.SetIrrelevant();
}

} // namespace NAlice::NHollywood::NMultiroom
