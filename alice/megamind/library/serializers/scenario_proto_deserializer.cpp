#include "scenario_proto_deserializer.h"

#include "speechkit_struct_serializer.h"

#include <alice/megamind/library/common/defs.h>
#include <alice/megamind/library/models/buttons/action_button_model.h>
#include <alice/megamind/library/models/cards/div2_card_model.h>
#include <alice/megamind/library/models/cards/div_card_model.h>
#include <alice/megamind/library/models/directives/memento_change_user_objects_directive_model.h>
#include <alice/megamind/library/models/directives/protobuf_uniproxy_directive_model.h>
#include <alice/megamind/library/models/directives/search_callback_directive_model.h>
#include <alice/megamind/library/models/directives/typed_semantic_frame_request_directive_model.h>
#include <alice/megamind/library/models/directives/update_datasync_directive_model.h>

#include <alice/library/json/json.h>
#include <alice/library/multiroom/location_info.h>
#include <alice/library/proto/protobuf.h>
#include <alice/library/typed_frame/typed_frames.h>

#include <alice/protos/api/matrix/schedule_action.pb.h>
#include <alice/protos/div/div2card.pb.h>
#include <alice/megamind/protos/common/permissions.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <alice/megamind/protos/scenarios/action_space.pb.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>
#include <google/protobuf/descriptor.pb.h>

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/generic/hash.h>
#include <util/generic/scope.h>
#include <util/string/cast.h>
#include <util/string/subst.h>

#include <utility>

