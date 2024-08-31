#include "onboarding.h"

#include <alice/hollywood/library/response/schedule_action.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/frame.pb.h>

#include <alice/library/experiments/flags.h>

#include <util/string/cast.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr ui32 MUSIC_ONBOARDING_DEFAULT_TRACK_REASK_COUNT = 2;
constexpr ui32 MUSIC_ONBOARDING_MAX_TRACK_REASK_DELAY_SECONDS = 90;
constexpr ui32 MUSIC_ONBOARDING_MIN_TRACK_REASK_DELAY_SECONDS = 30;

} // namespace

ui32 GetTracksReaskCount(const TScenarioBaseRequestWrapper& request) {
    return request.LoadValueFromExpPrefix<ui32>(
        EXP_HW_MUSIC_ONBOARDING_TRACKS_REASK_COUNT_PREFIX,
        MUSIC_ONBOARDING_DEFAULT_TRACK_REASK_COUNT
    );
}

ui32 GetTracksReaskDelaySeconds(const TScenarioBaseRequestWrapper& request) {
    return GetTracksReaskDelaySeconds(request, MUSIC_ONBOARDING_MAX_TRACK_REASK_DELAY_SECONDS);
}

ui32 GetTracksReaskDelaySeconds(const TScenarioBaseRequestWrapper& request, const ui32 defaultDelaySeconds) {
    const auto clampedDefaultDelay = std::clamp(
        defaultDelaySeconds,
        MUSIC_ONBOARDING_MIN_TRACK_REASK_DELAY_SECONDS,
        MUSIC_ONBOARDING_MAX_TRACK_REASK_DELAY_SECONDS
    );

    return request.LoadValueFromExpPrefix<ui32>(
        EXP_HW_MUSIC_ONBOARDING_TRACKS_REASK_DELAY_SECONDS_PREFIX,
        clampedDefaultDelay
    );
}

NScenarios::TServerDirective MakeTracksMidrollDirective(const TScenarioBaseRequestWrapper& request, const TStringBuf puid, const ui32 trackIndex) {
    return MakeTracksMidrollDirective(
        request,
        puid, 
        trackIndex,
        GetTracksReaskDelaySeconds(request)
    );
}

NScenarios::TServerDirective MakeTracksMidrollDirective(
    const TScenarioBaseRequestWrapper& request,
    TStringBuf puid,
    ui32 trackIndex,
    ui32 reaskDelaySeconds
) {
    TTypedSemanticFrame frame;
    frame.MutableMusicOnboardingTracksReaskSemanticFrame()->MutableTrackIndex()->SetNumValue(trackIndex);

    const auto actionId = TString::Join("music_onboarding_midroll_", ToString(trackIndex));

    auto builder = TScheduleActionDirectiveBuilder()
        .SetIds(actionId, request.BaseRequestProto().GetDeviceState().GetDeviceId(), puid)
        .SetTypedSemanticFrame(std::move(frame))
        .SetAnalytics("music_onboarding")
        .SetStartAtTimestampMs(request.ServerTimeMs() + reaskDelaySeconds * 1000)
        .SetSendOncePolicy();

    return std::move(builder).Build();
}

} // namespace NAlice::NHollywood::NMusic
