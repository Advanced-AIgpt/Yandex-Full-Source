#pragma once

#include <alice/hollywood/library/framework/core/request.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood {


// multiroom constants
constexpr TStringBuf EVERYWHERE_LOCATION_SLOT = "everywhere";
constexpr TStringBuf ALL_ROOMS_ROOM_ID = "__all__";
constexpr TStringBuf LOCATION_SLOT_NAME = "location";
constexpr TStringBuf LOCATION_ROOM_SLOT_NAME = "location_room";
constexpr TStringBuf LOCATION_DEVICE_SLOT_NAME = "location_device";
constexpr TStringBuf LOCATION_GROUP_SLOT_NAME = "location_group";
constexpr TStringBuf LOCATION_EVERYWHERE_SLOT_NAME = "location_everywhere";
constexpr TStringBuf LOCATION_SMART_SPEAKER_MODEL_SLOT_NAME = "location_smart_speaker_model";


class TMultiroom {
public:
    explicit TMultiroom(const TIoTUserInfo* iotUserInfo,
                        const TDeviceState& deviceState,
                        TRTLogger& logger = TRTLogger::NullLogger());

    explicit TMultiroom(const TScenarioRunRequestWrapper& request,
                        TRTLogger& logger = TRTLogger::NullLogger());

    explicit TMultiroom(const NHollywoodFw::TRunRequest& request);

    // try get device's groups
    TVector<TStringBuf> GetDeviceGroupIds() const;

    // a device is in one or more device groups
    bool IsDeviceInGroup() const;

    // a device is a slave (in case of "true" saves master device id)
    bool IsDeviceSlave(TStringBuf& masterDeviceId) const;

    // is multiroom session active
    bool IsMultiroomSessionActive() const;
    const TString& GetMultiroomSessionId() const;

    // is multiroom token active
    bool IsMultiroomTokenActive() const;
    const TString* GetMultiroomToken() const;

    // "visible peers" is devices that are enabled and in the same network
    THashSet<TStringBuf> GetVisiblePeersDeviceIds() const;

    // search for a visible peer from given locationId
    TMaybe<TStringBuf> FindVisiblePeerFromLocationInfo(const NScenarios::TLocationInfo& locationInfo) const;

    // checks whether `locationInfo` has some devices that are playing now
    bool LocationIntersectsWithPlayingDevices(const NScenarios::TLocationInfo& locationInfo) const;

    // checks whether the current device is in `locationInfo`
    bool IsDeviceInLocation(const NScenarios::TLocationInfo& locationInfo) const;

    // general checks ported from HollywoodMusic
    bool IsActive() const;
    bool IsActiveWithFrame(const TPtrWrapper<NAlice::TSemanticFrame>& frameProto) const;
    bool IsActiveWithFrame(const TFrame& frame) const; // additionally checks for related slots

private:
    TRTLogger& Logger_;
    const TIoTUserInfo* IotUserInfo_;
    const TDeviceState& DeviceState_;
};

bool FrameHasSomeLocationSlot(const TFrame& frame);

NScenarios::TLocationInfo MakeLocationInfo(const google::protobuf::Message& frame);
NScenarios::TLocationInfo MakeLocationInfo(const TFrame& frame); // DEPRECATED, use typed semantic frame

TLocationSlot ConstructLocationSlotFromLocationInfo(const NScenarios::TLocationInfo& locationInfo);

TVector<TString> FindDevicesQuasarIdsInLocation(const NScenarios::TLocationInfo& locationInfo,
                                                const TIoTUserInfo& iotUserInfo);

bool IsEverywhereLocationInfo(const NScenarios::TLocationInfo& locationInfo);

} // namespace NAlice::NHollywood
