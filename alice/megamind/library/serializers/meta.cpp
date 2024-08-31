#include "meta.h"

#include <alice/megamind/library/common/defs.h>

#include <alice/library/search/defs.h>

namespace NAlice::NMegamind {

TProtoStructBuilder GetCallbackPayload(const google::protobuf::Struct& rawPayload,
                                       const TSerializerMeta& serializerMeta, const TString& directiveName) {
    auto payload = TProtoStructBuilder(rawPayload);
    if (!serializerMeta.GetRequestId().empty()) {
        payload.Set(TString{REQUEST_ID_JSON_KEY}, serializerMeta.GetRequestId());
    }
    if (!serializerMeta.GetScenarioName().empty()) {
        payload.Set(TString{SCENARIO_NAME_JSON_KEY}, serializerMeta.GetCallbackDirectiveScenarioName(directiveName));
    }
    return payload;
}

google::protobuf::ListValue ToProtoList(const TVector<TString>& items) {
    TProtoListBuilder builder{};
    for (const auto& item : items) {
        builder.Add(item);
    }
    return builder.Build();
}

// TSerializerMeta -------------------------------------------------------------
TSerializerMeta::TSerializerMeta(const TString& scenarioName, const TString& requestId,
                                 const TClientInfo& clientInfo,
                                 const TMaybe<TIoTUserInfo>& iotUserInfo,
                                 const TSmartHomeInfo& smartHomeInfo,
                                 const TSpeechKitRequestProto::TRequest::TAdditionalOptions& additionalOptions,
                                 const IGuidGenerator& guidGenerator, const TString& onSuggestScenarioName,
                                 const TBuilderOptions& options)
    : OnSuggestScenarioName(onSuggestScenarioName)
    , ScenarioName(scenarioName)
    , RequestId(requestId)
    , ClientInfo(clientInfo)
    , IoTUserInfo(iotUserInfo)
    , SmartHomeInfo(smartHomeInfo)
    , AdditionalOptions(additionalOptions)
    , GuidGenerator(guidGenerator.Clone())
    , BuilderOptions(options)
{
}

const TString& TSerializerMeta::GetCallbackDirectiveScenarioName(const TString& directiveName) const {
    // TODO(alkapov): remove on suggest case
    return directiveName == ON_SUGGEST_DIRECTIVE_NAME ? OnSuggestScenarioName : GetScenarioName();
}

const TString& TSerializerMeta::GetScenarioName() const {
    return ScenarioName;
}

const TString& TSerializerMeta::GetRequestId() const {
    return RequestId;
}

const TSmartHomeInfo& TSerializerMeta::GetSmartHomeInfo() const {
    return SmartHomeInfo;
}

const TClientInfo& TSerializerMeta::GetClientInfo() const {
    return ClientInfo;
}

const TMaybe<TIoTUserInfo>& TSerializerMeta::GetIoTUserInfo() const {
    return IoTUserInfo;
}

const TSpeechKitRequestProto::TRequest::TAdditionalOptions& TSerializerMeta::GetAdditionalOptions() const {
    return AdditionalOptions;
}

const TIntrusivePtr<IGuidGenerator>& TSerializerMeta::GetGuidGenerator() const {
    return GuidGenerator;
}

TString TSerializerMeta::WrapDialogId(const TString& dialogId) const {
    // Dialogovo is going to replace Vins' externall skills in tabs
    if (GetScenarioName().empty() || GetScenarioName() == MM_DIALOGOVO_SCENARIO) {
        return dialogId;
    }
    return TStringBuilder{} << GetScenarioName() << DIALOG_ID_DELIMITER << dialogId;
}

const TBuilderOptions& TSerializerMeta::GetBuilderOptions() const {
    return BuilderOptions;
}

void ForEachQuasarDeviceIdInLocation(const TIoTUserInfoDevices& ioTUserInfoDevices, const TString& locationId,
                                     std::function<void(const TString&)> onDeviceId) {
    for (const auto& device : ioTUserInfoDevices) {
        const bool deviceIsSmartSpeaker = device.HasQuasarInfo();
        const bool deviceIsInTheRightLocation = (
            locationId == "__all__" ||
            device.GetRoomId() == locationId ||
            device.GetId() == locationId ||
            device.GetQuasarInfo().GetDeviceId() == locationId ||
            IsIn(device.GetGroupIds(), locationId)
        );

        if (deviceIsSmartSpeaker && deviceIsInTheRightLocation) {
            onDeviceId(device.GetQuasarInfo().GetDeviceId());
        }
    }
}

using TProtoArrayOfStrings = ::google::protobuf::RepeatedPtrField<TProtoStringType>;
bool ArraysHaveCommonElement(const TProtoArrayOfStrings& first, const TProtoArrayOfStrings& second) {
    for (const auto& first_el : first) {
        for (const auto& second_el : second) {
            if (first_el == second_el) {
                return true;
            }
        }
    }
    return false;
}

void ForEachQuasarDeviceIdInLocation(const TIoTUserInfoDevices& ioTUserInfoDevices,
                                     const NScenarios::TLocationInfo& locationInfo,
                                     std::function<void(const TString&)> onDeviceId,
                                     const TString& currentDeviceId) {
    if (locationInfo.GetRoomsIds().empty() && locationInfo.GetGroupsIds().empty() &&
        locationInfo.GetDevicesIds().empty() && locationInfo.GetSmartSpeakerModels().empty()
        && !locationInfo.GetEverywhere())
    {
        return;
    }

    for (const auto& device : ioTUserInfoDevices) {
        const bool deviceIsSmartSpeaker = device.HasQuasarInfo();
        const bool deviceIsInTheRightLocationAccordingToFilters = (
            (locationInfo.GetRoomsIds().empty() || IsIn(locationInfo.GetRoomsIds(), device.GetRoomId())) &&
            (locationInfo.GetGroupsIds().empty() || ArraysHaveCommonElement(locationInfo.GetGroupsIds(), device.GetGroupIds())) &&
            (locationInfo.GetDevicesIds().empty() || 
             IsIn(locationInfo.GetDevicesIds(), device.GetId()) ||
             IsIn(locationInfo.GetDevicesIds(), device.GetQuasarInfo().GetDeviceId())) &&
            (locationInfo.GetSmartSpeakerModels().empty() || IsIn(locationInfo.GetSmartSpeakerModels(), device.GetType()))
        );
        const bool deviceIsExplicitlyInTheRightLocation = (
            locationInfo.GetIncludeCurrentDeviceId() &&
            device.GetQuasarInfo().GetDeviceId() == currentDeviceId
        );
        const bool deviceIsInTheRightLocation = (
            deviceIsInTheRightLocationAccordingToFilters ||
            deviceIsExplicitlyInTheRightLocation
        );
        if (deviceIsSmartSpeaker && deviceIsInTheRightLocation) {
            onDeviceId(device.GetQuasarInfo().GetDeviceId());
        }
    }
}

void ForEachQuasarDeviceIdThatSharesGroupWith(const TIoTUserInfoDevices& ioTUserInfoDevices, const TString& deviceId,
                                              std::function<void(const TString&)> onDeviceId) {
    const auto* currentDevice = FindIfPtr(ioTUserInfoDevices, [&deviceId] (const auto& device) {
        return device.GetQuasarInfo().GetDeviceId() == deviceId;
    });
    if (!currentDevice || currentDevice->GetGroupIds().empty()) {
        return;
    }

    for (const auto& device : ioTUserInfoDevices) {
        if (!device.HasQuasarInfo()) {
            continue;
        }
        for (const auto& groupId : device.GetGroupIds()) {
            if (IsIn(currentDevice->GetGroupIds(), groupId)) {
                onDeviceId(device.GetQuasarInfo().GetDeviceId());
                break;
            }
        }
    }
}

} // namespace NAlice::NMegamind