namespace NAlice::NMegamind {

using namespace NAlice::NVideoCommon;

namespace {

// Use escapes from urllib.quote_plus
const THashMap<TStringBuf, TStringBuf> DEEP_LINK_CGI_ESCAPES = {
    {TStringBuf("+"), TStringBuf("%20")}, // We escape space (' ') here (due to cgi print implementation)
    {TStringBuf(";"), TStringBuf("%3B")},
    {TStringBuf("@"), TStringBuf("%40")},
};

constexpr TStringBuf DIALOG_ACTION_SCHEME = "dialog-action://?";

using TDeepLinkCreatorDelegate = std::function<TString(TString)>;

void ProcessPotentialDeepLink(TString& value, const TDeepLinkCreatorDelegate& deepLinkCreator) {
    if (!value.StartsWith(MM_DEEPLINK_PLACEHOLDER)) {
        return;
    }
    const auto actionId = value.substr(MM_DEEPLINK_PLACEHOLDER.size());
    value = deepLinkCreator(actionId);
}

void InflateDeepLinks(google::protobuf::Struct& structure, const TDeepLinkCreatorDelegate& deepLinkCreator);

void InflateDeepLinks(google::protobuf::ListValue& listValue, const TDeepLinkCreatorDelegate& deepLinkCreator);

void InflateDeepLinks(google::protobuf::Struct& structure, const TDeepLinkCreatorDelegate& deepLinkCreator) {
    auto* node = structure.mutable_fields();
    using KindCase = google::protobuf::Value::KindCase;
    for (auto& [key, value] : *node) {
        switch (value.kind_case()) {
            case KindCase::kStringValue:
                ProcessPotentialDeepLink(*value.mutable_string_value(), deepLinkCreator);
                break;
            case KindCase::kStructValue:
                InflateDeepLinks(*value.mutable_struct_value(), deepLinkCreator);
                break;
            case KindCase::kListValue: {
                InflateDeepLinks(*value.mutable_list_value(), deepLinkCreator);
                break;
            }
            case KindCase::kBoolValue:
            case KindCase::kNullValue:
            case KindCase::kNumberValue:
            case KindCase::KIND_NOT_SET:
                break;
        }
    }
}

void InflateDeepLinks(google::protobuf::ListValue& listValue, const TDeepLinkCreatorDelegate& deepLinkCreator) {
    auto* values = listValue.mutable_values();
    using KindCase = google::protobuf::Value::KindCase;
    for (auto& value : *values) {
        switch (value.kind_case()) {
            case KindCase::kStringValue:
                ProcessPotentialDeepLink(*value.mutable_string_value(), deepLinkCreator);
                break;
            case KindCase::kStructValue:
                InflateDeepLinks(*value.mutable_struct_value(), deepLinkCreator);
                break;
            case KindCase::kListValue: {
                InflateDeepLinks(*value.mutable_list_value(), deepLinkCreator);
                break;
            }
            case KindCase::kBoolValue:
            case KindCase::kNullValue:
            case KindCase::kNumberValue:
            case KindCase::KIND_NOT_SET:
                break;
        }
    }
}

TIntrusivePtr<IDirectiveModel> GenerateOnSuggestDirective(const TSerializerMeta& meta, const TString& caption,
                                                          TMaybe<TString> nluHint = Nothing()) {
    auto payload = TProtoStructBuilder()
                       .Set("caption", caption)
                       .Set("request_id", meta.GetRequestId())
                       .Set("scenario_name", meta.GetScenarioName())
                       .Set("button_id", meta.GetGuidGenerator()->GenerateGuid());
    if (nluHint.Defined()) {
        payload.Set("nlu_hint", nluHint.GetRef());
    }
    return MakeIntrusive<TCallbackDirectiveModel>(TString{ON_SUGGEST_DIRECTIVE_NAME}, /* ignoreAnswer= */ true,
                                                  payload.Build(), /* isLedSilent= */ true);
}

template <class TDirectiveType>
const google::protobuf::Message& GetDirectiveCase(const TDirectiveType& directive) {
    const auto fieldIndex = static_cast<int>(directive.GetDirectiveCase());
    return directive.GetReflection()->GetMessage(directive, directive.GetDescriptor()->FindFieldByNumber(fieldIndex));
}

template <typename TOption>
const TString& GetMessageOptionValue(const google::protobuf::Message& message, TOption option) {
    return message.GetDescriptor()->options().GetExtension(option);
}

TString GetStringFieldValue(const google::protobuf::Message& message, const TString& name) {
    return message.GetReflection()->GetString(message, message.GetDescriptor()->FindFieldByName(name));
}

TMaybe<NScenarios::TLocationInfo> GetMaybeLocationInfoFieldMessage(const google::protobuf::Message& message) {
    const auto* fieldDescriptor = message.GetDescriptor()->FindFieldByName("LocationInfo");
    if (fieldDescriptor != nullptr && message.GetReflection()->HasField(message, fieldDescriptor)) {
        const auto& locationInfoMessage = message.GetReflection()->GetMessage(message, fieldDescriptor);
        NScenarios::TLocationInfo result;
        if (result.GetDescriptor() == locationInfoMessage.GetDescriptor()) {
            result.CopyFrom(locationInfoMessage);
            return result;
        }
    }
    return Nothing();
}

bool DoesFieldExists(const google::protobuf::Message& message, const TString& name) {
    return message.GetDescriptor()->FindFieldByName(name) != nullptr;
}

EPlayerRewindType GetRewindType(const NScenarios::TPlayerRewindDirective::EType type) {
    switch (type) {
        case NScenarios::TPlayerRewindDirective_EType_Forward:
            return EPlayerRewindType::Forward;
        case NScenarios::TPlayerRewindDirective_EType_Backward:
            return EPlayerRewindType::Backward;
        default:
            return EPlayerRewindType::Absolute;
    }
}

ESettingsTarget GetModelsEnum(const NScenarios::TOpenSettingsDirective::ESettingsTarget target) {
    switch (target) {
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Accessibility:
            return ESettingsTarget::Accessibility;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Archiving:
            return ESettingsTarget::Archiving;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Colors:
            return ESettingsTarget::Colors;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Datetime:
            return ESettingsTarget::Datetime;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_DefaultApps:
            return ESettingsTarget::DefaultApps;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Defender:
            return ESettingsTarget::Defender;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Desktop:
            return ESettingsTarget::Desktop;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Display:
            return ESettingsTarget::Display;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Firewall:
            return ESettingsTarget::Firewall;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Folders:
            return ESettingsTarget::Folders;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_HomeGroup:
            return ESettingsTarget::HomeGroup;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Indexing:
            return ESettingsTarget::Indexing;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Keyboard:
            return ESettingsTarget::Keyboard;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Language:
            return ESettingsTarget::Language;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_LockScreen:
            return ESettingsTarget::LockScreen;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Microphone:
            return ESettingsTarget::Microphone;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Mouse:
            return ESettingsTarget::Mouse;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Network:
            return ESettingsTarget::Network;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Notifications:
            return ESettingsTarget::Notifications;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Power:
            return ESettingsTarget::Power;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Print:
            return ESettingsTarget::Print;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Privacy:
            return ESettingsTarget::Privacy;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Sound:
            return ESettingsTarget::Sound;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_StartMenu:
            return ESettingsTarget::StartMenu;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_System:
            return ESettingsTarget::System;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_TabletMode:
            return ESettingsTarget::TabletMode;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Tablo:
            return ESettingsTarget::Tablo;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Themes:
            return ESettingsTarget::Themes;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_UserAccount:
            return ESettingsTarget::UserAccount;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Volume:
            return ESettingsTarget::Volume;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_Vpn:
            return ESettingsTarget::Vpn;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_WiFi:
            return ESettingsTarget::WiFi;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_WinUpdate:
            return ESettingsTarget::WinUpdate;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_AddRemove:
            return ESettingsTarget::AddRemove;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_TaskManager:
            return ESettingsTarget::TaskManager;
        case NScenarios::TOpenSettingsDirective_ESettingsTarget_DeviceManager:
            return ESettingsTarget::DeviceManager;
        default:
            return ESettingsTarget::Accessibility;
    }
}

ESearchFilterLevel GetSearchFilterLevel(const NScenarios::TSetSearchFilterDirective::ESearchLevel level) {
    switch (level) {
        case NScenarios::TSetSearchFilterDirective_ESearchLevel_None:
            return ESearchFilterLevel::None;
        case NScenarios::TSetSearchFilterDirective_ESearchLevel_Strict:
            return ESearchFilterLevel::Strict;
        case NScenarios::TSetSearchFilterDirective_ESearchLevel_Moderate:
            return ESearchFilterLevel::Moderate;
        default:
            return ESearchFilterLevel::None;
    }
}

TString NavigateBrowserCommandToString(const NScenarios::TNavigateBrowserDirective_ECommand command) {
    switch (command) {
        case NScenarios::TNavigateBrowserDirective_ECommand_ClearHistory:
            return "clear_history";
        case NScenarios::TNavigateBrowserDirective_ECommand_CloseBrowser:
            return "close_browser";
        case NScenarios::TNavigateBrowserDirective_ECommand_GoHome:
            return "go_home";
        case NScenarios::TNavigateBrowserDirective_ECommand_NewTab:
            return "new_tab";
        case NScenarios::TNavigateBrowserDirective_ECommand_OpenBookmarksManager:
            return "open_bookmarks_manager";
        case NScenarios::TNavigateBrowserDirective_ECommand_OpenHistory:
            return "open_history";
        case NScenarios::TNavigateBrowserDirective_ECommand_OpenIncognitoMode:
            return "open_incognito_mode";
        case NScenarios::TNavigateBrowserDirective_ECommand_RestoreTab:
            return "restore_tab";
        case NScenarios::
            TNavigateBrowserDirective_ECommand_TNavigateBrowserDirective_ECommand_INT_MIN_SENTINEL_DO_NOT_USE_:
            break;
        case NScenarios::
            TNavigateBrowserDirective_ECommand_TNavigateBrowserDirective_ECommand_INT_MAX_SENTINEL_DO_NOT_USE_:
            break;
    }
    return {};
}

template <typename TPayload>
TIntrusivePtr<TUniversalClientDirectiveModel> MakeUniversalDirective(const google::protobuf::Message& directive,
                                                                     TPayload&& payload) {
    TMaybe<TString> multiroomSessionId = Nothing();
    if (DoesFieldExists(directive, "MultiroomSessionId") && !GetStringFieldValue(directive, "MultiroomSessionId").empty()) {
        multiroomSessionId = GetStringFieldValue(directive, "MultiroomSessionId");
    }

    TMaybe<TString> roomId = Nothing();
    if (DoesFieldExists(directive, "RoomId") && !GetStringFieldValue(directive, "RoomId").empty()) {
        roomId = GetStringFieldValue(directive, "RoomId");
    }
    TMaybe<NScenarios::TLocationInfo> locationInfo = GetMaybeLocationInfoFieldMessage(directive);

    return MakeIntrusive<TUniversalClientDirectiveModel>(GetMessageOptionValue(directive, NAlice::SpeechKitName),
                                                         GetStringFieldValue(directive, "Name"),
                                                         std::forward<TPayload>(payload),
                                                         multiroomSessionId,
                                                         roomId,
                                                         locationInfo);
}

void DropOuterFieldsFromPayload(const google::protobuf::Message& msg, TProtoStructBuilder& builder) {
    const auto* descr = msg.GetDescriptor();
    for (int fieldIdx = 0; fieldIdx < descr->field_count(); ++fieldIdx) {
        const auto* field = descr->field(fieldIdx);
        if (field->options().HasExtension(NScenarios::OuterField)) {
            builder.Drop(field->json_name());
        }
    }
}

void DropEmptyLocationFieldsFromPayload(google::protobuf::Struct& payloadStruct) {
    auto& payloadMap = *payloadStruct.mutable_fields();
    if (payloadMap.contains("room_id") && payloadMap.at("room_id").string_value().empty()) {
        payloadMap.erase("room_id");
    }
    if (payloadMap.contains("location_info") && payloadMap.at("location_info").has_null_value()) {
        payloadMap.erase("location_info");
    }
}

TString PackDeepLinkDirectives(const NJson::TJsonValue& directives) {
    TCgiParameters cgi;
    cgi.InsertUnescaped("directives", JsonToString(directives));
    auto cgiStr = cgi.Print();
    for (const auto [unescaped, escaped] : DEEP_LINK_CGI_ESCAPES) {
        SubstGlobal(cgiStr, unescaped, escaped);
    }
    return TString(TStringBuilder{} << DIALOG_ACTION_SCHEME << cgiStr);
}

TString SerializeDeepLinkDirectives(const TVector<TIntrusivePtr<IDirectiveModel>>& directiveModels,
                                    const TSpeechKitStructSerializer& structSerializer) {
    NJson::TJsonValue directives{NJson::JSON_ARRAY};
    for (const auto& directive : directiveModels) {
        directives.AppendValue(JsonFromProto(structSerializer.Serialize(*directive)));
    }
    return PackDeepLinkDirectives(directives);
}

TString JsonNameByField(const TString& name, const google::protobuf::Message& msg) {
    const auto* field = msg.GetDescriptor()->FindFieldByName(name);
    return field ? field->json_name() : name;
}

template <typename T>
TProtoStructBuilder RemindersDefaultPayloadBuilder(const T& directive,
                                                   const TScenarioProtoDeserializer& deserializer,
                                                   const TSerializerMeta& serializerMeta,
                                                   const TSpeechKitStructSerializer& structSerializer)
{
    static const auto fieldName = JsonNameByField("Name", directive);
    static const auto fieldOnSuccessCallback = JsonNameByField("OnSuccessCallback", directive);
    static const auto fieldOnFailCallback = JsonNameByField("OnFailCallback", directive);

    auto payload = MessageToStructBuilder(directive, serializerMeta.GetBuilderOptions())
        .Drop(fieldName)
        .Drop(fieldOnSuccessCallback)
        .Drop(fieldOnFailCallback);
    DropOuterFieldsFromPayload(directive, payload);

    payload.Set("scenario_name", serializerMeta.GetScenarioName());

    if (directive.HasOnSuccessCallback()) {
        if (const auto callback = deserializer.Deserialize(directive.GetOnSuccessCallback())) {
            payload.Set(fieldOnSuccessCallback, structSerializer.Serialize(*callback));
        }
    }
    if (directive.HasOnFailCallback()) {
        if (const auto callback = deserializer.Deserialize(directive.GetOnFailCallback())) {
            payload.Set(fieldOnFailCallback, structSerializer.Serialize(*callback));
        }
    }

    return payload;
}

const TString& GetPermissionsName(const NAlice::TPermissions::EValue permissions) {
    static const TString undefined = "undefined";
    static const TString location = "location";
    static const TString readContacts = "read_contacts";
    static const TString callPhone = "call_phone";
    static const TString scheduleExactAlarm = "schedule_exact_alarm";

    switch (permissions) {
        case NAlice::TPermissions::Location:
            return location;
        case NAlice::TPermissions::ReadContacts:
            return readContacts;
        case NAlice::TPermissions::CallPhone:
            return callPhone;
        case NAlice::TPermissions::ScheduleExactAlarm:
            return scheduleExactAlarm;
        case NAlice::TPermissions::Undefined:
        case NAlice::TPermissions_EValue_TPermissions_EValue_INT_MIN_SENTINEL_DO_NOT_USE_:
        case NAlice::TPermissions_EValue_TPermissions_EValue_INT_MAX_SENTINEL_DO_NOT_USE_:
            return undefined;
    }
}

google::protobuf::Struct BuildSemanticFrameRequestData(const TSerializerMeta& meta,
                                                       const TSemanticFrameRequestData& data)
{
    auto originPayload = TProtoStructBuilder()
        .Set("device_id", meta.GetClientInfo().DeviceId)
        .Set("uuid", meta.GetClientInfo().Uuid)
        .Build();

    auto semanticFrameRequestPayload = MessageToStructBuilder(data)
        .Set("origin", std::move(originPayload))
        .Build();

    return semanticFrameRequestPayload;
}

template<typename TDirective>
TProtoStructBuilder PushTypedSemanticFramePayloadBuilder(const TSerializerMeta& meta, const TDirective& directive) {
    auto originPayload = TProtoStructBuilder()
        .Set("device_id", meta.GetClientInfo().DeviceId)
        .Set("uuid", meta.GetClientInfo().Uuid)
        .Build();

    auto semanticFrameRequestPayload = MessageToStructBuilder(directive.GetSemanticFrameRequestData())
        .Set("origin", std::move(originPayload))
        .Build();

    auto payload = MessageToStructBuilder(directive, meta.GetBuilderOptions())
        .Set("semantic_frame_request_data", std::move(semanticFrameRequestPayload));
    return payload;
}

} // namespace

TScenarioProtoDeserializer::TScenarioProtoDeserializer(TSerializerMeta serializerMeta,
                                                       TRTLogger& logger,
                                                       THashMap<TString, NScenarios::TFrameAction>&& actions,
                                                       THashMap<TString, NScenarios::TActionSpace>&& actionSpaces,
                                                       TMaybe<TString>&& fillCloudUiDirectiveText)
    : SerializerMeta(std::move(serializerMeta))
    , Logger(logger)
    , Actions(std::move(actions))
    , ActionSpaces(std::move(actionSpaces))
    , FillCloudUiDirectiveText(std::move(fillCloudUiDirectiveText))
    , StructSerializer(SerializerMeta) {
}

TIntrusivePtr<IDirectiveModel> TScenarioProtoDeserializer::Deserialize(const NScenarios::TDirective& directive) const {
    if (auto directiveModel = TryDeserializeUniproxyDirective(directive); directiveModel) {
        return directiveModel;
    }
    if (auto directiveModel = TryDeserializeBaseDirective(directive); directiveModel) {
        return directiveModel;
    }

    LOG_ERR(Logger) << "Unhandled TDirective case: " << static_cast<int>(directive.GetDirectiveCase());
    return {};
}

TIntrusivePtr<TBaseDirectiveModel>
TScenarioProtoDeserializer::TryDeserializeBaseDirective(const NScenarios::TDirective& directive) const {
    auto directiveModel = TryDeserializeBaseDirectiveImpl(directive);

    if (directiveModel && directive.HasEndpointId()) {
        directiveModel->SetEndpointId(directive.GetEndpointId().value());;
    }

    return directiveModel;
}
TIntrusivePtr<TBaseDirectiveModel>
TScenarioProtoDeserializer::TryDeserializeBaseDirectiveImpl(const NScenarios::TDirective& directive) const {
    using NScenarios::TDirective;

    switch (directive.GetDirectiveCase()) {
        case TDirective::kCallbackDirective:
            return Deserialize(directive.GetCallbackDirective());
        case TDirective::kOpenDialogDirective:
            return Deserialize(directive.GetOpenDialogDirective());
        case TDirective::kUpdateDialogInfoDirective:
            return Deserialize(directive.GetUpdateDialogInfoDirective());
        case TDirective::kPlayerRewindDirective:
            return Deserialize(directive.GetPlayerRewindDirective());
        case TDirective::kSetCookiesDirective:
            return Deserialize(directive.GetSetCookiesDirective());
        case TDirective::kEndDialogSessionDirective:
            return Deserialize(directive.GetEndDialogSessionDirective());
        case TDirective::kMordoviaCommandDirective:
            return Deserialize(directive.GetMordoviaCommandDirective());
        case TDirective::kMordoviaShowDirective:
            return Deserialize(directive.GetMordoviaShowDirective());
        case TDirective::kMusicPlayDirective:
            return Deserialize(directive.GetMusicPlayDirective());
        case TDirective::kCloseDialogDirective:
            return Deserialize(directive.GetCloseDialogDirective());
        case TDirective::kThereminPlayDirective:
            return Deserialize(directive.GetThereminPlayDirective());
        case TDirective::kAlarmSetSoundDirective:
            return Deserialize(directive.GetAlarmSetSoundDirective());
        case TDirective::kAlarmNewDirective:
            return Deserialize(directive.GetAlarmNewDirective());
        case TDirective::kFindContactsDirective:
            return Deserialize(directive.GetFindContactsDirective());
        case TDirective::kOpenSettingsDirective:
            return Deserialize(directive.GetOpenSettingsDirective());
        case TDirective::kSetSearchFilterDirective:
            return Deserialize(directive.GetSetSearchFilterDirective());
        case TDirective::kSetTimerDirective:
            return Deserialize(directive.GetSetTimerDirective());
        case TDirective::kNavigateBrowserDirective:
            return DeserializeNavigateBrowserDirective(directive.GetNavigateBrowserDirective());
        case TDirective::kAudioPlayDirective:
            return Deserialize(directive.GetAudioPlayDirective());
        case TDirective::kAddCardDirective:
            return Deserialize(directive.GetAddCardDirective());
        case TDirective::kShowButtonsDirective:
            return Deserialize(directive.GetShowButtonsDirective());
        case TDirective::kRemindersSetDirective:
            return Deserialize(directive.GetRemindersSetDirective());
        case TDirective::kRemindersCancelDirective:
            return Deserialize(directive.GetRemindersCancelDirective());
        case TDirective::kRequestPermissionsDirective:
            return Deserialize(directive.GetRequestPermissionsDirective());
        case TDirective::kMultiroomSemanticFrameDirective:
            return Deserialize(directive.GetMultiroomSemanticFrameDirective());
        case TDirective::kStartMultiroomDirective:
            return Deserialize(directive.GetStartMultiroomDirective());
        case TDirective::DIRECTIVE_NOT_SET:
            break;
        default: {
            return DeserializeUniversalClientDirective(GetDirectiveCase(directive));
        }
    }

    return {};
}

TIntrusivePtr<TUniproxyDirectiveModel>
TScenarioProtoDeserializer::TryDeserializeUniproxyDirective(const NScenarios::TDirective& directive) const {
    using NScenarios::TDirective;

    switch (directive.GetDirectiveCase()) {
        case TDirective::kSaveVoiceprintDirective:
            return DeserializeUniversalUniproxyDirective(/* name= */ "save_voiceprint", directive);
        case TDirective::kRemoveVoiceprintDirective:
            return DeserializeUniversalUniproxyDirective(/* name= */ "remove_voiceprint", directive);
        case TDirective::kUpdateNotificationSubscriptionDirective:
            return DeserializeUniversalUniproxyDirective(/* name= */ "update_notification_subscription", directive);
        case TDirective::kMarkNotificationAsReadDirective:
            return DeserializeUniversalUniproxyDirective(/* name= */ "mark_notification_as_read", directive);
        default:
            return {};
    }
    return {};
}

TIntrusivePtr<IDirectiveModel> TScenarioProtoDeserializer::Deserialize(const NScenarios::TServerDirective& directive) const {
    TIntrusivePtr<IDirectiveModel> directiveModel = DeserializeServerDirective(directive);

    if (auto uniproxyDirectiveMeta = TryConstructUniproxyDirectiveMeta(directive)) {
        if (auto* uniproxyDirectiveModel = dynamic_cast<TUniproxyDirectiveModel*>(directiveModel.Get())) {
            uniproxyDirectiveModel->SetUniproxyDirectiveMeta(std::move(*uniproxyDirectiveMeta));
        } else if (auto* protobufUniproxyDirectiveModel = dynamic_cast<TProtobufUniproxyDirectiveModel*>(directiveModel.Get())) {
            protobufUniproxyDirectiveModel->SetUniproxyDirectiveMeta(std::move(*uniproxyDirectiveMeta));
        }
    }

    return directiveModel;
}

TIntrusivePtr<TAlarmNewDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TAlarmNewDirective& directive) const {
    return MakeIntrusive<TAlarmNewDirectiveModel>(directive.GetName(), directive.GetState(),
                                                  directive.GetOnSuccessCallbackPayload(),
                                                  directive.GetOnFailureCallbackPayload());
}

