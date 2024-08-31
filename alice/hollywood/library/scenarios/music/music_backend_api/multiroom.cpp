#include "multiroom.h"

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/hollywood/library/multiroom/multiroom.h>
#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/protos/data/device/info.pb.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_JBL = "hw_music_thin_client_jbl";

bool HasJblDevice(const TIoTUserInfo& iotUserInfo) {
    return AnyOf(iotUserInfo.GetDevices(), [](const TIoTUserInfo::TDevice& device) {
        return device.GetType() == EUserDeviceType::JBLLinkPortableDeviceType ||
            device.GetType() == EUserDeviceType::JBLLinkMusicDeviceType;
    });
}

}

TMultiroomTokenWrapper::TMultiroomTokenWrapper(TScenarioState& scenarioState)
    : ScenarioState_{scenarioState}
{
}

void TMultiroomTokenWrapper::GenerateNewToken() {
    ScenarioState_.SetMultiroomToken(CreateGuidAsString());
}

void TMultiroomTokenWrapper::DropToken() {
    ScenarioState_.ClearMultiroomToken();
}

TStringBuf TMultiroomTokenWrapper::GetToken() const {
    return ScenarioState_.GetMultiroomToken();
}

void ProcessMultiroom(const NHollywood::TScenarioApplyRequestWrapper& request,
                      TMultiroomCallbacks& callbacks)
{
    if (!request.HasExpFlag(EXP_HW_MUSIC_THIN_CLIENT_MULTIROOM)) {
        return;
    }
    const auto& deviceState = request.BaseRequestProto().GetDeviceState();

    const auto& applyArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
    const TMultiroom multiroom{&applyArgs.GetIoTUserInfo(), deviceState};

    if (const auto frameProto = request.Input().FindSemanticFrame(MUSIC_PLAY_FRAME)) {
        const auto frame = TFrame::FromProto(*frameProto);

        // we asked for music in some room and the Station is in that room, start multiroom in there
        if (FrameHasSomeLocationSlot(frame)) {
            const auto locationInfo = MakeLocationInfo(frame);
            const bool enabledByExp = !request.HasExpFlag(NBASS::EXPERIMENTAL_FLAG_DISABLE_MULTIROOM);
            const bool multiroomEnabled = request.Interfaces().GetMultiroom();
            const bool multiroomClusterEnabled = request.Interfaces().GetMultiroomCluster();

            if (enabledByExp && multiroomEnabled) {
                if (IsEverywhereLocationInfo(locationInfo) || multiroomClusterEnabled) {
                    callbacks.OnNeedStartMultiroom(locationInfo);
                } else {
                    callbacks.OnMultiroomRoomsNotSupported();
                }
            } else if (enabledByExp || multiroomEnabled) {
                callbacks.OnMultiroomNotSupported();
            }
        }
        // we didn't ask for a location
        else {
            bool multiroomInactive = !multiroom.IsMultiroomTokenActive();

            // we asked "continue"
            if (request.Input().FindSemanticFrame(NAlice::NMusic::PLAYER_CONTINUE)) {
                // do nothing, no need in restarting MR session
            }
            // we asked a generic query like "включи музыку" within an active MR session, should stop multiroom
            else if (IsOnYourWaveRequest(applyArgs) && multiroom.IsMultiroomSessionActive()) {
                multiroomInactive = true;
            }
            // we asked for music on not playing station, clear MR session if present
            else if (!IsAudioPlayerPlaying(deviceState) && multiroom.IsMultiroomSessionActive()) {
                multiroomInactive = true;
            }

            // in case of `multiroomInactive = true` we called `OnNeedStopMultiroom(multiroom.GetMultiroomSessionId())`
            // previously, BUT we don't do this anymore, because the hardware can't process `stop_multiroom`
            // and `audio_play` directives in order correctly (ask sparkle@ or fkuharenok@ if have questions)

            // the music should be played in all device's groups
            if (multiroomInactive) {
                callbacks.OnNeedDropMultiroomToken();
                if (const auto groupIds = multiroom.GetDeviceGroupIds(); !groupIds.empty()) {
                    NScenarios::TLocationInfo locationInfo;
                    for (const auto groupId : groupIds) {
                        locationInfo.AddGroupsIds(groupId.data(), groupId.size());
                    }
                    callbacks.OnNeedStartMultiroom(std::move(locationInfo));
                }
            }
        }
    }
}

bool WillPlayInMultiroomSession(const NHollywood::TScenarioApplyRequestWrapper& request) {
    // check if there is an active multiroom session
    const auto& applyArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
    if (TMultiroom{&applyArgs.GetIoTUserInfo(), request.BaseRequestProto().GetDeviceState()}.IsMultiroomSessionActive()) {
        return true;
    }

    // check if multiroom is going to start in next nodes
    struct TCallbacks : TMultiroomCallbacks {
        bool WillStartMr = false;
        void OnNeedStartMultiroom(NScenarios::TLocationInfo /* locationId */) override {
            WillStartMr = true;
        }
    } callbacks;
    ProcessMultiroom(request, callbacks);
    return callbacks.WillStartMr;
}

bool HasForbiddenMultiroom(NAlice::TRTLogger& logger,
                           const TScenarioRunRequestWrapper& request,
                           const TFrame& musicPlayFrame)
{
    if (!TMultiroom{request, logger}.IsActiveWithFrame(musicPlayFrame)) {
        // this is not a multiroom request, we don't have "forbidden multiroom"
        return false;
    }

    if (!request.HasExpFlag(EXP_HW_MUSIC_THIN_CLIENT_MULTIROOM)) {
        // multiroom is forbidden because the flag is missing
        return true;
    }

    const NScenarios::TDataSource* iotUserInfoPtr = request.GetDataSource(EDataSourceType::IOT_USER_INFO);
    if (iotUserInfoPtr &&
        HasJblDevice(iotUserInfoPtr->GetIoTUserInfo()) &&
        !request.HasExpFlag(EXP_HW_MUSIC_THIN_CLIENT_JBL))
    {
        // multiroom is forbidden because the user has a JBL device
        LOG_INFO(logger) << "Multiroom is forbidden, because the user has a JBL device";
        return true;
    }

    return false;
}

} // namespace NAlice::NHollywood::NMusic
