#include "indexer.h"

#include "defs.h"

#include <library/cpp/iterator/enumerate.h>


namespace NAlice::NIot {

namespace {

// Helps using << easily, but only works in TSmartHomeIndexer
#define LOG(x) if (LogBuilder) { *LogBuilder << x << Endl; } // Cout << x << Endl;


class TCapabilityIndexer {
public:
    TCapabilityIndexer(TSmartHomeIndex* smartHomeIndex, TStringBuilder* logBuilder)
        : SmartHomeIndex(smartHomeIndex)
        , LogBuilder(logBuilder)
    {
    }

    void Index(const TIoTUserInfo::TCapability& capability, const TVector<TString>& devicesAndGroupsIds) {
        LOG("Indexing capability " << capability.GetAnalyticsName() << "of type " << capability.GetAnalyticsType());

        Capability = &capability;
        DevicesAndGroupsIds = &devicesAndGroupsIds;

        switch (Capability->GetType()) {
            case TIoTUserInfo_TCapability_ECapabilityType_OnOffCapabilityType:
                IndexOnOffCapability();
                break;
            case TIoTUserInfo_TCapability_ECapabilityType_ColorSettingCapabilityType:
                IndexColorSettingCapability();
                break;
            case TIoTUserInfo_TCapability_ECapabilityType_ModeCapabilityType:
                IndexModeCapability();
                break;
            case TIoTUserInfo_TCapability_ECapabilityType_RangeCapabilityType:
                IndexRangeCapability();
                break;
            case TIoTUserInfo_TCapability_ECapabilityType_ToggleCapabilityType:
                IndexToggleCapability();
                break;
            case TIoTUserInfo_TCapability_ECapabilityType_CustomButtonCapabilityType:
                IndexCustomButtonCapability();
                break;
            default:
                LOG("This capability won't be indexed.")
        }
    }

private:
    void IndexBasicCapability(const TStringBuf type, const TString& instance, const TVector<TString>& values = {}) {
        if (type.empty() || instance.empty()) {
            return;
        }

        LOG("Indexing basic capability of type \"" << type << "\" and instance \"" << instance
            << "\" having values [" << JoinSeq(", ", values) << "]");

        auto& thisCapabilityTypeInstanceValues = SmartHomeIndex->CapabilityTypeInstanceToValues[std::make_pair(type, instance)];
        for (const auto& value : values) {
            thisCapabilityTypeInstanceValues.insert(value);
            SmartHomeIndex->CapabilityTypeValueToInstances[std::make_pair(type, value)].insert(instance);
        }

        for (const auto& deviceOrGroupId : *DevicesAndGroupsIds) {
            auto& deviceLikeValues = SmartHomeIndex->DeviceLikeCapabilityTypeInstanceToValues[std::make_tuple(deviceOrGroupId, ToString(type), instance)];
            for (const auto& value : values) {
                deviceLikeValues.insert(value);
            }
        }
    }

    void IndexOnOffCapability() {
        IndexBasicCapability(CAPABILITY_TYPE_ON_OFF, "on");
    }

    void IndexColorSettingCapability() {
        IndexBasicCapability(CAPABILITY_TYPE_COLOR_SETTING, "color");

        const auto& params = Capability->GetColorSettingCapabilityParameters();
        if (params.HasTemperatureK()) {
            IndexBasicCapability(CAPABILITY_TYPE_COLOR_SETTING, "temperature_k");
        }

        if (params.HasColorSceneParameters()) {
            TVector<TString> values;
            for (const auto& scene : params.GetColorSceneParameters().GetScenes()) {
                if (!IsIn(values, scene.GetID())) {
                    values.push_back(scene.GetID());
                }
            }
            IndexBasicCapability(CAPABILITY_TYPE_COLOR_SETTING, "color_scene", values);
        }
    }

    void IndexModeCapability() {
        TVector<TString> values;
        for (const auto& mode : Capability->GetModeCapabilityParameters().GetModes()) {
            values.push_back(mode.GetValue());
        }

        const auto& instance = Capability->GetModeCapabilityParameters().GetInstance();
        IndexBasicCapability(CAPABILITY_TYPE_MODE, instance, values);

        if (!instance.empty()) {
            SmartHomeIndex->ModeInstances.insert(instance);
        }
    }

    void IndexRangeCapability() {
        IndexBasicCapability(CAPABILITY_TYPE_RANGE, Capability->GetRangeCapabilityParameters().GetInstance());
    }

    void IndexToggleCapability() {
        IndexBasicCapability(CAPABILITY_TYPE_TOGGLE, Capability->GetToggleCapabilityParameters().GetInstance());
    }

    void IndexCustomButtonCapability() {
        IndexBasicCapability(CAPABILITY_TYPE_CUSTOM_BUTTON, Capability->GetCustomButtonCapabilityParameters().GetInstance());
    }

private:
    TSmartHomeIndex* SmartHomeIndex;
    mutable TStringBuilder* LogBuilder;
    const TIoTUserInfo::TCapability* Capability;
    const TVector<TString>* DevicesAndGroupsIds;
};


class TSmartHomeIndexBuilder {
public:
    TSmartHomeIndexBuilder(TStringBuilder* logBuilder)
        : LogBuilder(logBuilder)
    {
    }