TIntrusivePtr<TAlarmSetSoundDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TAlarmSetSoundDirective& directive) const {
    return MakeIntrusive<TAlarmSetSoundDirectiveModel>(
        directive.GetName(), std::move(*Deserialize(directive.GetCallback())), directive.GetSettings());
}

TIntrusivePtr<TOpenDialogDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TOpenDialogDirective& directive) const {
    TVector<TIntrusivePtr<IDirectiveModel>> directives;
    for (const auto& innerDirective : directive.GetDirectives()) {
        if (auto converted = Deserialize(innerDirective); converted) {
            directives.push_back(converted);
        }
    }
    return MakeIntrusive<TOpenDialogDirectiveModel>(directive.GetName(), directive.GetDialogId(), directives);
}

TIntrusivePtr<TOpenSettingsDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TOpenSettingsDirective& directive) const {
    return MakeIntrusive<TOpenSettingsDirectiveModel>(directive.GetName(), GetModelsEnum(directive.GetTarget()));
}

TIntrusivePtr<TUpdateDialogInfoDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TUpdateDialogInfoDirective& directive) const {
    TVector<TUpdateDialogInfoDirectiveMenuItemModel> menuItems;
    for (const auto& menuItem : directive.GetMenuItems()) {
        menuItems.push_back(Deserialize(menuItem));
    }
    return MakeIntrusive<TUpdateDialogInfoDirectiveModel>(
        directive.GetName(), directive.GetTitle(), directive.GetUrl(), directive.GetImageUrl(),
        Deserialize(directive.GetStyle()), Deserialize(directive.GetDarkStyle()), menuItems, directive.GetAdBlockId());
}

