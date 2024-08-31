#include "multiroom.h"

#include <alice/library/music/defs.h>
#include <alice/protos/extensions/extensions.pb.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/protos/data/device/info.pb.h>

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/reflection.h>

namespace NAlice::NHollywood {

namespace {

template<typename TRequest>
const TIoTUserInfo* TryGetIotUserInfo(const TRequest& request) {
    if (const auto* dataSource = request.GetDataSource(EDataSourceType::IOT_USER_INFO)) {
        if (dataSource->HasIoTUserInfo()) {
            return &dataSource->GetIoTUserInfo();
        }
    }
    return nullptr;
}

TVector<TStringBuf> GetDeviceGroupIdsImpl(const TIoTUserInfo& iotUserInfo, const TString& deviceId) {
    const auto* currentDevice = FindIfPtr(iotUserInfo.GetDevices(),
        [&deviceId] (const auto& device) {
            return device.GetQuasarInfo().GetDeviceId() == deviceId;
        });
    if (currentDevice) {
        const auto& groupIds = currentDevice->GetGroupIds();
        return {groupIds.begin(), groupIds.end()};
    }
    return {};
}

TMaybe<TStringBuf> GetMultiroomMasterDeviceId(const NAlice::TDeviceState_TMultiroom& multiroom) {
    // trusted way
    if (multiroom.HasMasterDeviceId()) {
        return multiroom.GetMasterDeviceId();
    }

    // untrusted legacy hacky way
    if (multiroom.HasMultiroomSessionId()) {
        return TStringBuf{multiroom.GetMultiroomSessionId()}.Before(':');
    }

    return Nothing();
}


const TVector<TStringBuf> NEW_LOCATION_SLOTS_NAMES = {
    LOCATION_ROOM_SLOT_NAME,
    LOCATION_DEVICE_SLOT_NAME,
    LOCATION_GROUP_SLOT_NAME,
    LOCATION_EVERYWHERE_SLOT_NAME,
    LOCATION_SMART_SPEAKER_MODEL_SLOT_NAME,
};


bool FrameHasNewLocationSlot(const TFrame& frame) {
    return AnyOf(NEW_LOCATION_SLOTS_NAMES, [&frame](const auto slotName) {
        return frame.FindSlot(slotName).IsValid();
    });
}

bool FrameHasOldLocationSlot(const TFrame& frame) {
    return frame.FindSlot(LOCATION_SLOT_NAME).IsValid();
}

bool ContainersHaveCommonElement(const auto& first, const auto& second) {
    for (const auto& firstEl : first) {
        for (const auto& secondEl : second) {
            if (firstEl == secondEl) {
                return true;
            }
        }
    }
    return false;
}

template <typename T>
TMaybe<T> TryGetSlotValue(const TFrame& frame, const TStringBuf name) {
    if (const auto slot = frame.FindSlot(name)) {
        return slot->Value.As<T>();
    }
    return {};
}

TVector<TStringBuf> GetPlayingDevicesIds(const TDeviceState& deviceState) {
    TVector<TStringBuf> devices;
    for (const auto& id : deviceState.GetMultiroom().GetRoomDeviceIds()) {
        devices.push_back(id);
    }
    return devices;
}

const google::protobuf::FieldDescriptor*
FindFieldBySlotName(const google::protobuf::Message& frame, TStringBuf slotName) {
    const auto& descriptor = *frame.GetDescriptor();
    for (int i = 0; i < descriptor.field_count(); ++i) {
        const auto& field = *descriptor.field(i);
        if (field.options().GetExtension(SlotName) == slotName) {
            return &field;
        }
    }
    return nullptr;
}

TString GetValue(const NAlice::TLocationSlot& slot) {
    const auto* reflection = slot.GetReflection();
    const auto* value = reflection->GetOneofFieldDescriptor(slot, slot.GetDescriptor()->FindOneofByName("Value"));
    return reflection->GetString(slot, value);
}

TVector<TString> GetValues(const google::protobuf::Message& frame, TStringBuf slotName) {
    TVector<TString> result;
    if (const auto* field = FindFieldBySlotName(frame, slotName)) {
        for (const auto& slot : frame.GetReflection()->GetRepeatedFieldRef<TLocationSlot>(frame, field)) {
            result.emplace_back(GetValue(slot));
        }
    }
    return result;
}

const TList<TSlot>& FindSlots(const TFrame& frame, TStringBuf slotName) {
    if (const auto* slots = frame.FindSlots(slotName)) {
        return *slots;
    }
    static const TList<TSlot> EMPTY_LIST;
    return EMPTY_LIST;
}

} // anonymous namespace

TMultiroom::TMultiroom(const TIoTUserInfo* iotUserInfo,
                       const TDeviceState& deviceState,
                       TRTLogger& logger)
    : Logger_{logger}
    , IotUserInfo_{iotUserInfo}
    , DeviceState_{deviceState}
{}

TMultiroom::TMultiroom(const TScenarioRunRequestWrapper& request,
                       TRTLogger& logger)
    : TMultiroom{TryGetIotUserInfo(request), request.BaseRequestProto().GetDeviceState(), logger}
{}

TMultiroom::TMultiroom(const NHollywoodFw::TRunRequest& request)
    : TMultiroom{TryGetIotUserInfo(request), request.GetRunRequest().GetBaseRequest().GetDeviceState(), request.Debug().Logger()}
{}

TVector<TStringBuf> TMultiroom::GetDeviceGroupIds() const {
    if (IotUserInfo_) {
        return GetDeviceGroupIdsImpl(*IotUserInfo_, DeviceState_.GetDeviceId());
    }
    return {};
}

bool TMultiroom::IsDeviceInGroup() const {
    return !GetDeviceGroupIds().empty();
}

bool TMultiroom::IsDeviceSlave(TStringBuf& outMasterDeviceId) const {
    const TString& deviceId = DeviceState_.GetDeviceId();
    const TString logPrefix = TString::Join("Device \"", deviceId, "\" :");

    if (DeviceState_.GetMultiroom().GetMode() != TDeviceState_TMultiroom_EMultiroomMode_Slave) {
        LOG_INFO(Logger_) << logPrefix << "Is not a slave";
        return false;
    }

    const TMaybe<TStringBuf> masterDeviceId = GetMultiroomMasterDeviceId(DeviceState_.GetMultiroom());
    if (!masterDeviceId) {
        LOG_INFO(Logger_) << logPrefix << "Can't find multiroom's master device id";
        return false;
    }

    if (deviceId == *masterDeviceId) {
        LOG_INFO(Logger_) << logPrefix << "Is the master";
        return false;
    }

    LOG_INFO(Logger_) << logPrefix << "Is a slave of device \"" << *masterDeviceId << "\"";
    outMasterDeviceId = *masterDeviceId;
    return true;
}

bool TMultiroom::IsMultiroomSessionActive() const {
    return !GetMultiroomSessionId().empty();
}

const TString& TMultiroom::GetMultiroomSessionId() const {
    return DeviceState_.GetMultiroom().GetMultiroomSessionId();
}

bool TMultiroom::IsMultiroomTokenActive() const {
    return GetMultiroomToken() != nullptr;
}

const TString* TMultiroom::GetMultiroomToken() const {
    if (const auto& token = DeviceState_.GetMultiroom().GetMultiroomToken(); !token.empty()) {
        return &token;
    }
    return nullptr;
}

THashSet<TStringBuf> TMultiroom::GetVisiblePeersDeviceIds() const {
    const auto& peerIds = DeviceState_.GetMultiroom().GetVisiblePeerDeviceIds();
    return {peerIds.begin(), peerIds.end()};
}


TMaybe<TStringBuf> TMultiroom::FindVisiblePeerFromLocationInfo(const NScenarios::TLocationInfo& locationInfo) const {
    if (!IotUserInfo_) {
        return Nothing();
    }

    const auto locationDevices = FindDevicesQuasarIdsInLocation(locationInfo, *IotUserInfo_);
    const auto peerIds = GetVisiblePeersDeviceIds();
    for (const auto& deviceQuasarId : locationDevices) {
        if (IsIn(peerIds, deviceQuasarId)) {
            return deviceQuasarId;
        }
    }
    return Nothing();
}

bool TMultiroom::LocationIntersectsWithPlayingDevices(const NScenarios::TLocationInfo& locationInfo) const {
    if (!IotUserInfo_) {
        return false;
    }
    const auto locationDevices = FindDevicesQuasarIdsInLocation(locationInfo, *IotUserInfo_);
    const auto playingDevices = GetPlayingDevicesIds(DeviceState_);
    return ContainersHaveCommonElement(locationDevices, playingDevices);
}

bool TMultiroom::IsDeviceInLocation(const NScenarios::TLocationInfo& locationInfo) const {
    if (!IotUserInfo_) {
        return false;
    }
    const auto locationDevices = FindDevicesQuasarIdsInLocation(locationInfo, *IotUserInfo_);
    const auto& thisDeviceId = DeviceState_.GetDeviceId();
    return IsIn(locationDevices, thisDeviceId);
}

bool TMultiroom::IsActive() const {
    bool result = false;

    if (IsMultiroomSessionActive()) {
        LOG_INFO(Logger_) << "HasMultiroom=true, reason: Device has a non-empty MultiroomSessionId";
        result |= true;
    }

    if (IsDeviceInGroup()) {
        LOG_INFO(Logger_) << "HasMultiroom=true, reason: found multiroom group(s) for device in IOT_USER_INFO datasource";
        result |= true;
    }

    if (!result) {
        LOG_INFO(Logger_) << "HasMultiroom=false";
    }
    return result;
}

bool TMultiroom::IsActiveWithFrame(const TPtrWrapper<NAlice::TSemanticFrame>& frameProto) const {
    if (!frameProto) {
        return IsActive();
    }
    TFrame frame = TFrame::FromProto(*frameProto);
    return IsActiveWithFrame(frame);
}

bool TMultiroom::IsActiveWithFrame(const TFrame& frame) const {
    bool result = false;

    if (TryGetSlotValue<bool>(frame, NAlice::NMusic::SLOT_DISABLE_MULTIROOM).GetOrElse(false)) {
        LOG_INFO(Logger_) << "HasMultiroom=false, reason: disable_multiroom=true slot found in the semantic frame";
        return false;
    }

    if (FrameHasSomeLocationSlot(frame)) {
        LOG_INFO(Logger_) << "HasMultiroom=true, reason: some location slot found in needed semantic frame";
        result |= true;
    }

    result |= IsActive();
    return result;
}

NScenarios::TLocationInfo MakeLocationInfo(const google::protobuf::Message& frame) {
    NAlice::NScenarios::TLocationInfo result;
    for (auto&& roomId : GetValues(frame, LOCATION_ROOM_SLOT_NAME)) {
        if (roomId == EVERYWHERE_LOCATION_SLOT) {
            result.SetEverywhere(true);
        } else {
            *result.AddRoomsIds() = std::move(roomId);
        }
    }
    for (auto&& groupId : GetValues(frame, LOCATION_GROUP_SLOT_NAME)) {
        *result.AddGroupsIds() = std::move(groupId);
    }
    for (auto&& deviceId : GetValues(frame, LOCATION_DEVICE_SLOT_NAME)) {
        *result.AddDevicesIds() = std::move(deviceId);
    }
    // TODO(sparkle): add test for smartSpeakerModel when you see frame with add
    if (const auto* field = FindFieldBySlotName(frame, LOCATION_EVERYWHERE_SLOT_NAME)) {
        if (frame.GetReflection()->HasField(frame, field)) {
            result.SetEverywhere(true);
        }
    }
    return result;
}

NScenarios::TLocationInfo MakeLocationInfo(const TFrame& frame) {
    NAlice::NScenarios::TLocationInfo result;

    if (FrameHasNewLocationSlot(frame)) {
        for (const auto& room : FindSlots(frame, LOCATION_ROOM_SLOT_NAME)) {
            if (const auto roomId = room.Value.AsString(); roomId != EVERYWHERE_LOCATION_SLOT) {
                *result.AddRoomsIds() = roomId;
            } else {
                result.SetEverywhere(true);
            }
        }
        for (const auto& group : FindSlots(frame, LOCATION_GROUP_SLOT_NAME)) {
            *result.AddGroupsIds() = group.Value.AsString();
        }
        for (const auto& device : FindSlots(frame, LOCATION_DEVICE_SLOT_NAME)) {
            *result.AddDevicesIds() = device.Value.AsString();
        }
        for (const auto& smartSpeakerModel : FindSlots(frame, LOCATION_SMART_SPEAKER_MODEL_SLOT_NAME)) {
            if (auto smartSpeakerModelId = smartSpeakerModel.Value.As<int>()) {
                result.AddSmartSpeakerModels(static_cast<EUserDeviceType>(smartSpeakerModelId.GetRef()));
            }
        }
        if (frame.FindSlot(LOCATION_EVERYWHERE_SLOT_NAME)) {
            result.SetEverywhere(true);
        }
    } else if (FrameHasOldLocationSlot(frame)) {
        const auto* locationSlot = frame.FindSlot(LOCATION_SLOT_NAME).Get();
        const auto locationId = locationSlot->Value.AsString();
        const auto locationType = locationSlot->Type;
        if (locationId == EVERYWHERE_LOCATION_SLOT) {
            result.SetEverywhere(true);
        } else if (locationType == NAlice::NMusic::SLOT_LOCATION_ROOM_TYPE) {
            *result.AddRoomsIds() = locationId;
        } else if (locationType == NAlice::NMusic::SLOT_LOCATION_GROUP_TYPE) {
            *result.AddGroupsIds() = locationId;
        } else if (locationType == NAlice::NMusic::SLOT_LOCATION_DEVICE_TYPE) {
            *result.AddDevicesIds() = locationId;
        } else if (locationType == NAlice::NMusic::SLOT_LOCATION_SMART_SPEAKER_MODEL_TYPE) {
            const auto locationSlotIntValue = locationSlot->Value.As<int>();
            if (locationSlotIntValue) {
                const auto smartSpeakerModelType = static_cast<EUserDeviceType>(locationSlotIntValue.GetRef());
                result.AddSmartSpeakerModels(smartSpeakerModelType);
            }
        }
    }

    return result;
}

TLocationSlot ConstructLocationSlotFromLocationInfo(const NScenarios::TLocationInfo& locationInfo) {
    TLocationSlot slot;
    if (locationInfo.GetEverywhere()) {
        slot.SetUserIotRoomValue("everywhere");
    } else if (!locationInfo.GetRoomsIds().empty()) {
        slot.SetUserIotRoomValue(locationInfo.GetRoomsIds(0));
    } else if (!locationInfo.GetGroupsIds().empty()) {
        slot.SetUserIotGroupValue(locationInfo.GetGroupsIds(0));
    } else if (!locationInfo.GetDevicesIds().empty()) {
        slot.SetUserIotDeviceValue(locationInfo.GetDevicesIds(0));
    }
    return slot;
}

TVector<TString> FindDevicesQuasarIdsInLocation(const NScenarios::TLocationInfo& locationInfo,
                                                const TIoTUserInfo& iotUserInfo)
{
    if (locationInfo.GetRoomsIds().empty() && locationInfo.GetGroupsIds().empty() &&
        locationInfo.GetDevicesIds().empty() && locationInfo.GetSmartSpeakerModels().empty()
        && !locationInfo.GetEverywhere())
    {
        return {};
    }

    TVector<TString> result;

    for (const auto& device : iotUserInfo.GetDevices()) {
        const bool deviceIsSmartSpeaker = device.HasQuasarInfo();
        const bool deviceIsInTheRightLocation = (
            (locationInfo.GetRoomsIds().empty() || IsIn(locationInfo.GetRoomsIds(), device.GetRoomId())) &&
            (locationInfo.GetGroupsIds().empty() || ContainersHaveCommonElement(locationInfo.GetGroupsIds(), device.GetGroupIds())) &&
            (locationInfo.GetDevicesIds().empty() ||
             IsIn(locationInfo.GetDevicesIds(), device.GetId()) ||
             IsIn(locationInfo.GetDevicesIds(), device.GetQuasarInfo().GetDeviceId())) &&
            (locationInfo.GetSmartSpeakerModels().empty() || IsIn(locationInfo.GetSmartSpeakerModels(), device.GetType()))
        );

        if (deviceIsSmartSpeaker && deviceIsInTheRightLocation) {
            result.push_back(device.GetQuasarInfo().GetDeviceId());
        }
    }
    return result;
}

bool FrameHasSomeLocationSlot(const TFrame& frame) {
    return FrameHasOldLocationSlot(frame) || FrameHasNewLocationSlot(frame);
}

bool IsEverywhereLocationInfo(const NScenarios::TLocationInfo& locationInfo) {
    return (
        locationInfo.GetDevicesIds().empty() &&
        locationInfo.GetGroupsIds().empty() &&
        locationInfo.GetSmartSpeakerModels().empty() &&
        AllOf(locationInfo.GetRoomsIds(), [](const auto& roomId) { return roomId == EVERYWHERE_LOCATION_SLOT; })
    );
}

} // namespace NAlice::NHollywood
