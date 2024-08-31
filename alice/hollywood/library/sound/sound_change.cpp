#include "sound_change.h"

#include <alice/hollywood/library/sound/sound_common.h>
#include <alice/hollywood/library/sound/sound_level_calculation.h>

#include <alice/megamind/protos/common/device_state.pb.h>

#include <alice/library/json/json.h>


namespace NAlice::NHollywood::NSound {

namespace {

constexpr i64 DEFAULT_VOLUME_STEPS_CHANGE_FOR_LOUDER_AND_QUITER = 2;

const TVector<TStringBuf> ALL_FRAMES {
    LOUDER_IN_CONTEXT_FRAME,
    QUITER_IN_CONTEXT_FRAME,
    SET_LEVEL_IN_CONTEXT_FRAME
};

void SetSoundLevel(TResponseBodyBuilder& bodyBuilder, const TFrame& frame, const TDeviceVolume& deviceSound,
                   NJson::TJsonValue directiveValue)
{
    i64 newSoundLevel = NSound::CalculateSoundLevelForSetLevel(frame, deviceSound);

    if (!deviceSound.IsSupported(newSoundLevel)) {
        // If user says "set volume on 11", probably he wants maximum volume
        // So we make volume maximal, but warn that Alice has [0;10] (or other) scale
        newSoundLevel = newSoundLevel > deviceSound.GetMax() ? deviceSound.GetMax() : deviceSound.GetMin();
    }

    directiveValue["new_level"] = newSoundLevel;
    bodyBuilder.AddClientActionDirective(TString{SOUND_SET_LEVEL_DIRECTIVE},
                                         TString{SOUND_SET_LEVEL_COMMAND}, directiveValue);
    bodyBuilder.AddClientActionDirective(TString{TTS_PLAY_PLACEHOLDER}, TString{SOUND_SET_LEVEL_DIRECTIVE});
}

void MakeSoundLouderOrQuiter(TResponseBodyBuilder& bodyBuilder, const TFrame& frame, const TDeviceVolume& deviceSound,
                             NJson::TJsonValue directiveValue, bool isLouder)
{
    const i64 newSoundLevel = NSound::CalculateSoundLevelForLouderOrQuiter(frame, deviceSound, isLouder, DEFAULT_VOLUME_STEPS_CHANGE_FOR_LOUDER_AND_QUITER).first;
    directiveValue["new_level"] = newSoundLevel;
    bodyBuilder.AddClientActionDirective(TString{SOUND_SET_LEVEL_DIRECTIVE},
                                         TString{SOUND_SET_LEVEL_COMMAND}, directiveValue);
    bodyBuilder.AddClientActionDirective(TString{TTS_PLAY_PLACEHOLDER}, TString{SOUND_SET_LEVEL_DIRECTIVE});
}

void FillSoundChangeAnalyticsInfo(TResponseBodyBuilder& bodyBuilder, TStringBuf action_id,
                                  TStringBuf action_name, const TString& action_description)
{
    auto& analyticsInfoBuilder = bodyBuilder.HasAnalyticsInfoBuilder() ? bodyBuilder.GetAnalyticsInfoBuilder() : bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfoBuilder.AddAction(TString(action_id), TString(action_name), action_description);
}

TMaybe<TString> ProcessSoundChange(const TScenarioBaseRequestWrapper& request, const TFrame& frame, IResponseBuilder& builder) {
    auto& bodyBuilder = builder.GetOrCreateResponseBodyBuilder(&frame);
    const auto& deviceState = request.BaseRequestProto().GetDeviceState();
    const i64 currentSoundLevel = deviceState.GetSoundLevel();

    if (!request.Interfaces().GetSupportsAnyPlayer() || currentSoundLevel == -1) { // TODO(HOLLYWOOD-182)
        return Nothing();
    }

    const TDeviceVolume deviceSound = TDeviceVolume::BuildFromState(deviceState);

    NJson::TJsonValue directiveValue;
    AddMultiroomSessionIdToDirectiveValue(directiveValue, deviceState);

    const auto& frameName = frame.Name();
    if (frameName == LOUDER_IN_CONTEXT_FRAME) {
        MakeSoundLouderOrQuiter(bodyBuilder, frame, deviceSound, std::move(directiveValue), /* isLouder = */ true);
        return TString(frameName);

    } else if (frameName == QUITER_IN_CONTEXT_FRAME) {
        MakeSoundLouderOrQuiter(bodyBuilder, frame, deviceSound, std::move(directiveValue), /* isLouder = */ false);
        return TString(frameName);

    } else if (frameName == SET_LEVEL_IN_CONTEXT_FRAME) {
        SetSoundLevel(bodyBuilder, frame, deviceSound, std::move(directiveValue));
        return TString(frameName);
    }

    return Nothing();
}

} // namespace

TMaybe<TString> RenderSoundChangeIfExists(TRTLogger& logger, const TScenarioBaseRequestWrapper& request,
                                             const TScenarioInputWrapper& requestInput, IResponseBuilder& builder)
{
    if (request.ClientInfo().IsNavigator() || request.ClientInfo().IsYaAuto()) {
        return Nothing();
    }
    const TMaybe<TFrame> frame = GetFrame(requestInput, ALL_FRAMES);
    if (!frame.Defined()) {
        LOG_INFO(logger) << "No such frame!";
        return Nothing();
    }

    TMaybe<TString> soundFrameName = ProcessSoundChange(request, *frame, builder);
    if (soundFrameName.Defined()) {
        LOG_INFO(logger) << "Got sound level change in request. Frame name: " << *soundFrameName;
    }
    return soundFrameName;
}

void FillAnalyticsInfoForSoundChangeIfExists(TMaybe<TString> soundFrameName, TResponseBodyBuilder& bodyBuilder) {
    if (!soundFrameName.Defined()) {
        return;
    }

    if (*soundFrameName == LOUDER_IN_CONTEXT_FRAME) {
        FillSoundChangeAnalyticsInfo(bodyBuilder,
                                     SOUND_LOUDER_IN_CONTEXT_ACTION_ID,
                                     SOUND_LOUDER_IN_CONTEXT_ACTION_NAME,
                                     "Make sound louder while processing another request.");

    } else if (*soundFrameName == QUITER_IN_CONTEXT_FRAME) {
        FillSoundChangeAnalyticsInfo(bodyBuilder,
                                     SOUND_QUITER_IN_CONTEXT_ACTION_ID,
                                     SOUND_QUITER_IN_CONTEXT_ACTION_NAME,
                                     "Make sound quiter while processing another request.");

    } else if (*soundFrameName == SET_LEVEL_IN_CONTEXT_FRAME) {
        FillSoundChangeAnalyticsInfo(bodyBuilder,
                                     SOUND_SET_LEVEL_IN_CONTEXT_ACTION_ID,
                                     SOUND_SET_LEVEL_IN_CONTEXT_ACTION_NAME,
                                     "Set sound level while processing another request.");
    }
}

} // namespace NAlice::NHollywood::NSound