TIntrusivePtr<TShowButtonsDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TShowButtonsDirective& directive) const {
    TMaybe<TString> screenId;
    if (directive.HasScreenId()) {
        screenId.ConstructInPlace(directive.GetScreenId());
    }

    TVector<TIntrusivePtr<IButtonModel>> buttons;
    for (const auto& button : directive.GetButtons()) {
        buttons.push_back(Deserialize(button));
    }

    return MakeIntrusive<TShowButtonsDirectiveModel>(
        directive.GetName(), std::move(buttons), std::move(screenId));
}

TIntrusivePtr<TUniversalClientDirectiveModel> TScenarioProtoDeserializer::Deserialize(
    const NScenarios::TRemindersSetDirective& directive) const
{
    static const auto fieldOnShootFrame = JsonNameByField("OnShootFrame", directive);
    static const auto fieldText = JsonNameByField("Text", directive);
    static const auto fieldEpoch = JsonNameByField("Epoch", directive);
    static const auto fieldId = JsonNameByField("Id", directive);
    static const auto fieldTimeZone = JsonNameByField("TimeZone", directive);

    auto payload = RemindersDefaultPayloadBuilder(directive, *this, SerializerMeta, StructSerializer)
        .Drop(fieldOnShootFrame);

    payload.Set(fieldText, directive.GetText());
    payload.Set(fieldEpoch, ToString(directive.GetEpoch()));
    payload.Set(fieldId, directive.GetId());
    payload.Set(fieldTimeZone, directive.GetTimeZone());

    if (directive.HasOnShootFrame()) {
        if (const auto frame = Deserialize(directive.GetOnShootFrame())) {
            payload.Set(fieldOnShootFrame, StructSerializer.Serialize(*frame));
        }
    }

    return MakeUniversalDirective(directive, payload.Build());
}

TIntrusivePtr<TUniversalClientDirectiveModel> TScenarioProtoDeserializer::Deserialize(
    const NScenarios::TRemindersCancelDirective& directive) const
{
    static const auto fieldId = JsonNameByField("Id", directive);
    static const auto fieldAction = JsonNameByField("Action", directive);

    auto payload = RemindersDefaultPayloadBuilder(directive, *this, SerializerMeta, StructSerializer);
    TProtoListBuilder idsBuilder;
    for (const auto& id : directive.GetIds()) {
        idsBuilder.Add(id);
    }
    payload.Set(fieldId, idsBuilder.Build());
    payload.Set(fieldAction, directive.GetAction());

    return MakeUniversalDirective(directive, payload.Build());
}

TIntrusivePtr<TTypedSemanticFrameRequestDirectiveModel> TScenarioProtoDeserializer::Deserialize(
    const TSemanticFrameRequestData& frame) const
{
    return MakeIntrusive<TTypedSemanticFrameRequestDirectiveModel>(frame);
}

TIntrusivePtr<TUniversalClientDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TRequestPermissionsDirective& directive) const {
    static const auto fieldName = JsonNameByField("Name", directive);
    static const auto fieldPermissions = JsonNameByField("Permissions", directive);
    static const auto fieldOnSuccess = JsonNameByField("OnSuccess", directive);
    static const auto fieldOnFail = JsonNameByField("OnFail", directive);

    TProtoListBuilder permissionsList;
    for (const auto& permission : directive.GetPermissions()) {
        permissionsList.Add(GetPermissionsName(
            static_cast<NAlice::TPermissions::EValue>(permission)
        ));
    }

    auto payload = MessageToStructBuilder(directive, SerializerMeta.GetBuilderOptions())
        .Drop(fieldName)
        .Set(fieldPermissions, permissionsList.Build());
    const auto addSubDirective = [&](const TString& field, const NScenarios::TDirective& subDirective) {
        if (const auto deserializedSubDirective = Deserialize(subDirective)) {
            payload.Set(
                field,
                TProtoListBuilder()
                    .Add(StructSerializer.Serialize(
                        *deserializedSubDirective
                    ))
                    .Build()
            );
        }
    };
    if (directive.HasOnSuccess()) {
        addSubDirective(fieldOnSuccess, directive.GetOnSuccess());
    }
    if (directive.HasOnFail()) {
        addSubDirective(fieldOnFail, directive.GetOnFail());
    }

    DropOuterFieldsFromPayload(directive, payload);

    return MakeUniversalDirective(directive, payload.Build());
}

TIntrusivePtr<TUniversalClientDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TMultiroomSemanticFrameDirective& directive) const {
    auto semanticFrameRequestDataPayload = BuildSemanticFrameRequestData(SerializerMeta, directive.GetBody());

    TProtoStructBuilder body;
    body.Set("name", TString{SEMANTIC_FRAME_REQUEST_NAME});
    body.Set("payload", MessageToStruct(semanticFrameRequestDataPayload, SerializerMeta.GetBuilderOptions()));
    body.Set("type", "server_action");

    TProtoStructBuilder payload;
    payload.Set("body", body.Build());

    NScenarios::TLocationInfo locationInfo;
    *locationInfo.AddDevicesIds() = directive.GetDeviceId();

    return MakeIntrusive<TUniversalClientDirectiveModel>(GetMessageOptionValue(directive, NAlice::SpeechKitName),
                                                         GetStringFieldValue(directive, "Name"),
                                                         payload.Build(),
                                                         /* multiroomSessionId = */ Nothing(),
                                                         /* roomId = */ directive.GetDeviceId(),
                                                         /* locationInfo = */ locationInfo);
}

TIntrusivePtr<TUniversalClientDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TStartMultiroomDirective& directive) const {
    TProtoStructBuilder payload;
    TMaybe<TString> roomIdMaybe;

    if (directive.HasLocationInfo()) {
        const auto& locationInfo = directive.GetLocationInfo();
        if (const auto* ioTUserInfo = SerializerMeta.GetIoTUserInfo().Get()) {
            TProtoListBuilder listBuilder;
            const auto onDeviceId = [&listBuilder](const TString& deviceId) {
                listBuilder.Add(deviceId);
            };
            ForEachQuasarDeviceIdInLocation(ioTUserInfo->GetDevices(), locationInfo, onDeviceId,
                                            SerializerMeta.GetClientInfo().DeviceId);
            payload.Set(ROOM_DEVICE_IDS, listBuilder.Build());
            payload.Set(ROOM_ID, TString{CollectLocationId(locationInfo)});
        }
    } else if (directive.HasRoomId()) {
        const TString& roomId = directive.GetRoomId();
        payload.Set(ROOM_ID, roomId);
        roomIdMaybe = roomId;
    }

    if (const auto& token = directive.GetMultiroomToken(); !token.Empty()) {
        payload.Set(MULTIROOM_TOKEN, token);
    }

    return MakeIntrusive<TUniversalClientDirectiveModel>(GetMessageOptionValue(directive, NAlice::SpeechKitName),
                                                         GetStringFieldValue(directive, "Name"),
                                                         payload.Build(),
                                                         /* multiroomSessionId = */ Nothing(),
                                                         /* roomId = */ std::move(roomIdMaybe));
}

TActionButtonModel::TTheme
TScenarioProtoDeserializer::Deserialize(const NScenarios::TLayout_TButton_TTheme& theme) const {
    return TActionButtonModel::TTheme(theme.GetImageUrl());
}

TActionButtonModel::TTheme
TScenarioProtoDeserializer::Deserialize(const NScenarios::TLayout_TSuggest_TActionButton_TTheme& theme) const {
    return TActionButtonModel::TTheme(theme.GetImageUrl());
}

TActionButtonModel::TTheme
TScenarioProtoDeserializer::Deserialize(const NScenarios::TButton_TTheme& theme) const {
    return TActionButtonModel::TTheme(theme.GetImageUrl());
}

TUpdateDialogInfoDirectiveMenuItemModel
TScenarioProtoDeserializer::Deserialize(const NScenarios::TUpdateDialogInfoDirective_TMenuItem& menuItem) const {
    return TUpdateDialogInfoDirectiveMenuItemModel(menuItem.GetTitle(), menuItem.GetUrl());
}