    TSmartHomeIndex Build(const TIoTEnvironment& env) {
        TSmartHomeIndex result;

        ClientDeviceId_ = env.ClientDeviceId;
        Result_ = &result;

        Result_->SmartHomesCount = env.SmartHomes.size();

        for (const auto& [smartHomeId, smartHome] : Enumerate(env.SmartHomes)) {
            CurrentSmartHomeId_ = smartHomeId;

            for (const auto& device : smartHome->GetDevices()) {
                ProcessDevice(device);
            }

            for (const auto& group : smartHome->GetGroups()) {
                ProcessGroup(group);
            }

            for (const auto& room : smartHome->GetRooms()) {
                ProcessRoom(room);
            }

            LOG("===");
        }

        return result;
    }

private:
    void ProcessRoom(const TUserRoom& room) {
        if (room.GetId().empty()) {
            return;
        }
        Result_->EntityToSmartHomeNumber[room.GetId()] = CurrentSmartHomeId_;
    }

    void ProcessGroup(const TUserGroup& group) {
        if (group.GetId().empty()) {
            return;
        }
        Result_->EntityToSmartHomeNumber[group.GetId()] = CurrentSmartHomeId_;
        Result_->GroupsIds.push_back(group.GetId());

        ProcessDeviceLikeEntity(group.GetId(), group.GetAnalyticsType());
    }

    void ProcessDevice(const TIoTUserInfo::TDevice& device) {
        if (device.GetId().empty()) {
            return;
        }
        Result_->EntityToSmartHomeNumber[device.GetId()] = CurrentSmartHomeId_;

        FillDeviceTypeToIds(device.GetId(), device.GetAnalyticsType(), Result_->DeviceTypeToDevicesIds);
        ProcessDeviceLikeEntity(device.GetId(), device.GetAnalyticsType());
        Result_->DeviceIds.push_back(device.GetId());

        const bool thisDeviceIsSourceOfUtterance = (
            !ClientDeviceId_.empty() && device.GetQuasarInfo().GetDeviceId() == ClientDeviceId_
        );
        if (thisDeviceIsSourceOfUtterance) {
            Result_->ClientRoom = device.GetRoomId();
            Result_->ClientHousehold = device.GetHouseholdId();
            Result_->ClientDeviceId = device.GetId();
        }

        if (!device.GetQuasarInfo().GetDeviceId().empty()) {
            Result_->SmartSpeakersIds.push_back(device.GetId());
        }

        if (!device.GetRoomId().empty()) {
            Result_->DeviceIdToRoom[device.GetId()] = device.GetRoomId();
            Result_->RoomToDeviceIds[device.GetRoomId()].insert(device.GetId());

            for (const auto& groupId : device.GetGroupIds()) {
                Result_->GroupIdToRooms[groupId].insert(device.GetRoomId());
            }
        }

        if (!device.GetHouseholdId().empty()) {
            Result_->DeviceIdToHousehold[device.GetId()] = device.GetHouseholdId();
            Result_->HouseholdToDeviceIds[device.GetHouseholdId()].insert(device.GetId());
        }

        TVector<TString> deviceIdAndItsGroupsIds(device.GetGroupIds().begin(), device.GetGroupIds().end());
        deviceIdAndItsGroupsIds.push_back(device.GetId());

        for (const auto& property : device.GetProperties()) {
            TString propertyInstance, propertyType;
            switch (property.GetType()) {
                case TIoTUserInfo_TProperty_EPropertyType_BoolPropertyType:
                    propertyInstance = property.GetBoolPropertyParameters().GetInstance();
                    propertyType = "devices.properties.bool";
                    break;
                case TIoTUserInfo_TProperty_EPropertyType_FloatPropertyType:
                    propertyInstance = property.GetFloatPropertyParameters().GetInstance();
                    propertyType = "devices.properties.float";
                    break;
                default:
                    continue;
            }
            if (propertyInstance.empty()) {
                continue;
            }

            for (const auto& deviceLike : deviceIdAndItsGroupsIds) {
                Result_->DeviceLikePropertyTypeInstances.insert(std::make_tuple(deviceLike, propertyType, propertyInstance));
            }
        }

        if (!device.GetCapabilities().empty()) {
            for (const auto& id : deviceIdAndItsGroupsIds) {
                Result_->DeviceLikeWithCapabilityInfo.insert(id);
            }
        }
        for (const auto& capability : device.GetCapabilities()) {
            TCapabilityIndexer(Result_, LogBuilder).Index(capability, deviceIdAndItsGroupsIds);
        }
    }

    void ProcessDeviceLikeEntity(const TString& id, const TString& type)  {
        if (type.empty()) {
            LOG("No type");
            return;
        }

        Result_->DeviceLikeIdToType[id] = type;

        FillDeviceTypeToIds(id, type, Result_->DeviceTypeToDevicesAndGroupsIds);
    }

    static void FillDeviceTypeToIds(const TString& id, const TString& type,
                                    THashMap<TString, THashSet<TString>>& DeviceTypeToIds)
    {
        DeviceTypeToIds[DEVICE_TYPE_ALL].insert(id);
        DeviceTypeToIds[type].insert(id);

        auto subtype = type;
        auto dotPosition = subtype.find('.');
        while (dotPosition != TString::npos) {
            subtype = subtype.substr(dotPosition + 1);
            DeviceTypeToIds[subtype].insert(id);
            dotPosition = subtype.find('.');
        }
    }

private:
    TStringBuilder* LogBuilder;
    TString ClientDeviceId_;
    TSmartHomeIndex* Result_;
    int CurrentSmartHomeId_;
};

} // namespace

TSmartHomeIndex BuildIndex(const TIoTEnvironment& env, TStringBuilder* logBuilder) {
    return TSmartHomeIndexBuilder(logBuilder).Build(env);
}

} // namespace NAlice::NIot
