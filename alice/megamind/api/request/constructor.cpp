#include "constructor.h"

#include <alice/library/json/json.h>
#include <alice/library/proto/protobuf.h>
#include <alice/library/restriction_level/protos/content_settings.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/megamind/protos/common/experiments.pb.h>
#include <alice/megamind/protos/common/location.pb.h>
#include <alice/protos/data/contacts.pb.h>

#include <util/string/builder.h>
#include <util/string/cast.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <utility>

namespace NAlice::NMegamindApi {

namespace {

constexpr double DEFAULT_LBS_ACCURACY = 140;

void PatchLbs(TSpeechKitRequestProto::TRequest& request) {
    if (!request.HasLocation()) {
        return;
    }
    auto& location = *request.MutableLocation();
    if (!location.HasAccuracy()) {
        location.SetAccuracy(DEFAULT_LBS_ACCURACY);
    }
}

void PatchDeviceState(TSpeechKitRequestProto::TRequest& request) {
    if (const auto& currentlyPlaying = request.GetDeviceState().GetVideo().GetCurrentlyPlaying();
        currentlyPlaying.HasPaused())
    {
        request.MutableDeviceState()->MutableVideo()->MutablePlayer()->SetPause(currentlyPlaying.GetPaused());
    }
}

// FIXME(g-kostin): Temporary fix to prevent invalid child_content_settings value to break request.
//  Should be removed after proper implementation of request sparsing MEGAMIND-1709 or proper support
//  in arcadia protobuf/json_util CONTRIB-1948.
// + Fix for video.current_screen, should be remove after AI resolving in https://st.yandex-team.ru/SPI-29461
bool TryToFixRequest(NJson::TJsonValue& skrJson, TRTLogger& logger) {
    bool isFixApplied = false;
    const static auto DEFAULT_CHILD_CONTENT = EContentSettings_Name(EContentSettings::children);
    auto& deviceState = skrJson["request"]["device_state"];
    /* device_config.child_content_settings */ {
        auto& deviceConfig = deviceState["device_config"];
        if (deviceConfig.Has("child_content_settings")) {
            const auto& childContentSettingsString = deviceConfig["child_content_settings"].GetString();
            EContentSettings value;
            if (!EContentSettings_Parse(childContentSettingsString, &value)) {
                LOG_WARN(logger) << "Changed child_content_settings from invalid \"" << childContentSettingsString
                                << "\" to default value \"" << DEFAULT_CHILD_CONTENT << "\"";
                deviceConfig["child_content_settings"] = DEFAULT_CHILD_CONTENT;
                isFixApplied = true;
            }
        }
    }
    /* video.current_screen */ {
        auto& video = deviceState["video"];
        if (!video["current_screen"].IsString()) {
            video.EraseValue("current_screen");
            isFixApplied = true;
        }
    }
    return isFixApplied;
}

void FixProtoDefaults(const NJson::TJsonValue& skrJson, TSpeechKitRequestProto& skr) {
    // Fix default for ChildContentSettings from medium to children
    const auto& deviceState = skr.GetRequest().GetDeviceState();
    if (deviceState.HasDeviceConfig()) {
        const auto childContentSettingsName = EContentSettings_Name(deviceState.GetDeviceConfig().GetChildContentSettings());
        if (skrJson["request"]["device_state"]["device_config"]["child_content_settings"].GetString() != childContentSettingsName) {
            skr.MutableRequest()->MutableDeviceState()->MutableDeviceConfig()->SetChildContentSettings(EContentSettings::children);
        }
    }
}

TMaybe<TString> ValidateActions(const NJson::TJsonValue& actionsJson) {
    google::protobuf::util::JsonParseOptions strictParseOptions;
    strictParseOptions.ignore_unknown_fields = false;

    for (const auto& [name, action] : actionsJson.GetMapSafe()) {
        NAlice::TDeviceStateAction actionProto;
        const auto status = JsonStringToMessage(ToString(action), &actionProto, strictParseOptions);
        if (!status.ok()) {
            return TStringBuilder{} << "ParseRequest: Can not parse action: " << status.ToString();
        }
    }

    return Nothing();
}

TMaybe<TString> ConvertExperiments(const NJson::TJsonValue& experimentsJson, TSpeechKitRequestProto& skrProto) {
    auto& storageProto = *skrProto.MutableRequest()->MutableExperiments()->MutableStorage();
    if (experimentsJson.IsMap()) {
        for (const auto& exp : experimentsJson.GetMap()) {
            auto& value = storageProto[exp.first];
            switch (exp.second.GetType()) {
                case NJson::EJsonValueType::JSON_INTEGER:
                case NJson::EJsonValueType::JSON_UINTEGER:
                case NJson::EJsonValueType::JSON_DOUBLE:
                    value.SetNumber(exp.second.GetDoubleSafe());
                    break;

                case NJson::EJsonValueType::JSON_BOOLEAN:
                    value.SetBoolean(exp.second.GetBooleanSafe());
                    break;

                case NJson::EJsonValueType::JSON_STRING:
                    value.SetString(exp.second.GetStringSafe());
                    break;

                case NJson::EJsonValueType::JSON_NULL:
                case NJson::EJsonValueType::JSON_UNDEFINED:
                    break;

                case NJson::EJsonValueType::JSON_MAP:
                case NJson::EJsonValueType::JSON_ARRAY:
                    return TStringBuilder{} << "Invalid type (" << exp.second.GetType()
                                            << ") of experimental flag (" << exp.first << ')';
            }
        }
    } else if (experimentsJson.IsArray()) {
        // Backward compatibility for old applications.
        for (const auto& exp : experimentsJson.GetArray()) {
            if (!exp.IsString()) {
                return TStringBuilder{} << "Invalid type of experiment field (must be string): "
                                        << exp.GetType();
            }
            storageProto[exp.GetString()].SetString("1");
        }

    } else if (experimentsJson.IsDefined()) {
        return TStringBuilder{}
            << "experiments field should be either a dict or an array, got: "
            << experimentsJson.GetType();
    }

    return Nothing();
}

void ParseRawDeviceAndEnvironmentState(TSpeechKitRequestProto& skr) {
    if (skr.GetRequest().HasDeviceStateRaw()) {
        NAlice::TDeviceState deviceState;
        if (deviceState.ParseFromString(skr.GetRequest().GetDeviceStateRaw())) {
            skr.MutableRequest()->MutableDeviceState()->CopyFrom(deviceState);
            skr.MutableRequest()->ClearDeviceStateRaw();
        }
    }
    if (skr.GetRequest().HasEnvironmentStateRaw()) {
        NAlice::TEnvironmentState environmentState;
        if (environmentState.ParseFromString(skr.GetRequest().GetEnvironmentStateRaw())) {
            skr.MutableRequest()->MutableEnvironmentState()->CopyFrom(environmentState);
            skr.MutableRequest()->ClearEnvironmentStateRaw();
        }
    }
}

} // namespace

TRequestConstructor::TProtocolStatus TRequestConstructor::PushSpeechKitJson(const NJson::TJsonValue& speechKitJson) {
    constexpr auto validateUtf8 = false;
    constexpr auto ignoreUnknownFields = true;

    auto status = JsonToProto(speechKitJson, SpeechKitRequest, validateUtf8, ignoreUnknownFields);
    if (!status.ok()) {
        LOG_ERROR(Logger) << "Unable to parse SKR (JsonToProto): " << status.ToString() << "\nTrying to fix request.";
        auto mutableJson = speechKitJson;
        if (TryToFixRequest(mutableJson, Logger)) {
            status = JsonToProto(mutableJson, SpeechKitRequest, validateUtf8, ignoreUnknownFields);
            if (!status.ok()) {
                LOG_ERROR(Logger) << "Impossible to parse SKR (JsonToProto): " << status.ToString();
                return TProtocolStatus{TProtocolStatus::EStatusCode::ParseError, status.ToString()};
            }
        }
        ParseRawDeviceAndEnvironmentState(SpeechKitRequest);
    } else {
        ParseRawDeviceAndEnvironmentState(SpeechKitRequest);
        // New protobuf compatibility. Remove previous block after update.
        FixProtoDefaults(speechKitJson, SpeechKitRequest);
    }

    if (const auto* personalDataJson = speechKitJson.GetValueByPath("request.personal_data")) {
        SpeechKitRequest.MutableRequest()->SetRawPersonalData(JsonToString(*personalDataJson));
    }

    PatchContacts(SpeechKitRequest, Logger);

    const auto& contacts = SpeechKitRequest.GetContacts();
    const auto& contactsData = contacts.GetData();
    LOG_INFO(Logger) << "Contacts status: " <<  contacts.GetStatus()
                     << " IsKnownUuid: " << contactsData.GetIsKnownUuid()
                     << " Deleted: " << contactsData.GetDeleted()
                     << " Truncated: " << contactsData.GetTruncated()
                     << " ContactsCount: " << contactsData.GetContacts().size()
                     << " PhonesCount: " << contactsData.GetPhones().size()
                     << Endl;

    if (auto errStr = ConvertExperiments(speechKitJson["request"]["experiments"], SpeechKitRequest)) {
        LOG_WARN(Logger) << "ParseRequest: convert experiments problem: " << *errStr;
    }

    if (const auto* actionsJson = speechKitJson.GetValueByPath("request.device_state.actions")) {
        if (auto errStr = ValidateActions(*actionsJson)) {
            return TProtocolStatus{TProtocolStatus::EStatusCode::ParseError, *errStr};
        }
    }

    return TProtocolStatus::StatusOk();
}

TSpeechKitRequestProto TRequestConstructor::MakeRequest() && {
    PatchLbs(*SpeechKitRequest.MutableRequest());
    PatchDeviceState(*SpeechKitRequest.MutableRequest());
    return std::move(SpeechKitRequest);
}

// static
void TRequestConstructor::PatchContacts(TSpeechKitRequestProto& skr, TRTLogger& log) {
    if (skr.GetContacts().GetStatus() == "ok") {
        LOG_INFO(log) << "Got skr contacts" << Endl;
        return;
    }

    const auto& contactsString = skr.GetContactsProto();
    if (contactsString.empty()) {
        return;
    }

    TString errMsg;
    TSpeechKitRequestProto::TContacts contactsProto;
    try {
        if (contactsProto.ParseFromString(Base64Decode(contactsString))) {
            skr.MutableContacts()->Swap(&contactsProto);
            skr.ClearContactsProto();
            return;
        }
        errMsg = "TSpeechKitRequestProto::TContacts::ParseFromString failed";
    } catch (...) {
        errMsg = CurrentExceptionMessage();
    }

    LOG_ERROR(log) << "Unable to parse contacts_proto: " << errMsg << Endl;
}

} // namespace NAlice::NMegamindApi