TUpdateDialogInfoDirectiveStyleModel
TScenarioProtoDeserializer::Deserialize(const NScenarios::TUpdateDialogInfoDirective_TStyle& style) const {
    TUpdateDialogInfoDirectiveStyleModelBuilder builder{};
    builder.SetSuggestBorderColor(style.GetSuggestBorderColor())
        .SetUserBubbleFillColor(style.GetUserBubbleFillColor())
        .SetSuggestTextColor(style.GetSuggestTextColor())
        .SetSuggestFillColor(style.GetSuggestFillColor())
        .SetUserBubbleTextColor(style.GetUserBubbleTextColor())
        .SetSkillActionsTextColor(style.GetSkillActionsTextColor())
        .SetSkillBubbleFillColor(style.GetSkillBubbleFillColor())
        .SetSkillBubbleTextColor(style.GetSkillBubbleTextColor())
        .SetOknyxLogo(style.GetOknyxLogo());

    for (const auto& oknyxErrorColor : style.GetOknyxErrorColors()) {
        builder.AddOknyxErrorColor(oknyxErrorColor);
    }

    for (const auto& oknyxNormalColors : style.GetOknyxNormalColors()) {
        builder.AddOknyxNormalColor(oknyxNormalColors);
    }

    return builder.Build();
}

TIntrusivePtr<TPlayerRewindDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TPlayerRewindDirective& directive) const {
    return MakeIntrusive<TPlayerRewindDirectiveModel>(directive.GetName(), directive.GetAmount(),
                                                      GetRewindType(directive.GetType()));
}

TIntrusivePtr<TSetSearchFilterDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TSetSearchFilterDirective& directive) const {
    return MakeIntrusive<TSetSearchFilterDirectiveModel>(directive.GetName(),
                                                         GetSearchFilterLevel(directive.GetLevel()));
}

TIntrusivePtr<TSetTimerDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TSetTimerDirective& directive) const {
    TVector<TIntrusivePtr<IDirectiveModel>> directives{};
    for (const auto& innerDirective : directive.GetDirectives()) {
        if (auto converted = Deserialize(innerDirective); converted) {
            directives.push_back(converted);
        }
    }
    return MakeIntrusive<TSetTimerDirectiveModel>(
        directive.GetName(), directive.GetDuration(), directive.GetListeningIsPossible(), directive.GetTimestamp(),
        directive.GetOnSuccessCallbackPayload(), directive.GetOnFailureCallbackPayload(), std::move(directives));
}

TIntrusivePtr<TSetCookiesDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TSetCookiesDirective& directive) const {
    return MakeIntrusive<TSetCookiesDirectiveModel>(directive.GetName(), directive.GetValue());
}

TIntrusivePtr<TEndDialogSessionDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TEndDialogSessionDirective& directive) const {
    return MakeIntrusive<TEndDialogSessionDirectiveModel>(directive.GetName(), directive.GetDialogId());
}

TIntrusivePtr<TFindContactsDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TFindContactsDirective& directive) const {
    TVector<TFindContactsDirectiveModel::TRequestPart> requestParts;
    for (const auto& part : directive.GetRequest()) {
        requestParts.emplace_back(part.GetTag());
        for (const auto& value : part.GetValues()) {
            requestParts.back().AddValue(value);
        }
    }
    const auto& whitelist = directive.GetMimeTypesWhitelist();
    return MakeIntrusive<TFindContactsDirectiveModel>(
        directive.GetName(), TVector<TString>(whitelist.GetColumn().begin(), whitelist.GetColumn().end()),
        TVector<TString>(whitelist.GetName().begin(), whitelist.GetName().end()),
        directive.GetOnPermissionDeniedCallbackPayload(), std::move(requestParts),
        TVector<TString>(directive.GetValues().begin(), directive.GetValues().end()), directive.GetCallbackName());
}

TIntrusivePtr<TUniversalClientDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TMordoviaCommandDirective& directive) const {
    auto payload = MessageToStructBuilder(directive, SerializerMeta.GetBuilderOptions())
        .Drop("name")
        .Drop("meta");

    DropOuterFieldsFromPayload(directive, payload);
    payload.Set("meta", JsonStringFromProto(directive.GetMeta()));
    const auto& scenarioName = SerializerMeta.GetScenarioName();
    payload.Set("scenario_name", scenarioName);
    payload.Set("scenario", directive.GetViewKey().Contains(':') ?
                                directive.GetViewKey() :
                                TString::Join(scenarioName, ':', directive.GetViewKey()));
    payload.Set("view_key", directive.GetViewKey());
    return MakeUniversalDirective(directive, payload.Build());
}

TIntrusivePtr<TUniversalClientDirectiveModel>
TScenarioProtoDeserializer::Deserialize(NScenarios::TAddCardDirective directive) const {
    const auto& spaceId = directive.GetActionSpaceId();
    const auto* actionSpace = ActionSpaces.FindPtr(spaceId);
    if (!actionSpace) {
        LOG_ERR(Logger) << "Unable to find corresponding action space for id: " << spaceId;
    } else {
        const auto& actions = actionSpace->GetActions();
        const auto deepLinkCreator = [&](const TString& actionId) {
            if (!actions.contains(actionId)) {
                return TString{};
            }
            return SerializeDeepLinkDirectives(GenerateActionSpaceDirectives(actionId, "DeepLink", actions),
                                               StructSerializer);
        };
        InflateDeepLinks(*directive.MutableDiv2Card()->MutableBody(), deepLinkCreator);
        InflateDeepLinks(*directive.MutableDiv2Templates(), deepLinkCreator);
    }
    return DeserializeUniversalClientDirective(directive);
}

TIntrusivePtr<IDirectiveModel> TScenarioProtoDeserializer::DeserializeServerDirective(
    const NScenarios::TServerDirective& directive) const
{
    using NScenarios::TServerDirective;

    switch (directive.GetDirectiveCase()) {
        case TServerDirective::kUpdateDatasyncDirective:
            return Deserialize(directive.GetUpdateDatasyncDirective());
        case TServerDirective::kPushMessageDirective:
            return DeserializePushUniproxyDirective(/* name= */ "push_message", directive);
        case TServerDirective::kPersonalCardsDirective:
            return DeserializeUniversalUniproxyDirective(/* name= */ "personal_cards", directive);
        case TServerDirective::kMementoChangeUserObjectsDirective:
            return DeserializeMementoChangeUserObjectsDirective(directive.GetMementoChangeUserObjectsDirective());
        case TServerDirective::kSendPushDirective:
            return DeserializeUniversalUniproxyDirective(/* name= */ "send_push_directive", directive);
        case TServerDirective::kDeletePushesDirective:
            return DeserializeUniversalUniproxyDirective(/* name= */ "delete_pushes_directive", directive);
        case TServerDirective::kPushTypedSemanticFrameDirective:
            return DeserializePushTypedSemanticFrameDirective(directive.GetPushTypedSemanticFrameDirective());
        case TServerDirective::kAddScheduleActionDirective:
            return DeserializeUniversalUniproxyDirective(/* name= */ "add_schedule_action", directive);
        case TServerDirective::kSaveUserAudioDirective:
            return DeserializeUniversalUniproxyDirective(/* name= */ "save_user_audio", directive);
        case TServerDirective::kPatchAsrOptionsForNextRequestDirective:
            return DeserializeUniversalUniproxyDirective(/* name= */ "patch_asr_options_for_next_request", directive);
        case TServerDirective::kCancelScheduledActionDirective:
            return Deserialize(directive.GetCancelScheduledActionDirective());
        case TServerDirective::kEnlistScheduledActionDirective:
            return Deserialize(directive.GetEnlistScheduledActionDirective());
        case TServerDirective::DIRECTIVE_NOT_SET:
            LOG_ERR(Logger) << "Unhandled TServerDirective case: " << static_cast<int>(directive.GetDirectiveCase());
            return {};
    }
}

TMaybe<NSpeechKit::TUniproxyDirectiveMeta> TScenarioProtoDeserializer::TryConstructUniproxyDirectiveMeta(
    const NScenarios::TServerDirective& directive) const
{
    TMaybe<NSpeechKit::TUniproxyDirectiveMeta> uniproxyDirectiveMeta;
    if (directive.HasMeta()) {
        TString puid;
        switch (directive.GetMeta().GetApplyFor()) {
            case NScenarios::TServerDirective::TMeta::EApplyFor::TServerDirective_TMeta_EApplyFor_DeviceOwner:
                puid = SerializerMeta.GetAdditionalOptions().GetPuid();
                break;
            case NScenarios::TServerDirective::TMeta::EApplyFor::TServerDirective_TMeta_EApplyFor_CurrentUser:
                puid = SerializerMeta.GetAdditionalOptions().GetGuestUserOptions().GetYandexUID();
                break;
            default:
                break;
        }
        if (puid) {
            uniproxyDirectiveMeta.ConstructInPlace().SetPuid(puid.data(), puid.size());
        }
    }
    return uniproxyDirectiveMeta;
}

