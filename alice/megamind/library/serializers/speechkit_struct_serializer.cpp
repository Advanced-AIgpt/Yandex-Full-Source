#include "speechkit_struct_serializer.h"

#include <alice/megamind/library/common/defs.h>
#include <alice/megamind/library/models/buttons/action_button_model.h>
#include <alice/megamind/library/models/cards/div2_card_model.h>
#include <alice/megamind/library/models/cards/div_card_model.h>
#include <alice/megamind/library/models/cards/text_card_model.h>
#include <alice/megamind/library/models/cards/text_with_button_card_model.h>
#include <alice/megamind/library/models/directives/add_contact_book_asr_directive_model.h>
#include <alice/megamind/library/models/directives/alarm_new_directive_model.h>
#include <alice/megamind/library/models/directives/audio_play_directive_model.h>
#include <alice/megamind/library/models/directives/callback_directive_model.h>
#include <alice/megamind/library/models/directives/close_dialog_directive_model.h>
#include <alice/megamind/library/models/directives/defer_apply_directive_model.h>
#include <alice/megamind/library/models/directives/end_dialog_session_directive_model.h>
#include <alice/megamind/library/models/directives/find_contacts_directive_model.h>
#include <alice/megamind/library/models/directives/memento_change_user_objects_directive_model.h>
#include <alice/megamind/library/models/directives/music_play_directive_model.h>
#include <alice/megamind/library/models/directives/open_dialog_directive_model.h>
#include <alice/megamind/library/models/directives/open_settings_directive_model.h>
#include <alice/megamind/library/models/directives/player_rewind_directive_model.h>
#include <alice/megamind/library/models/directives/protobuf_uniproxy_directive_model.h>
#include <alice/megamind/library/models/directives/set_cookies_directive_model.h>
#include <alice/megamind/library/models/directives/set_search_filter_directive_model.h>
#include <alice/megamind/library/models/directives/set_timer_directive_model.h>
#include <alice/megamind/library/models/directives/show_buttons_directive_model.h>
#include <alice/megamind/library/models/directives/theremin_play_directive_model.h>
#include <alice/megamind/library/models/directives/typed_semantic_frame_request_directive_model.h>
#include <alice/megamind/library/models/directives/universal_client_directive_model.h>
#include <alice/megamind/library/models/directives/universal_uniproxy_directive_model.h>
#include <alice/megamind/library/models/directives/update_datasync_directive_model.h>
#include <alice/megamind/library/models/directives/update_dialog_info_directive_model.h>
#include <alice/megamind/library/models/directives/update_space_actions.h>
#include <alice/megamind/library/request/event/server_action_event.h>

#include <util/generic/variant.h>
#include <util/string/cast.h>

#include <utility>

