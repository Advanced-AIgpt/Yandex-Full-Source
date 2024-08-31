#include "device_state_features.h"
#include "current_screen_features.h"

#include <alice/library/restriction_level/protos/content_settings.pb.h>

#include <alice/megamind/library/scenarios/defs/names.h>

#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/device_helpers.h>

#include <kernel/alice/device_state_factors_info/factors_gen.h>
#include <kernel/factor_storage/factor_storage.h>

using namespace NAliceDeviceState;

namespace NAlice {

namespace {

const THashMap<TStringBuf, size_t> ACTIVATION_FACTOR_NAMES = {
    { "spotter", FI_ACTIVATED_BY_SPOTTER },
    { "okniks", FI_ACTIVATED_BY_BUTTON },
    { "auto_listening", FI_ACTIVATED_BY_AUTO_LISTENING },
    { "directive", FI_ACTIVATED_BY_DIRECTIVE },
    { "auto_activation", FI_ACTIVATED_BY_AUTO_ACTIVATION },
};

bool HasCurrentlyPlayingMusic(const TDeviceState& deviceState) {
    return deviceState.HasMusic() && deviceState.GetMusic().HasCurrentlyPlaying();
}

bool HasCurrentlyPlayingVideo(const TDeviceState& deviceState) {
    return deviceState.HasVideo() && deviceState.GetVideo().HasCurrentlyPlaying();
}

bool PlayingContentIsNotPaused(const TDeviceState& deviceState) {
    const bool video = HasCurrentlyPlayingVideo(deviceState) && !deviceState.GetVideo().GetCurrentlyPlaying().GetPaused();
    const bool music = HasCurrentlyPlayingMusic(deviceState) && !deviceState.GetMusic().GetPlayer().GetPause();
    return video || music;
}

bool IsAlarmBuzzing(const TDeviceState& deviceState) {
    return deviceState.HasAlarmState() && deviceState.GetAlarmState().GetCurrentlyPlaying();
}

bool IsTimerBuzzing(const TDeviceState& deviceState) {
    if (!deviceState.HasTimers()) {
        return false;
    }
    for (const auto& timer : deviceState.GetTimers().GetActiveTimers()) {
        if (timer.GetCurrentlyPlaying()) {
            return true;
        }
    }
    return false;
}

void FillContentSetting(const TDeviceState& deviceState, const TFactorView view) {
    const EContentSettings contentSetting = deviceState.GetDeviceConfig().GetContentSettings();

    view[FI_CONTENT_SETTING_MEDIUM] = contentSetting == EContentSettings::medium;
    view[FI_CONTENT_SETTING_CHILDREN] = contentSetting == EContentSettings::children;
    view[FI_CONTENT_SETTING_WITHOUT] = contentSetting == EContentSettings::without;
}

} // namespace

void FillDeviceStateFactors(const TSpeechKitRequest& request, TFactorStorage& storage) {
    const TDeviceState& deviceState = request.Proto().GetRequest().GetDeviceState();
    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_DEVICE_STATE);

    view[FI_HAS_CURRENTLY_PLAYING_MUSIC] = HasCurrentlyPlayingMusic(deviceState);
    view[FI_HAS_CURRENTLY_PLAYING_VIDEO] = HasCurrentlyPlayingVideo(deviceState);
    view[FI_PLAYING_CONTENT_IS_NOT_PAUSED] = PlayingContentIsNotPaused(deviceState);
    view[FI_IS_ALARM_BUZZING] = IsAlarmBuzzing(deviceState);
    view[FI_IS_TIMER_BUZZING] = IsTimerBuzzing(deviceState);
    view[FI_SOUND_MUTED] = deviceState.GetSoundLevel() == 0;
    view[FI_IS_TV_PLUGGED_IN] = deviceState.GetIsTvPluggedIn();

    FillContentSetting(deviceState, view);
    FillCurrentScreen(deviceState, view);

    const auto& activationType = request.Proto().GetRequest().GetActivationType();
    if (ACTIVATION_FACTOR_NAMES.contains(activationType)) {
        view[ACTIVATION_FACTOR_NAMES.at(activationType)] = true;
    }
}

void FillSessionFactors(const ISession* session, TFactorStorage& storage) {
    if (!session) {
        return;
    }
    const TFactorView view = storage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_DEVICE_STATE);
    const auto& scenarioName = session->GetPreviousScenarioName();

    if (scenarioName == MM_VIDEO_PROTOCOL_SCENARIO) {
        view[FI_PREVIOUS_SCENARIO_WAS_VIDEO] = true;
    } else if (scenarioName == HOLLYWOOD_MUSIC_SCENARIO) {
        view[FI_PREVIOUS_SCENARIO_WAS_MUSIC] = true;
    } else if (scenarioName == MM_VINS_SCENARIO || scenarioName == MM_PROTO_VINS_SCENARIO) {
        view[FI_PREVIOUS_SCENARIO_WAS_VINS] = true;
    } else if (scenarioName == MM_SEARCH_PROTOCOL_SCENARIO) {
        view[FI_PREVIOUS_SCENARIO_WAS_SEARCH] = true;
    } else if (scenarioName == PROTOCOL_GENERAL_CONVERSATION_SCENARIO) {
        view[FI_PREVIOUS_SCENARIO_WAS_GC] = true;
    } else {
        view[FI_PREVIOUS_SCENARIO_WAS_OTHER] = true;
    }
}

} // namespace NAlice