TIntrusivePtr<TProtobufUniproxyDirectiveModel> TScenarioProtoDeserializer::Deserialize(
    const NScenarios::TCancelScheduledActionDirective& directive) const
{
    return MakeIntrusive<TProtobufUniproxyDirectiveModel>(directive);
}

TIntrusivePtr<TProtobufUniproxyDirectiveModel> TScenarioProtoDeserializer::Deserialize(
    const NScenarios::TEnlistScheduledActionDirective& directive) const
{
    return MakeIntrusive<TProtobufUniproxyDirectiveModel>(directive);
}

TIntrusivePtr<TUniversalClientDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TMordoviaShowDirective& directive) const {
    auto payload = MessageToStructBuilder(directive, SerializerMeta.GetBuilderOptions())
                       .Drop("name")
                       .Drop("callback_prototype");
    DropOuterFieldsFromPayload(directive, payload);
    if (directive.HasCallbackPrototype()) {
        if (const auto callback = Deserialize(directive.GetCallbackPrototype())) {
            payload.Set("callback_prototype", StructSerializer.Serialize(*callback));
        }
    }
    const auto& scenarioName = SerializerMeta.GetScenarioName();
    payload.Set("scenario_name", scenarioName);
    payload.Set("scenario", directive.GetViewKey().Contains(':') ?
                            directive.GetViewKey() :
                            TString::Join(scenarioName, ':', directive.GetViewKey()));
    payload.Set("view_key", directive.GetViewKey());
    return MakeUniversalDirective(directive, payload.Build());
}

TIntrusivePtr<TMusicPlayDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TMusicPlayDirective& directive) const {
    return MakeIntrusive<TMusicPlayDirectiveModel>(
        directive.GetName(), directive.GetUid(), directive.GetSessionId(), directive.GetOffset(),
        directive.GetAlarmId().Empty() ? Nothing() : TMaybe<TString>{directive.GetAlarmId()},
        directive.GetFirstTrackId().Empty() ? Nothing() : TMaybe<TString>{directive.GetFirstTrackId()},
        directive.GetRoomId().Empty() ? Nothing() : TMaybe<TString>{directive.GetRoomId()},
        directive.HasLocationInfo() ? TMaybe<NScenarios::TLocationInfo>(directive.GetLocationInfo()) : Nothing()
    );
}

TIntrusivePtr<TCloseDialogDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TCloseDialogDirective& directive) const {
    TMaybe<TString> screenId;
    if (directive.HasScreenId()) {
        screenId.ConstructInPlace(directive.GetScreenId());
    }
    return MakeIntrusive<TCloseDialogDirectiveModel>(directive.GetName(), directive.GetDialogId(),
                                                     std::move(screenId));
}

TIntrusivePtr<TCallbackDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TCallbackDirective& directive) const {
    return MakeIntrusive<TCallbackDirectiveModel>(directive.GetName(), directive.GetIgnoreAnswer(),
                                                  directive.GetPayload(), directive.GetIsLedSilent());
}

TIntrusivePtr<TThereminPlayDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TThereminPlayDirective& directive) const {
    using NScenarios::TThereminPlayDirective;
    TIntrusivePtr<IThereminPlayDirectiveSetModel> set;
    switch (directive.GetTSetCase()) {
        case TThereminPlayDirective::kInternalSet: {
            const auto& internalSet = directive.GetInternalSet();
            set = MakeIntrusive<TThereminPlayDirectiveInternalSetModel>(internalSet.GetMode());
            break;
        }
        case TThereminPlayDirective::kExternalSet: {
            const auto& externalSet = directive.GetExternalSet();
            TVector<TThereminPlayDirectiveExternalSetSampleModel> samples{};
            for (const auto& sample : externalSet.GetSamples()) {
                samples.push_back(TThereminPlayDirectiveExternalSetSampleModel(sample.GetUrl()));
            }
            set = MakeIntrusive<TThereminPlayDirectiveExternalSetModel>(externalSet.GetNoOverlaySamples(),
                                                                        externalSet.GetRepeatSoundInside(),
                                                                        externalSet.GetStopOnCeil(), samples);
            break;
        }
        case TThereminPlayDirective::TSET_NOT_SET:
            break;
    }
    if (set) {
        return MakeIntrusive<TThereminPlayDirectiveModel>(directive.GetName(), set);
    }

    LOG_ERR(Logger) << "Unhandled TSet case: " << static_cast<int>(directive.GetTSetCase());
    return {};
}

// static
TIntrusivePtr<ICardModel> TScenarioProtoDeserializer::Deserialize(const NScenarios::TLayout_TCard& card) const {
    using NScenarios::TLayout_TCard;
    switch (card.GetCardCase()) {
        case TLayout_TCard::kText:
            return MakeIntrusive<TTextCardModel>(card.GetText());
        case TLayout_TCard::kTextWithButtons:
            return Deserialize(card.GetTextWithButtons());
        case TLayout_TCard::kDivCard:
            return DeserializeDivCard(card.GetDivCard());
        case TLayout_TCard::kDiv2Card:
            return DeserializeDiv2Card(card.GetDiv2Card(), /* hideBorders= */ false, {});
        case TLayout_TCard::kDiv2CardExtended:
            return DeserializeDiv2Card(card.GetDiv2CardExtended().GetBody(),
                                       card.GetDiv2CardExtended().GetHideBorders(),
                                       card.GetDiv2CardExtended().GetText());
        case TLayout_TCard::kDiv2CardExtendedNew:
            return DeserializeDiv2Card(card.GetDiv2CardExtendedNew().GetBody(),
                                       card.GetDiv2CardExtendedNew().GetHideBorders(),
                                       card.GetDiv2CardExtended().GetText());
        case TLayout_TCard::CARD_NOT_SET:
            break;
    }

    LOG_ERR(Logger) << "Unhandled Card case: " << static_cast<int>(card.GetCardCase());
    return {};
}

TIntrusivePtr<TTextWithButtonCardModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TLayout_TTextWithButtons& card) const {
    TVector<TIntrusivePtr<IButtonModel>> buttons;
    for (const auto& button : card.GetButtons()) {
        buttons.push_back(Deserialize(button));
    }
    return MakeIntrusive<TTextWithButtonCardModel>(card.GetText(), buttons);
}

TIntrusivePtr<IButtonModel> TScenarioProtoDeserializer::Deserialize(const NScenarios::TLayout_TButton& button,
                                                                    const bool fromSuggest) const {
    TMaybe<TActionButtonModel::TTheme> theme;
    if (fromSuggest && button.HasTheme()) {
        theme = Deserialize(button.GetTheme());
    }
    return MakeIntrusive<TActionButtonModel>(button.GetTitle(),
                                             GenerateDirectives(button.GetActionId(), button.GetTitle()),
                                             theme);
}

TIntrusivePtr<IButtonModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TLayout_TSuggest& suggest) const {
    switch (suggest.GetActionCase()) {
        case NScenarios::TLayout_TSuggest::kActionButton: {
            const auto& button = suggest.GetActionButton();
            TMaybe<TActionButtonModel::TTheme> theme;
            if (button.HasTheme()) {
                theme = Deserialize(button.GetTheme());
            }
            return MakeIntrusive<TActionButtonModel>(button.GetTitle(),
                                                     GenerateDirectives(button.GetActionId(), button.GetTitle()),
                                                     theme);
        }
        case NScenarios::TLayout_TSuggest::kSearchButton:
            return DeserializeSearchButton(suggest.GetSearchButton());
        case NScenarios::TLayout_TSuggest::ACTION_NOT_SET:
            return {};
    }
}

TIntrusivePtr<IButtonModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TButton& button) const {
    TVector<TIntrusivePtr<IDirectiveModel>> directives;
    if (button.HasActionId()) {
        directives = GenerateDirectives(button.GetActionId());
    }

    TMaybe<TActionButtonModel::TTheme> theme;
    if (button.HasTheme()) {
        theme = Deserialize(button.GetTheme());
    }

    TMaybe<TString> text;
    if (button.HasText()) {
        text = button.GetText();
    }

    return MakeIntrusive<TActionButtonModel>(button.GetTitle(), std::move(directives), std::move(theme), std::move(text));
}

TIntrusivePtr<IButtonModel>
TScenarioProtoDeserializer::DeserializeSearchButton(const NScenarios::TLayout_TSuggest_TSearchButton& button) const {
    return MakeIntrusive<TActionButtonModel>(
        TString::Join("🔍 \"", button.GetTitle(), '"'),
        TVector<TIntrusivePtr<IDirectiveModel>>{{MakeIntrusive<TSearchCallbackDirectiveModel>(button.GetQuery()),
                                                 GenerateOnSuggestDirective(SerializerMeta, button.GetTitle())}});
}