namespace NAlice::NMegamind {

namespace {

void FillRoomDeviceIds(const TIoTUserInfoDevices& ioTUserInfoDevices, const TString& locationId,
                       TProtoStructBuilder& directive) {
    TProtoListBuilder roomDeviceIds;
    const auto onDeviceId = [&roomDeviceIds](const TString& deviceId) {
        roomDeviceIds.Add(deviceId);
    };
    ForEachQuasarDeviceIdInLocation(ioTUserInfoDevices, locationId, onDeviceId);

    auto roomDeviceIdsList = roomDeviceIds.Build();
    if (roomDeviceIdsList.values_size()) {
        directive.Set(ROOM_DEVICE_IDS, std::move(roomDeviceIdsList));
    }
}

void FillRoomDeviceIds(const TIoTUserInfoDevices& ioTUserInfoDevices, const NScenarios::TLocationInfo& locationInfo,
                       TProtoStructBuilder& directive, const TString& currentDeviceId)
{
    TProtoListBuilder roomDeviceIds;
    const auto onDeviceId = [&roomDeviceIds](const TString& deviceId) {
        roomDeviceIds.Add(deviceId);
    };
    ForEachQuasarDeviceIdInLocation(ioTUserInfoDevices, locationInfo, onDeviceId, currentDeviceId);

    auto roomDeviceIdsList = roomDeviceIds.Build();
    if (roomDeviceIdsList.values_size()) {
        directive.Set(ROOM_DEVICE_IDS, std::move(roomDeviceIdsList));
    }
}

void TryFillGroupDeviceIds(const TIoTUserInfoDevices& ioTUserInfoDevices, const TString& deviceId,
                           TProtoStructBuilder& directive) {
    TProtoListBuilder groupDeviceIds;
    const auto onDeviceId = [&groupDeviceIds](const TString& deviceId) {
        groupDeviceIds.Add(deviceId);
    };
    ForEachQuasarDeviceIdThatSharesGroupWith(ioTUserInfoDevices, deviceId, onDeviceId);

    auto groupDeviceIdsList = groupDeviceIds.Build();
    if (groupDeviceIdsList.values_size()) {
        directive.Set(ROOM_DEVICE_IDS, std::move(groupDeviceIdsList));
    }
}

} // namespace

TSpeechKitStructSerializer::TSpeechKitStructSerializer(TSerializerMeta serializerMeta)
    : SerializerMeta(std::move(serializerMeta)) {
}

google::protobuf::Struct TSpeechKitStructSerializer::Serialize(const IModel& model) const {
    TSpeechKitStructSerializer serializer(SerializerMeta);
    model.Accept(serializer);
    return serializer.Struct;
}

google::protobuf::Struct
TSpeechKitStructSerializer::Serialize(const TAlarmSetSoundDirectiveModel::TSettings& model) const {
    return MessageToStructBuilder(model).Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::Serialize(const TUpdateSpaceActionsDirectiveModel::TActionSpaces& model) const {
    TProtoStructBuilder payload{};
    for (const auto& [spaceId, frames] : model) {
        TProtoStructBuilder actionFrames{};
        for (const auto& [frame, semanticFrame] : frames) {
            actionFrames.Set(
                frame, TProtoStructBuilder{}
                           .Set("typed_semantic_frame",
                                MessageToStruct(semanticFrame.GetFrame(), SerializerMeta.GetBuilderOptions()))
                           .Set("analytics",
                                MessageToStruct(semanticFrame.GetAnalytics(), SerializerMeta.GetBuilderOptions()))
                           .Build());
        }
        payload.Set(spaceId, actionFrames.Build());
    }
    return payload.Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::Serialize(const THashMap<TString, TConditionalAction>& model) const {
    TProtoStructBuilder payload{};
    for (const auto& [id, action] : model) {
        payload.Set(id, TProtoStructBuilder{}
                           .Set("conditional_semantic_frame",
                                MessageToStruct(action.GetConditionalSemanticFrame(), SerializerMeta.GetBuilderOptions()))
                           .Set("effect_frame_request_data",
                                MessageToStruct(action.GetEffectFrameRequestData(), SerializerMeta.GetBuilderOptions()))
                           .Build());
    }
    return payload.Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::Serialize(const TVector<NData::TExternalEntityDescription>& model) const {
    TProtoListBuilder listEntities{};
    for (const auto& entity : model) {
        listEntities.Add(MessageToStruct(entity, SerializerMeta.GetBuilderOptions()));
    }

    TProtoStructBuilder payload{};
    payload.Set("external_entities_description", listEntities.Build());
    return payload.Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TActionButtonModel& model) const {
    TProtoListBuilder directives{};
    for (const auto& directive : model.GetDirectives()) {
        directives.Add(Serialize(*directive));
    }
    auto button = SerializeBaseModel(model).Set("directives", directives.Build());
    if (const auto& theme = model.GetTheme(); theme.Defined()) {
        button.Set("theme", TProtoStructBuilder{}.Set("image_url", theme->GetImageUrl()).Build());
    }
    if (const auto& text = model.GetText(); text.Defined()) {
        button.Set("text", *text);
    }
    return button.Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TDiv2CardModel& model) const {
    return SerializeBaseModel(model)
        .Set("body", model.GetBody())
        .SetBool("has_borders", model.GetHasBorders())
        .Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TDivCardModel& model) const {
    return SerializeBaseModel(model).Set("text", "...").Set("body", model.GetBody()).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TTextCardModel& model) const {
    return SerializeBaseModel(model).Set("text", model.GetText()).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TTextWithButtonCardModel& model) const {
    TProtoListBuilder buttons{};
    for (const auto& button : model.GetButtons()) {
        buttons.Add(Serialize(*button));
    }
    return SerializeBaseModel(model).Set("text", model.GetText()).Set("buttons", buttons.Build()).Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TAddContactBookAsrDirectiveModel& model) const {
    return SerializeBaseDirective(model).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TAlarmNewDirectiveModel& model) const {
    const auto payload =
        TProtoStructBuilder()
            .Set("state", model.GetState())
            .Set("on_fail",
                 GetCallbackPayload(model.GetOnFailureCallbackPayload(), SerializerMeta, model.GetName()).Build())
            .Set("on_success",
                 GetCallbackPayload(model.GetOnSuccessCallbackPayload(), SerializerMeta, model.GetName()).Build());
    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TAlarmSetSoundDirectiveModel& model) const {
    const auto payload = TProtoStructBuilder()
                             .Set("server_action", SerializeModel(model.GetCallback()))
                             .Set("sound_alarm_setting", Serialize(model.GetSettings()));
    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TCloseDialogDirectiveModel& model) const {
    auto payload = TProtoStructBuilder();
    payload.Set("dialog_id", SerializerMeta.WrapDialogId(model.GetDialogId()));
    if (model.GetScreenId().Defined()) {
        payload.Set("screen_id", *model.GetScreenId());
    }
    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TEndDialogSessionDirectiveModel& model) const {
    const auto payload = TProtoStructBuilder().Set("dialog_id", SerializerMeta.WrapDialogId(model.GetDialogId()));
    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TFindContactsDirectiveModel& model) const {
    TProtoListBuilder request{};
    for (const auto& part : model.GetRequest()) {
        request.Add(
            TProtoStructBuilder().Set("tag", part.GetTag()).Set("values", ToProtoList(part.GetValues())).Build());
    }

    const auto payload =
        TProtoStructBuilder()
            .Set("mimetypes_whitelist", TProtoStructBuilder()
                                            .Set("column", ToProtoList(model.GetMimeTypeWhiteListColumn()))
                                            .Set("name", ToProtoList(model.GetMimeTypeWhiteListName()))
                                            .Build())
            .Set("on_permission_denied_payload",
                 GetCallbackPayload(model.GetOnPermissionDeniedCallbackPayload(), SerializerMeta, model.GetName())
                     .Build())
            .Set("request", request.Build())
            .Set("values", ToProtoList(model.GetValues()))
            .Set("form", model.GetCallbackName());
    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TMementoChangeUserObjectsDirectiveModel& model) const {
    auto builder = TProtoStructBuilder()
        .Set("user_objects", ProtoToBase64String(model.GetUserObjects()));
    return SerializeBaseDirective(model)
        .Set("payload", builder.Build())
        .Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TMusicPlayDirectiveModel& model) const {
    auto payload = TProtoStructBuilder()
                       .Set("uid", model.GetUid())
                       .Set("session_id", model.GetSessionId())
                       .SetDouble("offset", model.GetOffset());
    if (const auto& alarmId = model.GetAlarmId(); alarmId.Defined()) {
        payload.Set("alarm_id", alarmId.GetRef());
    }
    if (const auto& firstTrackId = model.GetFirstTrackId(); firstTrackId.Defined()) {
        payload.Set("first_track_id", firstTrackId.GetRef());
    }

    auto res = SerializeBaseDirective(model).Set("payload", payload.Build());

    if (const auto* ioTUserInfo = SerializerMeta.GetIoTUserInfo().Get()) {
        if (const auto* locationInfo = model.GetLocationInfo().Get()) {
            FillRoomDeviceIds(ioTUserInfo->GetDevices(), *locationInfo, res, SerializerMeta.GetClientInfo().DeviceId);
        } else if (const auto* roomId = model.GetRoomId().Get()) {
            FillRoomDeviceIds(ioTUserInfo->GetDevices(), *roomId, res);
        } else if (SerializerMeta.GetClientInfo().IsSmartSpeaker()) {
            TryFillGroupDeviceIds(ioTUserInfo->GetDevices(), SerializerMeta.GetClientInfo().DeviceId, res);
        }
    }

    return res.Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TOpenDialogDirectiveModel& model) const {
    TProtoListBuilder directives{};
    for (const auto& directive : model.GetDirectives()) {
        directives.Add(Serialize(*directive));
    }
    const auto payload = TProtoStructBuilder()
                             .Set("dialog_id", SerializerMeta.WrapDialogId(model.GetDialogId()))
                             .Set("directives", directives.Build());
    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TOpenSettingsDirectiveModel& model) const {
    const auto payload = TProtoStructBuilder().Set("target", ToString(model.GetTarget()));
    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TPlayerRewindDirectiveModel& model) const {
    const auto payload =
        TProtoStructBuilder().SetUInt64("amount", model.GetAmount()).Set("type", ToString(model.GetRewindType()));
    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TSetCookiesDirectiveModel& model) const {
    const auto payload = TProtoStructBuilder().Set("value", model.GetValue());
    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TSetSearchFilterDirectiveModel& model) const {
    const auto payload = TProtoStructBuilder().Set("new_level", ToString(model.GetLevel()));
    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TSetTimerDirectiveModel& model) const {
    TProtoStructBuilder payload{};
    if (model.IsTimestampCase()) {
        payload.SetUInt64("timestamp", model.GetTimestamp())
            .Set("on_fail",
                 GetCallbackPayload(model.GetOnFailureCallbackPayload(), SerializerMeta, model.GetName()).Build())
            .Set("on_success",
                 GetCallbackPayload(model.GetOnSuccessCallbackPayload(), SerializerMeta, model.GetName()).Build());
    } else {
        payload.SetUInt64("duration", model.GetDuration())
            .SetBool("listening_is_possible", model.GetListeningIsPossible());
    }
    if (!model.GetDirectives().empty()) {
        TProtoListBuilder directives{};
        for (const auto& directive : model.GetDirectives()) {
            directives.Add(Serialize(*directive));
        }
        payload.Set("directives", directives.Build());
    }
    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TThereminPlayDirectiveModel& model) const {
    const auto payload = TProtoStructBuilder().Set(ToString(model.GetSet()->GetType()), Serialize(*model.GetSet()));
    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TThereminPlayDirectiveExternalSetModel& model) const {
    TProtoListBuilder samples{};
    for (const auto& sample : model.GetSamples()) {
        samples.Add(TProtoStructBuilder().Set("url", sample.GetUrl()).Build());
    }
    return TProtoStructBuilder()
        .SetBool("repeat_sound_inside", model.GetRepeatSoundInside())
        .SetBool("no_overlay_samples", model.GetNoOverlaySamples())
        .SetBool("stop_on_ceil", model.GetStopOnCeil())
        .Set("samples", samples.Build())
        .Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TThereminPlayDirectiveInternalSetModel& model) const {
    return TProtoStructBuilder().SetInt("mode", model.GetMode()).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TTypedSemanticFrameRequestDirectiveModel& model) const {
    TProtoStructBuilder payload;
    payload.Set("typed_semantic_frame", MessageToStruct(model.GetFrame(), SerializerMeta.GetBuilderOptions()));
    payload.Set("analytics", MessageToStruct(model.GetAnalytics(), SerializerMeta.GetBuilderOptions()));
    if (const auto& utterance = model.GetUtterance(); !utterance.empty()) {
        payload.Set("utterance", utterance);
    }
    if (const auto& params = model.GetFrameRequestParams(); params.Defined()) {
        payload.Set("params", MessageToStruct(*params, SerializerMeta.GetBuilderOptions()));
    }
    if (const auto& requestParams = model.GetRequestParams(); requestParams.Defined()) {
        payload.Set("request_params", MessageToStruct(*requestParams, SerializerMeta.GetBuilderOptions()));
    }

    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TUniversalClientDirectiveModel& model) const {
    auto res = SerializeBaseDirective(model).Set("payload", model.GetPayload());

    if (model.GetMultiroomSessionId().Defined()) {
        res.Set("multiroom_session_id", model.GetMultiroomSessionId().GetRef());
    }

    if (const auto* ioTUserInfo = SerializerMeta.GetIoTUserInfo().Get()) {
        if (const auto* locationInfo = model.GetLocationInfo().Get()) {
            if (!locationInfo->GetCurrentGroup()) {
                FillRoomDeviceIds(ioTUserInfo->GetDevices(), *locationInfo, res, SerializerMeta.GetClientInfo().DeviceId);
            } else if (SerializerMeta.GetClientInfo().IsSmartSpeaker()) {
                TryFillGroupDeviceIds(ioTUserInfo->GetDevices(), SerializerMeta.GetClientInfo().DeviceId, res);
            }
        } else if (const auto* roomId = model.GetRoomId().Get()) {
            if (*roomId != IOT_GROUP_ROOM_ID) {
                FillRoomDeviceIds(ioTUserInfo->GetDevices(), *roomId, res);
            } else if (SerializerMeta.GetClientInfo().IsSmartSpeaker()) {
                TryFillGroupDeviceIds(ioTUserInfo->GetDevices(), SerializerMeta.GetClientInfo().DeviceId, res);
            }
        }
    }

    return res.Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TUpdateSpaceActionsDirectiveModel& model) const {
    return SerializeBaseDirective(model).Set("payload", Serialize(model.GetActionSpaces())).Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TAddConditionalActionsDirectiveModel& model) const {
    return SerializeBaseDirective(model).Set("payload", Serialize(model.GetConditionalActions())).Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TAddExternalEntitiesDescriptionDirectiveModel& model) const {
    return SerializeBaseDirective(model).Set("payload", Serialize(model.GetExternalEntitiesDescription())).Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TShowButtonsDirectiveModel& model) const {
    TProtoStructBuilder payload;
    if (const auto& screenId = model.GetScreenId(); screenId.Defined()) {
        payload.Set("screen_id", *screenId);
    }

    TProtoListBuilder buttons;
    for (const auto& button : model.GetButtons()) {
        buttons.Add(Serialize(*button));
    }
    payload.Set("buttons", buttons.Build());

    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TUniversalUniproxyDirectiveModel& model) const {
    return SerializeBaseDirective(model).Set("payload", model.GetPayload()).Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TUpdateDialogInfoDirectiveModel& model) const {
    TProtoListBuilder menuItems{};
    for (const auto& menuItem : model.GetMenuItems()) {
        menuItems.Add(SerializeModel(menuItem));
    }
    const auto payload = TProtoStructBuilder()
                             .Set("image_url", model.GetImageUrl())
                             .Set("url", model.GetUrl())
                             .Set("title", model.GetTitle())
                             .Set("style", SerializeModel(model.GetStyle()))
                             .Set("dark_style", SerializeModel(model.GetDarkStyle()))
                             .Set("menu_items", menuItems.Build())
                             .Set("ad_block_id", model.GetAdBlockId());
    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TUpdateDialogInfoDirectiveStyleModel& model) const {
    TProtoListBuilder oknyxErrorColors{};
    for (const auto& oknyxErrorColor : model.GetOknyxErrorColors()) {
        oknyxErrorColors.Add(oknyxErrorColor);
    }
    TProtoListBuilder oknyxNormalColors{};
    for (const auto& oknyxNormalColor : model.GetOknyxNormalColors()) {
        oknyxNormalColors.Add(oknyxNormalColor);
    }
    return TProtoStructBuilder()
        .Set("oknyx_logo", model.GetOknyxLogo())
        .Set("suggest_border_color", model.GetSuggestBorderColor())
        .Set("suggest_fill_color", model.GetSuggestFillColor())
        .Set("suggest_text_color", model.GetSuggestTextColor())
        .Set("skill_actions_text_color", model.GetSkillActionsTextColor())
        .Set("skill_bubble_fill_color", model.GetSkillBubbleFillColor())
        .Set("skill_bubble_text_color", model.GetSkillBubbleTextColor())
        .Set("user_bubble_fill_color", model.GetUserBubbleFillColor())
        .Set("user_bubble_text_color", model.GetUserBubbleTextColor())
        .Set("oknyx_error_colors", oknyxErrorColors.Build())
        .Set("oknyx_normal_colors", oknyxNormalColors.Build())
        .Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TUpdateDialogInfoDirectiveMenuItemModel& model) const {
    return TProtoStructBuilder().Set("title", model.GetTitle()).Set("url", model.GetUrl()).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TCallbackDirectiveModel& model) const {
    return SerializeBaseDirective(model)
        .SetBool("is_led_silent", model.GetIsLedSilent())
        .Set("payload", GetCallbackPayload(model.GetPayload(), SerializerMeta, model.GetName()).Build())
        .Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TGetNextCallbackDirectiveModel& model) const {
    TProtoStructBuilder payloadBuilder;
    if (const auto& recoveryCallback = model.GetRecoveryCallback(); recoveryCallback.Defined()) {
        payloadBuilder.Set(MM_STACK_ENGINE_RECOVERY_CALLBACK_FIELD_NAME, SerializeModel(recoveryCallback.GetRef()));
    }
    auto getNextCallbackPayload = GetCallbackPayload(payloadBuilder.Build(), SerializerMeta, model.GetName());
    getNextCallbackPayload.Set(TString{MM_STACK_ENGINE_SESSION_ID}, model.GetSessionId());
    getNextCallbackPayload.Set(TString{MM_STACK_ENGINE_PRODUCT_SCENARIO_NAME}, model.GetProductScenarioName());
    return SerializeBaseDirective(model)
        .SetBool("is_led_silent", model.GetIsLedSilent())
        .Set("payload", getNextCallbackPayload.Build())
        .Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TDeferApplyDirectiveModel& model) const {
    const auto payload = TProtoStructBuilder().Set("session", model.GetSession());
    return SerializeBaseDirective(model).Set("payload", payload.Build()).Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TUpdateDatasyncDirectiveModel& model) const {
    auto builder = TProtoStructBuilder{}
        .Set("key", model.GetKey())
        .Set("method", ToString(model.GetMethod()))
        .SetBool("listening_is_possible", true);

    std::visit([&builder](const auto& value) {
        builder.Set("value", value);
    }, model.GetValue());

    return SerializeBaseDirective(model)
        .Set("payload", builder.Build())
        .Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TStubInnerGlagolMetadataModel& /* model */) const {
    return {};
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TMusicMetadataModel& model) const {
    auto builder = TProtoStructBuilder();
    builder.Set("id", model.GetId())
           .Set("type", ToString(model.GetType()))
           .Set("description", model.GetDescription())
           .Set("prev_track_info", TProtoStructBuilder{}
                                       .Set("id", model.GetPrevTrackInfo().Id)
                                       .Set("type", ToString(model.GetPrevTrackInfo().StreamType)).Build())
           .Set("next_track_info", TProtoStructBuilder{}
                                       .Set("id", model.GetNextTrackInfo().Id)
                                       .Set("type", ToString(model.GetNextTrackInfo().StreamType)).Build());

    if (const auto shuffled = model.GetShuffled(); shuffled.Defined()) {
        builder.SetBool("shuffled", shuffled.GetRef());
    }
    if (const auto repeatMode = model.GetRepeatMode(); repeatMode.Defined()) {
        builder.Set("repeat_mode", ToString(repeatMode.GetRef()));
    }

    return builder.Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TGlagolMetadataModel& model) const {
    return TProtoStructBuilder()
        .Set(ToString(model.GetInnerGlagolMetadata().GetInnerGlagolMetadataType()), Serialize(model.GetInnerGlagolMetadata()))
        .Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TAudioPlayDirectiveMetadataModel& model) const {
    return TProtoStructBuilder()
        .Set("title", model.GetTitle())
        .Set("subtitle", model.GetSubTitle())
        .Set("art_image_url", model.GetArtImageUrl())
        .Set("glagol_metadata", Serialize(model.GetGlagolMetadata()))
        .SetBool("hide_progress_bar", model.GetHideProgressBar())
        .Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TAudioPlayDirectiveStreamModel& model) const {
    TProtoStructBuilder builder;
    builder.Set("id", model.GetId())
           .Set("url", model.GetUrl())
           .SetInt("offset_ms", model.GetOffsetMs())
           .Set("format", ToString(model.GetStreamFormat()))
           .Set("type", ToString(model.GetStreamType()));
    if (const auto& normalization = model.GetNormalization()) {
        builder.Set("normalization", Serialize(*normalization));
    }
    return builder.Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TAudioPlayDirectiveCallbacksModel& model) const {
    auto callbacksBuilder = TProtoStructBuilder();
    if (const auto& callback = model.GetOnFailedCallback()) {
        callbacksBuilder.Set("on_failed", Serialize(*callback));
    }
    if (const auto& callback = model.GetOnPlayFinishedCallback()) {
        callbacksBuilder.Set("on_finished", Serialize(*callback));
    }
    if (const auto& callback = model.GetOnPlayStartedCallback()) {
        callbacksBuilder.Set("on_started", Serialize(*callback));
    }
    if (const auto& callback = model.GetOnPlayStoppedCallback()) {
        callbacksBuilder.Set("on_stopped", Serialize(*callback));
    }
    return callbacksBuilder.Build();
}

google::protobuf::Struct
TSpeechKitStructSerializer::SerializeModel(const TAudioPlayDirectiveStreamNormalizationModel& model) const {
    return TProtoStructBuilder()
        .SetDouble("integrated_loudness", model.GetIntegratedLoudness())
        .SetDouble("true_peak", model.GetTruePeak())
        .Build();
}

google::protobuf::Struct TSpeechKitStructSerializer::SerializeModel(const TAudioPlayDirectiveModel& model) const {
    auto scenarioMetaBuilder = TProtoStructBuilder();
    for(auto const& [key, value] : model.GetScenarioMeta()){
        scenarioMetaBuilder.Set(key, value);
    }
    scenarioMetaBuilder.Set(TString{SCENARIO_NAME_JSON_KEY}, SerializerMeta.GetScenarioName());

    auto builder = TProtoStructBuilder{}
            .Set("stream", SerializeModel(model.GetStream()))
            .Set("metadata", SerializeModel(model.GetAudioPlayMetadata()))
            .Set("callbacks", SerializeModel(model.GetCallbacks()))
            .Set("scenario_meta", scenarioMetaBuilder.Build())
            .Set("background_mode", ToString(model.GetBackgroundMode()))
            .Set("provider_name", ToString(model.GetProviderName()))
            .Set("screen_type", ToString(model.GetScreenType()))
            .SetBool("set_pause", model.GetSetPause());
    if (const auto& multiroomToken = model.GetMultiroomToken(); multiroomToken.Defined()) {
        builder.Set("multiroom_token", *multiroomToken);
    }

    return SerializeBaseDirective(model).Set("payload", builder.Build()).Build();
}

TProtoStructBuilder TSpeechKitStructSerializer::SerializeBaseModel(const IButtonModel& model) const {
    return TProtoStructBuilder().Set("title", model.GetTitle()).Set("type", ToString(model.GetType()));
}

TProtoStructBuilder TSpeechKitStructSerializer::SerializeBaseModel(const ICardModel& model) const {
    return TProtoStructBuilder().Set("type", ToString(model.GetType()));
}

TProtoStructBuilder TSpeechKitStructSerializer::SerializeBaseModel(const IDirectiveModel& model) const {
    return TProtoStructBuilder()
        .Set("name", model.GetName())
        .Set("type", ToString(model.GetType()))
        .Set("payload", google::protobuf::Struct());
}

TProtoStructBuilder TSpeechKitStructSerializer::SerializeBaseDirective(const TClientDirectiveModel& model) const {
    return SerializeBaseModel(model).Set("sub_name", model.GetAnalyticsType());
}

TProtoStructBuilder TSpeechKitStructSerializer::SerializeBaseDirective(const TServerDirectiveModel& model) const {
    auto res = SerializeBaseModel(model).SetBool("ignore_answer", model.GetIgnoreAnswer());

    if (model.GetMultiroomSessionId().Defined()) {
        res.Set("multiroom_session_id", model.GetMultiroomSessionId().GetRef());
    }

    if (const auto* ioTUserInfo = SerializerMeta.GetIoTUserInfo().Get()) {
        if (const auto* locationInfo = model.GetLocationInfo().Get()) {
            FillRoomDeviceIds(ioTUserInfo->GetDevices(), *locationInfo, res, SerializerMeta.GetClientInfo().DeviceId);
        } else if (const auto* roomId = model.GetRoomId().Get()) {
            FillRoomDeviceIds(ioTUserInfo->GetDevices(), *roomId, res);
        }
    }

    return res;
}

TProtoStructBuilder TSpeechKitStructSerializer::SerializeBaseDirective(const TUniproxyDirectiveModel& model) const {
    TProtoStructBuilder directive = SerializeBaseModel(model);
    if (const auto* uniproxyDirectiveMeta = model.GetUniproxyDirectiveMeta()) {
        TProtoStructBuilder meta;
        meta.Set("puid", uniproxyDirectiveMeta->GetPuid());
        directive.Set("uniproxy_directive_meta", meta.Build());
    }
    return directive;
}

void TSpeechKitStructSerializer::Visit(const TActionButtonModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TDiv2CardModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TDivCardModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TTextCardModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TTextWithButtonCardModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TAddContactBookAsrDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TAlarmNewDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TAlarmSetSoundDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TCloseDialogDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TEndDialogSessionDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TFindContactsDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TMementoChangeUserObjectsDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TMusicPlayDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TOpenDialogDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TOpenSettingsDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TPlayerRewindDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TSetCookiesDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TSetSearchFilterDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TSetTimerDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TThereminPlayDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TThereminPlayDirectiveExternalSetModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TThereminPlayDirectiveInternalSetModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TTypedSemanticFrameRequestDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TUniversalClientDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TUniversalUniproxyDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TUpdateDialogInfoDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TUpdateDialogInfoDirectiveMenuItemModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TUpdateDialogInfoDirectiveStyleModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TCallbackDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TGetNextCallbackDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TDeferApplyDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TUpdateDatasyncDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TAudioPlayDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TAudioPlayDirectiveMetadataModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TAudioPlayDirectiveStreamModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TAudioPlayDirectiveCallbacksModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TAudioPlayDirectiveStreamNormalizationModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TStubInnerGlagolMetadataModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TMusicMetadataModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TGlagolMetadataModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TUpdateSpaceActionsDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TAddConditionalActionsDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TAddExternalEntitiesDescriptionDirectiveModel& model) {
    Struct = SerializeModel(model);
}

void TSpeechKitStructSerializer::Visit(const TProtobufUniproxyDirectiveModel& model) {
    std::visit([this](const auto& value) {
        Struct = MessageToStruct(value);
    }, model.Directives());
}

void TSpeechKitStructSerializer::Visit(const TShowButtonsDirectiveModel& model) {
    Struct = SerializeModel(model);
}

} // namespace NAlice::NMegamind
