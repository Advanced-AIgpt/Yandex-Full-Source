#include "speechkit_proto_serializer.h"

#include <alice/megamind/library/common/defs.h>
#include <alice/megamind/library/models/buttons/action_button_model.h>
#include <alice/megamind/library/models/cards/div2_card_model.h>
#include <alice/megamind/library/models/cards/div_card_model.h>
#include <alice/megamind/library/models/cards/text_card_model.h>
#include <alice/megamind/library/models/cards/text_with_button_card_model.h>
#include <alice/megamind/library/models/directives/add_contact_book_asr_directive_model.h>
#include <alice/megamind/library/models/directives/alarm_new_directive_model.h>
#include <alice/megamind/library/models/directives/alarm_set_sound_directive_model.h>
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

#include <alice/protos/api/matrix/scheduler_api.pb.h>

#include <alice/library/proto/protobuf.h>
#include <alice/library/response/defs.h>

#include <util/generic/variant.h>
#include <util/string/cast.h>

#include <utility>

namespace NAlice::NMegamind {

namespace {

void FillRoomDeviceIds(const TIoTUserInfoDevices& ioTUserInfoDevices, const TString& locationId,
                       NSpeechKit::TDirective& directive) {
    const auto onDeviceId = [&directive](const TString& deviceId) {
        *directive.AddRoomDeviceIds() = deviceId;
    };
    ForEachQuasarDeviceIdInLocation(ioTUserInfoDevices, locationId, onDeviceId);
}

void FillRoomDeviceIds(const TIoTUserInfoDevices& ioTUserInfoDevices, const NScenarios::TLocationInfo& locationInfo,
                       NSpeechKit::TDirective& directive, const TString& currentDeviceId) {
    const auto onDeviceId = [&directive](const TString& deviceId) {
        *directive.AddRoomDeviceIds() = deviceId;
    };
    ForEachQuasarDeviceIdInLocation(ioTUserInfoDevices, locationInfo, onDeviceId, currentDeviceId);
}

void TryFillGroupDeviceIds(const TIoTUserInfoDevices& iotUserInfoDevices, const TString& deviceId,
                           NSpeechKit::TDirective& directive) {
    const auto onDeviceId = [&directive](const TString& deviceId) {
        *directive.AddRoomDeviceIds() = deviceId;
    };
    ForEachQuasarDeviceIdThatSharesGroupWith(iotUserInfoDevices, deviceId, onDeviceId);
}

} // namespace

TSpeechKitProtoSerializer::TSpeechKitProtoSerializer(TSerializerMeta serializerMeta)
    : SerializerMeta(std::move(serializerMeta))
    , SpeechKitStructSerializer(TSpeechKitStructSerializer(SerializerMeta)) {
}

TSpeechKitResponseProto_TResponse_TButton TSpeechKitProtoSerializer::Serialize(const IButtonModel& model) const {
    TSpeechKitProtoSerializer serializer(SerializerMeta);
    model.Accept(serializer);
    return serializer.Button;
}

TSpeechKitResponseProto_TResponse_TCard TSpeechKitProtoSerializer::Serialize(const ICardModel& model) const {
    TSpeechKitProtoSerializer serializer(SerializerMeta);
    model.Accept(serializer);
    return serializer.Card;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::Serialize(const IDirectiveModel& model) const {
    TSpeechKitProtoSerializer serializer(SerializerMeta);
    model.Accept(serializer);
    return serializer.Directive;
}

TMaybe<NSpeechKit::TProtobufUniproxyDirective> TSpeechKitProtoSerializer::SerializeProtobufUniproxyDirective(const IDirectiveModel& model) const {
    TSpeechKitProtoSerializer serializer(SerializerMeta);
    model.Accept(serializer);
    return serializer.ProtobufUniproxyDirective;
}

TSpeechKitResponseProto_TResponse_TButton
TSpeechKitProtoSerializer::SerializeModel(const TActionButtonModel& model) const {
    auto button = SerializeBaseModel(model);
    for (const auto& directive : model.GetDirectives()) {
        *button.AddDirectives() = Serialize(*directive);
    }
    if (const auto& theme = model.GetTheme(); theme.Defined()) {
        button.MutableTheme()->SetImageUrl(theme->GetImageUrl());
    }
    if (const auto& text = model.GetText(); text.Defined()) {
        button.SetText(*text);
    }
    return button;
}

TSpeechKitResponseProto_TResponse_TCard TSpeechKitProtoSerializer::SerializeModel(const TDiv2CardModel& model) const {
    auto card = SerializeBaseModel(model);
    *card.MutableBody() = model.GetBody();
    card.MutableHasBorders()->set_value(model.GetHasBorders());
    if (const auto& text = model.GetText(); !text.empty()) {
        card.SetText(text);
    }
    return card;
}

TSpeechKitResponseProto_TResponse_TCard TSpeechKitProtoSerializer::SerializeModel(const TDivCardModel& model) const {
    auto card = SerializeBaseModel(model);
    // default text value from VINS
    // see https://a.yandex-team.ru/arc/trunk/arcadia/alice/vins/core/vins_core/dm/response.py?rev=5007750#L79
    // FIXME(sparkle, zhigan, g-kostin): return back to "..." after HOLLYWOOD-586 supported on search app
    if (const auto& text = model.GetText(); text.Defined()) {
        card.SetText(text->data(), text->size());
    } else {
        card.SetText(NResponse::THREE_DOTS);
    }
    *card.MutableBody() = model.GetBody();
    return card;
}

TSpeechKitResponseProto_TResponse_TCard TSpeechKitProtoSerializer::SerializeModel(const TTextCardModel& model) const {
    auto card = SerializeBaseModel(model);
    card.SetText(model.GetText());
    return card;
}

TSpeechKitResponseProto_TResponse_TCard
TSpeechKitProtoSerializer::SerializeModel(const TTextWithButtonCardModel& model) const {
    auto card = SerializeBaseModel(model);
    card.SetText(model.GetText());
    for (const auto& button : model.GetButtons()) {
        *card.AddButtons() = Serialize(*button);
    }
    return card;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TAddContactBookAsrDirectiveModel& model) const {
    return SerializeDirective(model);
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TAlarmNewDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() =
        TProtoStructBuilder()
            .Set("state", model.GetState())
            .Set("on_fail",
                 GetCallbackPayload(model.GetOnFailureCallbackPayload(), SerializerMeta, model.GetName()).Build())
            .Set("on_success",
                 GetCallbackPayload(model.GetOnSuccessCallbackPayload(), SerializerMeta, model.GetName()).Build())
            .Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TAlarmSetSoundDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() =
        TProtoStructBuilder()
            .Set("server_action", SpeechKitStructSerializer.Serialize(model.GetCallback()))
            .Set("sound_alarm_setting", SpeechKitStructSerializer.Serialize(model.GetSettings()))
            .Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TCloseDialogDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    auto payload = TProtoStructBuilder();
    payload.Set("dialog_id", SerializerMeta.WrapDialogId(model.GetDialogId()));
    if (model.GetScreenId().Defined()) {
        payload.Set("screen_id", *model.GetScreenId());
    }
    *directive.MutablePayload() = payload.Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TEndDialogSessionDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() =
        TProtoStructBuilder().Set("dialog_id", SerializerMeta.WrapDialogId(model.GetDialogId())).Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TFindContactsDirectiveModel& model) const {
    TProtoListBuilder request{};
    for (const auto& part : model.GetRequest()) {
        request.Add(
            TProtoStructBuilder().Set("tag", part.GetTag()).Set("values", ToProtoList(part.GetValues())).Build());
    }

    auto directive = SerializeDirective(model);
    *directive.MutablePayload() =
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
            .Set("form", model.GetCallbackName())
            .Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TMementoChangeUserObjectsDirectiveModel& model) const {
    auto builder = TProtoStructBuilder()
        .Set("user_objects", ProtoToBase64String(model.GetUserObjects()));
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = builder.Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TMusicPlayDirectiveModel& model) const {
    auto payloadBuilder = TProtoStructBuilder()
                              .Set("uid", model.GetUid())
                              .Set("session_id", model.GetSessionId())
                              .SetDouble("offset", model.GetOffset());
    if (const auto& alarmId = model.GetAlarmId(); alarmId.Defined()) {
        payloadBuilder.Set("alarm_id", alarmId.GetRef());
    }
    if (const auto& firstTrackId = model.GetFirstTrackId(); firstTrackId.Defined()) {
        payloadBuilder.Set("first_track_id", firstTrackId.GetRef());
    }

    auto directive = SerializeDirective(model);

    if (const auto* ioTUserInfo = SerializerMeta.GetIoTUserInfo().Get()) {
        if (const auto* locationInfo = model.GetLocationInfo().Get()) {
            FillRoomDeviceIds(ioTUserInfo->GetDevices(), *locationInfo, directive, SerializerMeta.GetClientInfo().DeviceId);
        } else if (const auto* roomId = model.GetRoomId().Get()) {
            FillRoomDeviceIds(ioTUserInfo->GetDevices(), *roomId, directive);
        } else if (SerializerMeta.GetClientInfo().IsSmartSpeaker()) {
            TryFillGroupDeviceIds(ioTUserInfo->GetDevices(), SerializerMeta.GetClientInfo().DeviceId, directive);
        }
    }

    *directive.MutablePayload() = payloadBuilder.Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TOpenDialogDirectiveModel& model) const {
    TProtoListBuilder directiveList{};
    for (const auto& payloadDirective : model.GetDirectives()) {
        directiveList.Add(SpeechKitStructSerializer.Serialize(*payloadDirective));
    }
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = TProtoStructBuilder()
                                      .Set("dialog_id", SerializerMeta.WrapDialogId(model.GetDialogId()))
                                      .Set("directives", directiveList.Build())
                                      .Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TOpenSettingsDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = TProtoStructBuilder().Set("target", ToString(model.GetTarget())).Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TPlayerRewindDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = TProtoStructBuilder()
                                      .SetUInt64("amount", model.GetAmount())
                                      .Set("type", ToString(model.GetRewindType()))
                                      .Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TSetCookiesDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = TProtoStructBuilder().Set("value", model.GetValue()).Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TSetSearchFilterDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = TProtoStructBuilder().Set("new_level", ToString(model.GetLevel())).Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TSetTimerDirectiveModel& model) const {
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
        for (const auto& payloadDirective : model.GetDirectives()) {
            directives.Add(SpeechKitStructSerializer.Serialize(*payloadDirective));
        }
        payload.Set("directives", directives.Build());
    }

    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = payload.Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TThereminPlayDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() =
        TProtoStructBuilder()
            .Set(ToString(model.GetSet()->GetType()), SpeechKitStructSerializer.Serialize(*model.GetSet()))
            .Build();
    return directive;
}

NSpeechKit::TDirective
TSpeechKitProtoSerializer::SerializeModel(const TTypedSemanticFrameRequestDirectiveModel& model) const {
    auto directive = SerializeDirective(model);

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

    *directive.MutablePayload() = payload.Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TUniversalClientDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = model.GetPayload();
    if (model.GetMultiroomSessionId().Defined()) {
        directive.SetMultiroomSessionId(model.GetMultiroomSessionId().GetRef());
    }

    const auto* ioTUserInfo = SerializerMeta.GetIoTUserInfo().Get();

    if (ioTUserInfo) {
        if (const auto* locationInfo = model.GetLocationInfo().Get()) {
            if (!locationInfo->GetCurrentGroup()) {
                FillRoomDeviceIds(ioTUserInfo->GetDevices(), *locationInfo, directive, SerializerMeta.GetClientInfo().DeviceId);
            } else if (SerializerMeta.GetClientInfo().IsSmartSpeaker()) {
                TryFillGroupDeviceIds(ioTUserInfo->GetDevices(), SerializerMeta.GetClientInfo().DeviceId, directive);
            }
        } else if (const auto* roomId = model.GetRoomId().Get()) {
            if (*roomId != IOT_GROUP_ROOM_ID) {
                FillRoomDeviceIds(ioTUserInfo->GetDevices(), *roomId, directive);
            } else if (SerializerMeta.GetClientInfo().IsSmartSpeaker()) {
                TryFillGroupDeviceIds(ioTUserInfo->GetDevices(), SerializerMeta.GetClientInfo().DeviceId, directive);
            }
        }
    }

    return directive;
}

NSpeechKit::TDirective
TSpeechKitProtoSerializer::SerializeModel(const TUpdateSpaceActionsDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = SpeechKitStructSerializer.Serialize(model.GetActionSpaces());
    return directive;
}

NSpeechKit::TDirective
TSpeechKitProtoSerializer::SerializeModel(const TAddConditionalActionsDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = SpeechKitStructSerializer.Serialize(model.GetConditionalActions());
    return directive;
}

NSpeechKit::TDirective
TSpeechKitProtoSerializer::SerializeModel(const TAddExternalEntitiesDescriptionDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = SpeechKitStructSerializer.Serialize(model.GetExternalEntitiesDescription());
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TShowButtonsDirectiveModel& model) const {
    TProtoStructBuilder payload;
    if (const auto& screenId = model.GetScreenId(); screenId.Defined()) {
        payload.Set("screen_id", *screenId);
    }

    TProtoListBuilder buttons;
    for (const auto& button : model.GetButtons()) {
        buttons.Add(SpeechKitStructSerializer.Serialize(*button));
    }
    payload.Set("buttons", buttons.Build());

    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = payload.Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TUniversalUniproxyDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = model.GetPayload();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TUpdateDialogInfoDirectiveModel& model) const {
    TProtoListBuilder menuItems{};
    for (const auto& menuItem : model.GetMenuItems()) {
        menuItems.Add(SpeechKitStructSerializer.Serialize(menuItem));
    }

    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = TProtoStructBuilder()
                                      .Set("image_url", model.GetImageUrl())
                                      .Set("url", model.GetUrl())
                                      .Set("title", model.GetTitle())
                                      .Set("style", SpeechKitStructSerializer.Serialize(model.GetStyle()))
                                      .Set("dark_style", SpeechKitStructSerializer.Serialize(model.GetDarkStyle()))
                                      .Set("menu_items", menuItems.Build())
                                      .Set("ad_block_id", model.GetAdBlockId())
                                      .Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TCallbackDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = GetCallbackPayload(model.GetPayload(), SerializerMeta, model.GetName()).Build();
    directive.SetIsLedSilent(model.GetIsLedSilent());
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TGetNextCallbackDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    TProtoStructBuilder payloadBuilder;
    if (const auto& recoveryCallback = model.GetRecoveryCallback(); recoveryCallback.Defined()) {
        payloadBuilder.Set(MM_STACK_ENGINE_RECOVERY_CALLBACK_FIELD_NAME, SpeechKitStructSerializer.Serialize(recoveryCallback.GetRef()));
    }
    auto getNextCallbackPayload = GetCallbackPayload(payloadBuilder.Build(), SerializerMeta, model.GetName());
    getNextCallbackPayload.Set(TString{MM_STACK_ENGINE_SESSION_ID}, model.GetSessionId());
    getNextCallbackPayload.Set(TString{MM_STACK_ENGINE_PRODUCT_SCENARIO_NAME}, model.GetProductScenarioName());
    *directive.MutablePayload() = getNextCallbackPayload.Build();
    directive.SetIsLedSilent(model.GetIsLedSilent());
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TDeferApplyDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    *directive.MutablePayload() = TProtoStructBuilder().Set("session", model.GetSession()).Build();
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TUpdateDatasyncDirectiveModel& model) const {
    auto directive = SerializeDirective(model);
    auto builder = TProtoStructBuilder{}
                       .Set("key", model.GetKey())
                       .Set("method", ToString(model.GetMethod()))
                       .SetBool("listening_is_possible", true);
    std::visit([&builder](const auto& value) {
        builder.Set("value", value);
    }, model.GetValue());
    *directive.MutablePayload() = builder.Build();

    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeModel(const TAudioPlayDirectiveModel& model) const {
    auto directive = SerializeDirective(model);

    auto scenarioMetaBuilder = TProtoStructBuilder();
    for(auto const& [key, value] : model.GetScenarioMeta()){
        scenarioMetaBuilder.Set(key, value);
    }
    scenarioMetaBuilder.Set(TString{SCENARIO_NAME_JSON_KEY}, SerializerMeta.GetScenarioName());

    auto builder = TProtoStructBuilder()
            .Set("stream", SpeechKitStructSerializer.Serialize(model.GetStream()))
            .Set("metadata", SpeechKitStructSerializer.Serialize(model.GetAudioPlayMetadata()))
            .Set("callbacks", SpeechKitStructSerializer.Serialize(model.GetCallbacks()))
            .Set("scenario_meta", scenarioMetaBuilder.Build())
            .Set("background_mode", ToString(model.GetBackgroundMode()))
            .Set("provider_name", ToString(model.GetProviderName()))
            .Set("screen_type", ToString(model.GetScreenType()))
            .SetBool("set_pause", model.GetSetPause());
    if (const auto& multiroomToken = model.GetMultiroomToken(); multiroomToken.Defined()) {
        builder.Set("multiroom_token", *multiroomToken);
    }

    *directive.MutablePayload() = builder.Build();
    return directive;
}

TSpeechKitResponseProto_TResponse_TButton
TSpeechKitProtoSerializer::SerializeBaseModel(const IButtonModel& model) const {
    TSpeechKitResponseProto_TResponse_TButton button;
    button.SetTitle(model.GetTitle());
    button.SetType(ToString(model.GetType()));
    return button;
}

TSpeechKitResponseProto_TResponse_TCard TSpeechKitProtoSerializer::SerializeBaseModel(const ICardModel& model) const {
    TSpeechKitResponseProto_TResponse_TCard card;
    card.SetType(ToString(model.GetType()));
    return card;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeBaseModel(const IDirectiveModel& model) const {
    NSpeechKit::TDirective directive;
    directive.SetName(model.GetName());
    directive.SetType(ToString(model.GetType()));
    *directive.MutablePayload() = google::protobuf::Struct();

    return directive;
}


NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeBaseDirective(const TBaseDirectiveModel& model) const {
    auto directive = SerializeBaseModel(model);

    if (const auto& endpointId = model.GetEndpointId(); endpointId) {
        *directive.MutableEndpointId()->mutable_value() = *endpointId;
    }

    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeDirective(const TClientDirectiveModel& model) const {
    auto directive = SerializeBaseDirective(model);
    directive.SetAnalyticsType(model.GetAnalyticsType());
    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeDirective(const TServerDirectiveModel& model) const {
    auto directive = SerializeBaseDirective(model);
    directive.SetIgnoreAnswer(model.GetIgnoreAnswer());

    if (model.GetMultiroomSessionId().Defined()) {
        directive.SetMultiroomSessionId(model.GetMultiroomSessionId().GetRef());
    }

    if (const auto& ioTUserInfo = SerializerMeta.GetIoTUserInfo().Get()) {
        if (const auto* locationInfo = model.GetLocationInfo().Get()) {
            FillRoomDeviceIds(ioTUserInfo->GetDevices(), *locationInfo, directive, SerializerMeta.GetClientInfo().DeviceId);
        } else if (const auto* roomId = model.GetRoomId().Get()) {
            FillRoomDeviceIds(ioTUserInfo->GetDevices(), *roomId, directive);
        }
    }

    return directive;
}

NSpeechKit::TDirective TSpeechKitProtoSerializer::SerializeDirective(const TUniproxyDirectiveModel& model) const {
    NSpeechKit::TDirective directive = SerializeBaseModel(model);
    if (const auto* uniproxyDirectiveMeta = model.GetUniproxyDirectiveMeta()) {
        *directive.MutableUniproxyDirectiveMeta() = *uniproxyDirectiveMeta;
    }
    return directive;
}

void TSpeechKitProtoSerializer::Visit(const TActionButtonModel& model) {
    Button = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TDiv2CardModel& model) {
    Card = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TDivCardModel& model) {
    Card = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TTextCardModel& model) {
    Card = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TTextWithButtonCardModel& model) {
    Card = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TAddContactBookAsrDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TAlarmNewDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TAlarmSetSoundDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TCloseDialogDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TEndDialogSessionDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TFindContactsDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TMementoChangeUserObjectsDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TMusicPlayDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TOpenDialogDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TOpenSettingsDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TPlayerRewindDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TSetCookiesDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TSetSearchFilterDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TSetTimerDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TThereminPlayDirectiveExternalSetModel& /* model */) {
    // Protocol doesn't have Message mapping for TThereminPlayDirectiveExternalSetModel
}

void TSpeechKitProtoSerializer::Visit(const TThereminPlayDirectiveInternalSetModel& /* model */) {
    // Protocol doesn't have Message mapping for TThereminPlayDirectiveInternalSetModel
}

void TSpeechKitProtoSerializer::Visit(const TThereminPlayDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TTypedSemanticFrameRequestDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TUniversalClientDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TUniversalUniproxyDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TUpdateDialogInfoDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TUpdateDialogInfoDirectiveMenuItemModel& /* model */) {
    // Protocol doesn't have Message mapping for TUpdateDialogInfoDirectiveStyleModel
}

void TSpeechKitProtoSerializer::Visit(const TUpdateDialogInfoDirectiveStyleModel& /* model */) {
    // Protocol doesn't have Message mapping for TUpdateDialogInfoDirectiveStyleModel
}

void TSpeechKitProtoSerializer::Visit(const TCallbackDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TGetNextCallbackDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TDeferApplyDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TUpdateDatasyncDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TAudioPlayDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TAudioPlayDirectiveMetadataModel& /* model */) {
    // Protocol doesn't have Message mapping for TAudioPlayDirectiveMetadataModel
}

void TSpeechKitProtoSerializer::Visit(const TAudioPlayDirectiveStreamModel& /* model */) {
    // Protocol doesn't have Message mapping for TAudioPlayDirectiveStreamModel
}

void TSpeechKitProtoSerializer::Visit(const TAudioPlayDirectiveCallbacksModel& /* model */) {
    // Protocol doesn't have Message mapping for TAudioPlayDirectiveCallbacksModel
}

void TSpeechKitProtoSerializer::Visit(const TAudioPlayDirectiveStreamNormalizationModel& /* model */) {
    // Protocol doesn't have Message mapping for TAudioPlayDirectiveCallbacksModel
}

void TSpeechKitProtoSerializer::Visit(const TMusicMetadataModel& /* model */) {
    // Protocol doesn't have Message mapping for TMusicMetadataModel
}

void TSpeechKitProtoSerializer::Visit(const TStubInnerGlagolMetadataModel& /* model */) {
    // Protocol doesn't have Message mapping for TStubInnerGlagolMetadataModel
}

void TSpeechKitProtoSerializer::Visit(const TGlagolMetadataModel& /* model */) {
    // Protocol doesn't have Message mapping for TGlagolMetadataModel
}

void TSpeechKitProtoSerializer::Visit(const TUpdateSpaceActionsDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TAddConditionalActionsDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TShowButtonsDirectiveModel& model) {
    Directive = SerializeModel(model);
}

void TSpeechKitProtoSerializer::Visit(const TProtobufUniproxyDirectiveModel& model) {
    const auto& ds = model.Directives();

    if (!ProtobufUniproxyDirective.Defined()) {
        ProtobufUniproxyDirective.ConstructInPlace();
    } else {
        ProtobufUniproxyDirective->Clear();
    }

    if (const auto* cancel = std::get_if<NScenarios::TCancelScheduledActionDirective>(&ds)) {
        auto& csd = *ProtobufUniproxyDirective->MutableContextSaveDirective();
        csd.MutablePayload()->PackFrom(cancel->GetRemoveScheduledActionRequest());
        // This is an item type
        // (https://a.yandex-team.ru/arc/trunk/arcadia/alice/matrix/scheduler/library/services/scheduler/protos/service.proto?rev=r9370130#L24)
        // and rename in graph
        // (https://a.yandex-team.ru/arc/trunk/arcadia/apphost/conf/verticals/VOICE/context_save.json?blame=true&rev=r9475788#L31)
        csd.SetDirectiveId("matrix_scheduler_remove_scheduled_action_request");
    } else if (const auto* enlist = std::get_if<NScenarios::TEnlistScheduledActionDirective>(&ds)) {
        auto& csd = *ProtobufUniproxyDirective->MutableContextSaveDirective();
        csd.MutablePayload()->PackFrom(enlist->GetAddScheduledActionRequest());
        // This is an item type:
        // (https://a.yandex-team.ru/svn/trunk/arcadia/alice/matrix/scheduler/library/services/scheduler/protos/service.proto?rev=r9370130#L15)
        // In graph:
        // (https://a.yandex-team.ru/svn/trunk/arcadia/apphost/conf/verticals/VOICE/context_save.json?rev=r9644879#L26)
        csd.SetDirectiveId("matrix_scheduler_add_scheduled_action_request");
    } else {
        ythrow yexception() << "No serialization for: " << model.GetName();
    }

    if (const auto* uniproxyDirectiveMeta = model.GetUniproxyDirectiveMeta()) {
        *ProtobufUniproxyDirective->MutableUniproxyDirectiveMeta() = *uniproxyDirectiveMeta;
    }
}

void TSpeechKitProtoSerializer::Visit(const TAddExternalEntitiesDescriptionDirectiveModel& model) {
    Directive = SerializeModel(model);
}

} // namespace NAlice::NMegamind