TIntrusivePtr<ICardModel> TScenarioProtoDeserializer::DeserializeDiv2Card(google::protobuf::Struct body,
                                                                          bool hideBorders,
                                                                          TString text) const {
    CreateDeepLinks(body);
    return MakeIntrusive<TDiv2CardModel>(std::move(body), !hideBorders, text);
}

void TScenarioProtoDeserializer::CreateDeepLinks(google::protobuf::Struct& body, const bool addOnSuggestDirective) const {
    InflateDeepLinks(body, /* deepLinkCreator= */ [&](const TString& actionId) {
        if (!Actions.contains(actionId)) {
            return TString{};
        }
        return SerializeDeepLinkDirectives(
            GenerateDirectives(actionId, addOnSuggestDirective
                                             ? TMaybe<TString>("DeepLink")
                                             : Nothing()),
            StructSerializer);
    });
}

TIntrusivePtr<ICardModel> TScenarioProtoDeserializer::DeserializeDivCard(google::protobuf::Struct body) const {
    CreateDeepLinks(body);
    return MakeIntrusive<TDivCardModel>(std::move(body), FillCloudUiDirectiveText);
}

TVector<TIntrusivePtr<IDirectiveModel>> TScenarioProtoDeserializer::GenerateDirectives(const TString& actionId,
                                                                                       const TMaybe<TString>& suggestTitle) const {
    const auto tryGetTypedFrameFromParsedUtterance = [](const NScenarios::TParsedUtterance& parsedUtterance) -> TMaybe<TTypedSemanticFrame> {
        if (parsedUtterance.HasTypedSemanticFrame()) {
            return parsedUtterance.GetTypedSemanticFrame();
        }
        if (const auto frame = TryMakeTypedSemanticFrameFromSemanticFrame(parsedUtterance.GetFrame()); frame.Defined()) {
            return *frame;
        }
        return Nothing();
    };
    TVector<TIntrusivePtr<IDirectiveModel>> directives;
    if (const auto* action = Actions.FindPtr(actionId)) {
        switch (action->GetEffectCase()) {
            case NScenarios::TFrameAction::EffectCase::kDirectives:
                for (const auto& directive : action->GetDirectives().GetList()) {
                    if (auto converted = Deserialize(directive); converted) {
                        directives.push_back(converted);
                    }
                }
                break;
            case NScenarios::TFrameAction::EffectCase::kCallback:
                if (auto converted = Deserialize(action->GetCallback()); converted) {
                    directives.push_back(converted);
                }
                break;
            case NScenarios::TFrameAction::EffectCase::kFrame:
                if (const auto frame = TryMakeTypedSemanticFrameFromSemanticFrame(action->GetFrame())) {
                    directives.push_back(
                        MakeIntrusive<TTypedSemanticFrameRequestDirectiveModel>(*frame, TAnalyticsTrackingModule{}, /* params= */ Nothing(),
                                                                                /* requestParams= */ Nothing()));
                }
                break;
            case NScenarios::TFrameAction::EffectCase::kParsedUtterance: {
                if (const auto frame = tryGetTypedFrameFromParsedUtterance(action->GetParsedUtterance()); frame.Defined()) {
                    directives.push_back(
                        MakeIntrusive<TTypedSemanticFrameRequestDirectiveModel>(*frame, action->GetParsedUtterance().GetAnalytics(),
                                                                                action->GetParsedUtterance().HasParams()
                                                                                    ? TMaybe<TFrameRequestParams>(action->GetParsedUtterance().GetParams())
                                                                                    : Nothing(),
                                                                                action->GetParsedUtterance().HasRequestParams()
                                                                                    ? TMaybe<TRequestParams>(action->GetParsedUtterance().GetRequestParams())
                                                                                    : Nothing(),
                                                                                action->GetParsedUtterance().GetUtterance()));
                }
                break;
            }
            case NScenarios::TFrameAction::EFFECT_NOT_SET:
                LOG_ERR(Logger) << "Directive case is not set for action with id: " << actionId;
                break;
        }
    }
    // BROWSER-117931
    if (suggestTitle.Defined()) {
        directives.push_back(GenerateOnSuggestDirective(SerializerMeta, *suggestTitle));
    }
    return directives;
}

TVector<TIntrusivePtr<IDirectiveModel>> TScenarioProtoDeserializer::GenerateActionSpaceDirectives(
    const TString& actionId, const TString& onSuggestCaption,
    const google::protobuf::Map<TString, NScenarios::TActionSpace::TAction>& actions) const {
    TVector<TIntrusivePtr<IDirectiveModel>> directives{};
    Y_DEFER {
        // BROWSER-117931
        directives.push_back(GenerateOnSuggestDirective(SerializerMeta, onSuggestCaption));
    };
    if (!actions.contains(actionId)) {
        LOG_ERR(Logger) << "Unable to find action among provided actions on id: " << actionId;
        return directives;
    }
    const auto& action = actions.at(actionId);
    switch (action.GetEffectCase()) {
        case NScenarios::TActionSpace_TAction::kSemanticFrame:
            // TODO: add analytics verification
            directives.push_back(MakeIntrusive<TTypedSemanticFrameRequestDirectiveModel>(action.GetSemanticFrame()));
            break;
        case NScenarios::TActionSpace_TAction::EFFECT_NOT_SET:
            LOG_ERR(Logger) << "Directive case is not set for action with id: " << actionId;
            break;
    }
    return directives;
}

TIntrusivePtr<TUniversalClientDirectiveModel>
TScenarioProtoDeserializer::DeserializeUniversalClientDirective(const google::protobuf::Message& directive) const {
    auto payload = MessageToStructBuilder(directive, SerializerMeta.GetBuilderOptions())
        .Drop("name");
    DropOuterFieldsFromPayload(directive, payload);
    auto payloadStruct = payload.Build();
    DropEmptyLocationFieldsFromPayload(payloadStruct);
    CreateDeepLinks(payloadStruct, /* addOnSuggestDirective= */ !SerializerMeta.GetClientInfo().IsCentaur());
    return MakeUniversalDirective(directive, std::move(payloadStruct));
}

TIntrusivePtr<TUniversalUniproxyDirectiveModel>
TScenarioProtoDeserializer::DeserializeUniversalUniproxyDirective(const TString& name,
                                                                  const NScenarios::TDirective& directive) const {
    return MakeIntrusive<TUniversalUniproxyDirectiveModel>(
        name, /* payload= */ MessageToStructBuilder(GetDirectiveCase(directive), SerializerMeta.GetBuilderOptions())
                  .Build());
}

TIntrusivePtr<TUniversalUniproxyDirectiveModel>
TScenarioProtoDeserializer::DeserializeUniversalUniproxyDirective(const TString& name,
                                                                  const NScenarios::TServerDirective& directive) const {
    return MakeIntrusive<TUniversalUniproxyDirectiveModel>(
        name, /* payload= */ MessageToStructBuilder(GetDirectiveCase(directive), SerializerMeta.GetBuilderOptions())
            .Build());
}

TIntrusivePtr<TUniversalUniproxyDirectiveModel>
TScenarioProtoDeserializer::DeserializePushUniproxyDirective(const TString& name,
                                                             const NScenarios::TServerDirective& directive) const {
    auto body = MessageToStructBuilder(GetDirectiveCase(directive), SerializerMeta.GetBuilderOptions()).Build();
    CreateDeepLinks(body);
    return MakeIntrusive<TUniversalUniproxyDirectiveModel>(name, /* payload= */ body);
}

TIntrusivePtr<TUniversalUniproxyDirectiveModel>
TScenarioProtoDeserializer::DeserializePushTypedSemanticFrameDirective(const NScenarios::TPushTypedSemanticFrameDirective& directive) const {
    static const TString name = "push_typed_semantic_frame";
    auto semanticFrameRequestDataPayload = BuildSemanticFrameRequestData(SerializerMeta, directive.GetSemanticFrameRequestData());
    auto payload = MessageToStructBuilder(directive, SerializerMeta.GetBuilderOptions())
        .Set("semantic_frame_request_data", std::move(semanticFrameRequestDataPayload))
        .Build();
    return MakeIntrusive<TUniversalUniproxyDirectiveModel>(name, std::move(payload));
}

TIntrusivePtr<TMementoChangeUserObjectsDirectiveModel> TScenarioProtoDeserializer::DeserializeMementoChangeUserObjectsDirective(
    const NScenarios::TMementoChangeUserObjectsDirective& directive) const {
    return MakeIntrusive<TMementoChangeUserObjectsDirectiveModel>(SerializerMeta.GetScenarioName(), directive);
}

TIntrusivePtr<TUniversalClientDirectiveModel> TScenarioProtoDeserializer::DeserializeNavigateBrowserDirective(
    const NScenarios::TNavigateBrowserDirective& directive) const {
    return MakeUniversalDirective(directive,
                                  /* payload= */ TProtoStructBuilder{}
                                      .Set("command_name", NavigateBrowserCommandToString(directive.GetCommand()))
                                      .Build());
}

EStreamFormat GetEStreamFormat(const NScenarios::TAudioPlayDirective::TStream::TStreamFormat format) {
    switch (format) {
        case NScenarios::TAudioPlayDirective_TStream_TStreamFormat_MP3:
            return EStreamFormat::MP3;
        case NScenarios::TAudioPlayDirective_TStream_TStreamFormat_HLS:
            return EStreamFormat::HLS;
        case NScenarios::TAudioPlayDirective_TStream_TStreamFormat_Unknown:
            return EStreamFormat::Unknown;
        default:
            return EStreamFormat::Unknown;
    }
}

EStreamType GetEStreamType(const NScenarios::TAudioPlayDirective::TStream::TStreamType type) {
    switch (type) {
        case NScenarios::TAudioPlayDirective_TStream_TStreamType_Track:
            return EStreamType::Track;
        case NScenarios::TAudioPlayDirective_TStream_TStreamType_Shot:
            return EStreamType::Shot;
        default:
            return EStreamType::Track;
    }
}

EBackgroundMode GetEBackgroundMode(const NScenarios::TAudioPlayDirective::TBackgroundMode mode) {
    switch (mode) {
        case NScenarios::TAudioPlayDirective_TBackgroundMode_Ducking:
            return EBackgroundMode::Ducking;
        case NScenarios::TAudioPlayDirective_TBackgroundMode_Pause:
            return EBackgroundMode::Pause;
        default:
            return EBackgroundMode::Pause;
    }
}

EScreenType GetEScreenType(const NScenarios::TAudioPlayDirective::EScreenType screenType) {
    switch (screenType) {
        case NScenarios::TAudioPlayDirective_EScreenType_Music:
            return EScreenType::Music;
        case NScenarios::TAudioPlayDirective_EScreenType_Radio:
            return EScreenType::Radio;
        default:
            return EScreenType::Default;
    }
}

EContentType GetEContentType(const NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType type) {
    switch (type) {
        case NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Track:
            return EContentType::Track;
        case NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Album:
            return EContentType::Album;
        case NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Artist:
            return EContentType::Artist;
        case NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Playlist:
            return EContentType::Playlist;
        case NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Radio:
            return EContentType::Radio;
        case NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Generative:
            return EContentType::Generative;
        case NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_FmRadio:
            return EContentType::FmRadio;
        default:
            return EContentType::Track;
    }
}

ERepeatMode GetERepeateMode(const NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode repeatMode) {
    switch (repeatMode) {
        case NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode_None:
            return ERepeatMode::None;
        case NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode_One:
            return ERepeatMode::One;
        case NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode_All:
            return ERepeatMode::All;
        default:
            return ERepeatMode::Unknown;
    }
}

TPrevNextTrackInfo GetPrevNextTrackInfo(const NScenarios::TAudioPlayDirective_TAudioPlayMetadata_TPrevNextTrackInfo prevNextTrackInfo) {
    return {prevNextTrackInfo.GetId(), GetEStreamType(prevNextTrackInfo.GetStreamType())};
}

TIntrusivePtr<IInnerGlagolMetadataModel> GetInnerGlagolMetadata(const NScenarios::TAudioPlayDirective& directive) {
    switch (directive.GetAudioPlayMetadata().GetGlagolMetadata().GetTInnerGlagolMetadataCase()) {
        case NScenarios::TAudioPlayDirective_TAudioPlayMetadata_TGlagolMetadata::kMusicMetadata: {
            TMaybe<bool> shuffled;
            if (directive.GetAudioPlayMetadata().GetGlagolMetadata().GetMusicMetadata().HasShuffled()) {
                shuffled = directive.GetAudioPlayMetadata().GetGlagolMetadata().GetMusicMetadata().GetShuffled();
            }
            TMaybe<ERepeatMode> repeatMode;
            if (directive.GetAudioPlayMetadata().GetGlagolMetadata().GetMusicMetadata().HasRepeatMode()) {
                repeatMode = GetERepeateMode(directive.GetAudioPlayMetadata().GetGlagolMetadata().GetMusicMetadata().GetRepeatMode());
            }
            return MakeIntrusive<TMusicMetadataModel>(
                directive.GetAudioPlayMetadata().GetGlagolMetadata().GetMusicMetadata().GetId(),
                GetEContentType(directive.GetAudioPlayMetadata().GetGlagolMetadata().GetMusicMetadata().GetType()),
                directive.GetAudioPlayMetadata().GetGlagolMetadata().GetMusicMetadata().GetDescription(),
                GetPrevNextTrackInfo(directive.GetAudioPlayMetadata().GetGlagolMetadata().GetMusicMetadata().GetPrevTrackInfo()),
                GetPrevNextTrackInfo(directive.GetAudioPlayMetadata().GetGlagolMetadata().GetMusicMetadata().GetNextTrackInfo()),
                shuffled,
                repeatMode
            );
        }
        default:
            return MakeIntrusive<TStubInnerGlagolMetadataModel>();
    }
}

TIntrusivePtr<TAudioPlayDirectiveModel>
TScenarioProtoDeserializer::Deserialize(const NScenarios::TAudioPlayDirective& directive) const {
    TMaybe<TAudioPlayDirectiveStreamNormalizationModel> normalization;
    if (directive.GetStream().HasNormalization()) {
        normalization = TAudioPlayDirectiveStreamNormalizationModel(directive.GetStream().GetNormalization().GetIntegratedLoudness(),
                                                                         directive.GetStream().GetNormalization().GetTruePeak());
    }

    auto stream = TAudioPlayDirectiveStreamModel(directive.GetStream().GetId(),
                                                 directive.GetStream().GetUrl(),
                                                 directive.GetStream().GetOffsetMs(),
                                                 GetEStreamFormat(directive.GetStream().GetStreamFormat()),
                                                 GetEStreamType(directive.GetStream().GetStreamType()),
                                                 normalization);

    auto innerGlagolMetadata = GetInnerGlagolMetadata(directive);

    auto glagolMetadata = TGlagolMetadataModel(innerGlagolMetadata);

    auto metadata = TAudioPlayDirectiveMetadataModel(directive.GetAudioPlayMetadata().GetTitle(),
                                                     directive.GetAudioPlayMetadata().GetSubTitle(),
                                                     directive.GetAudioPlayMetadata().GetArtImageUrl(),
                                                     glagolMetadata,
                                                     directive.GetAudioPlayMetadata().GetHideProgressBar());

    const auto& rawCallbacks = directive.GetCallbacks();
    auto callbacks = TAudioPlayDirectiveCallbacksModel(
        rawCallbacks.HasOnPlayStartedCallback() ? Deserialize(rawCallbacks.GetOnPlayStartedCallback()) : nullptr,
        rawCallbacks.HasOnPlayStoppedCallback() ? Deserialize(rawCallbacks.GetOnPlayStoppedCallback()) : nullptr,
        rawCallbacks.HasOnPlayFinishedCallback() ? Deserialize(rawCallbacks.GetOnPlayFinishedCallback()) : nullptr,
        rawCallbacks.HasOnFailedCallback() ? Deserialize(rawCallbacks.GetOnFailedCallback()) : nullptr);

    THashMap<TString, TString> meta;
    for(auto const& [key, value] : directive.GetScenarioMeta()) {
        meta.insert({key, value});
    }

    TMaybe<TString> multiroomToken;
    if (const auto& token = directive.GetMultiroomToken(); !token.Empty()) {
        multiroomToken.ConstructInPlace(token);
    }

    return MakeIntrusive<TAudioPlayDirectiveModel>(directive.GetName(), stream, metadata, callbacks, meta,
                                                   GetEBackgroundMode(directive.GetBackgroundMode()),
                                                   directive.GetProviderName(),
                                                   GetEScreenType(directive.GetScreenType()), directive.GetSetPause(),
                                                   std::move(multiroomToken));
}

TIntrusivePtr<TUniproxyDirectiveModel> TScenarioProtoDeserializer::Deserialize(const NScenarios::TUpdateDatasyncDirective& directive) const {
    using NScenarios::TUpdateDatasyncDirective_EDataSyncMethod;
    switch (directive.GetMethod()) {
        case NScenarios::TUpdateDatasyncDirective_EDataSyncMethod_Put: {
            switch (directive.Value_case()) {
                case NScenarios::TUpdateDatasyncDirective::kStructValue:
                    return MakeIntrusive<TUpdateDatasyncDirectiveModel>(
                        directive.GetKey(), directive.GetStructValue(), EUpdateDatasyncMethod::Put);
                case NScenarios::TUpdateDatasyncDirective::kStringValue:
                    [[ fallthrough ]];
                default:
                    return MakeIntrusive<TUpdateDatasyncDirectiveModel>(
                        directive.GetKey(), directive.GetStringValue(), EUpdateDatasyncMethod::Put);
            }
        }
        default:
            return {};
    }
}

} // namespace NAlice::NMegamind
