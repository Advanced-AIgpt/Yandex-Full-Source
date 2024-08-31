#include "shared_resources.h"
#include "shared_ut.h"

#include <alice/megamind/library/models/directives/protobuf_uniproxy_directive_model.h>
#include <alice/megamind/library/serializers/scenario_proto_deserializer.h>

#include <alice/library/frame/builder.h>
#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>

#include <alice/megamind/protos/common/app_type.pb.h>
#include <alice/protos/div/div2patch.pb.h>
#include <alice/megamind/protos/common/origin.pb.h>
#include <alice/megamind/protos/common/permissions.pb.h>
#include <alice/memento/proto/api.pb.h>
#include <alice/protos/api/matrix/delivery.pb.h>
#include <alice/protos/api/matrix/schedule_action.pb.h>
#include <alice/protos/api/matrix/scheduler_api.pb.h>
#include <alice/protos/data/layer.pb.h>
#include <alice/protos/data/video/video.pb.h>
#include <alice/protos/div/div2card.pb.h>
#include <alice/protos/endpoint/capability.pb.h>
#include <alice/protos/endpoint/capabilities/div_view/capability.pb.h>
#include <alice/protos/endpoint/capabilities/layered_div_ui/capability.pb.h>
#include <alice/protos/data/scenario/centaur/teasers/teaser_settings.pb.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/serialized_enum.h>

#include <google/protobuf/util/json_util.h>

#include <utility>

using namespace NAlice::NScenarios;

namespace NAlice::NMegamind {

#if defined(ASSERT_DESERIALIZE_PROTO_MODEL)
#error ASSERT_DESERIALIZE_PROTO_MODEL is already defined!
#endif

#define ASSERT_DESERIALIZE_PROTO_MODEL_CUSTOM_DESERIALIZER(deserializer, Type, model, skJsonStringBuf)                \
    do {                                                                                                              \
        Type expectedProto;                                                                                           \
        google::protobuf::util::JsonParseOptions options;                                                             \
        options.ignore_unknown_fields = false;                                                                        \
        UNIT_ASSERT(                                                                                                  \
            google::protobuf::util::JsonStringToMessage(TString{skJsonStringBuf}, &expectedProto, options).ok());     \
        const auto& actualProto =                                                                                     \
            GetSpeechKitProtoModelSerializer().Serialize(*deserializer.Deserialize(model));                           \
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);                                                       \
    } while (false)

#define ASSERT_DESERIALIZE_PROTO_MODEL(Type, model, skJsonStringBuf)                                                  \
    ASSERT_DESERIALIZE_PROTO_MODEL_CUSTOM_DESERIALIZER(GetScenarioProtoDeserializer(), Type, model, skJsonStringBuf)

namespace {
    TDirective GetAudioPlayDirective(bool noCallbacks = false, bool someCallbacks = false) {
        TDirective directive{};
        auto& audioPlayDirective = *directive.MutableAudioPlayDirective();

        audioPlayDirective.SetName(TString{ ANALYTICS_TYPE });

        auto& stream = *audioPlayDirective.MutableStream();
        stream.SetStreamFormat(NScenarios::TAudioPlayDirective_TStream_TStreamFormat_MP3);
        stream.SetStreamType(NScenarios::TAudioPlayDirective_TStream_TStreamType_Track);
        stream.SetId("token");
        stream.SetOffsetMs(5000);
        stream.SetUrl("https://s3.yandex.net/record.mp3");

        auto& normalization = *stream.MutableNormalization();
        normalization.SetIntegratedLoudness(-0.7);
        normalization.SetTruePeak(-0.13);

        auto& metadata = *audioPlayDirective.MutableAudioPlayMetadata();
        metadata.SetArtImageUrl("https://img.jpg");
        metadata.SetTitle("title");
        metadata.SetSubTitle("subtitle");

        auto& musicMetadata = *metadata.MutableGlagolMetadata()->MutableMusicMetadata();
        musicMetadata.SetId("12345");
        musicMetadata.SetType(NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Track);
        musicMetadata.SetDescription("Описание");
        auto& prevTrackInfo = *musicMetadata.MutablePrevTrackInfo();
        prevTrackInfo.SetId("12344");
        prevTrackInfo.SetStreamType(NScenarios::TAudioPlayDirective_TStream_TStreamType_Track);
        auto& nextTrackInfo = *musicMetadata.MutableNextTrackInfo();
        nextTrackInfo.SetId("12346");
        nextTrackInfo.SetStreamType(NScenarios::TAudioPlayDirective_TStream_TStreamType_Track);
        musicMetadata.SetShuffled(true);
        musicMetadata.SetRepeatMode(NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode_All);

        if (!noCallbacks) {
            auto& callbacks = *audioPlayDirective.MutableCallbacks();

            auto& onStartedCallback = *callbacks.MutableOnPlayStartedCallback();
            onStartedCallback.SetName(TString{ "on_started" });
            *onStartedCallback.MutablePayload() = TProtoStructBuilder{}.Set(TString{ SKILL_ID_KEY }, TString{ SKILL_ID_VALUE }).Build();
            onStartedCallback.SetIgnoreAnswer(true);
            onStartedCallback.SetIsLedSilent(true);

            auto& onStoppedCallback = *callbacks.MutableOnPlayStoppedCallback();
            onStoppedCallback.SetName(TString{ "on_stopped" });
            *onStoppedCallback.MutablePayload() = TProtoStructBuilder{}.Set(TString{ SKILL_ID_KEY }, TString{ SKILL_ID_VALUE }).Build();
            onStoppedCallback.SetIgnoreAnswer(true);
            onStoppedCallback.SetIsLedSilent(true);

            if (!someCallbacks) {
                auto& onFinishedCallback = *callbacks.MutableOnPlayFinishedCallback();
                onFinishedCallback.SetName(TString{ "on_finished" });
                *onFinishedCallback.MutablePayload() = TProtoStructBuilder{}.Set(TString{ SKILL_ID_KEY }, TString{ SKILL_ID_VALUE }).Build();
                onFinishedCallback.SetIgnoreAnswer(true);
                onFinishedCallback.SetIsLedSilent(true);

                auto& onFailedCallback = *callbacks.MutableOnFailedCallback();
                onFailedCallback.SetName(TString{ "on_failed" });
                *onFailedCallback.MutablePayload() = TProtoStructBuilder{}.Set(TString{ SKILL_ID_KEY }, TString{ SKILL_ID_VALUE }).Build();
                onFailedCallback.SetIgnoreAnswer(true);
                onFailedCallback.SetIsLedSilent(true);
            }
        }

        auto& scenarioMeta = *audioPlayDirective.MutableScenarioMeta();
        scenarioMeta.insert({ TString{SKILL_ID_KEY}, TString{SKILL_ID_VALUE} });

        audioPlayDirective.SetBackgroundMode(NScenarios::TAudioPlayDirective_TBackgroundMode_Ducking);
        audioPlayDirective.SetProviderName(TString{ "ЛитРес" });
        audioPlayDirective.SetScreenType(NScenarios::TAudioPlayDirective_EScreenType_Default);
        audioPlayDirective.SetMultiroomToken(TEST_MULTIROOM_TOKEN.data(), TEST_MULTIROOM_TOKEN.size());

        return directive;
    }
}

Y_UNIT_TEST_SUITE_F(TScenarioProtoDeserializer, TFixture) {
    Y_UNIT_TEST(TestDeserializeTActionButtonModelWithDirectives) {
        TLayout_TButton button;
        button.SetTitle(TString{TITLE});
        button.SetActionId(TString{ACTION_ID});

        THashMap<TString, TFrameAction> actions{};
        TFrameAction action;
        action.MutableDirectives()->AddList()->MutableStartImageRecognizerDirective()->SetName(
            TString{INNER_ANALYTICS_TYPE});
        actions.insert({TString{ACTION_ID}, action});

        TScenarioProtoDeserializer deserializer(GetSerializerMeta(), TRTLogger::StderrLogger(), std::move(actions));

        TSpeechKitResponseProto_TResponse_TButton expectedProto;
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = false;
        UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(TString{ACTION_BUTTON_WITH_ON_SUGGEST}, &expectedProto,
                                                                options)
                        .ok());
        const auto& actualProto = GetSpeechKitProtoModelSerializer().Serialize(*deserializer.Deserialize(button));
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);
    }

    Y_UNIT_TEST(TestDeserializeTThemedActionButtonModelWithDirectives) {
        TLayout_TButton button;
        button.SetTitle(TString{TITLE});
        button.SetActionId(TString{ACTION_ID});
        button.MutableTheme()->SetImageUrl(TString{IMAGE_URL});

        THashMap<TString, TFrameAction> actions{};
        TFrameAction action;
        action.MutableDirectives()->AddList()->MutableStartImageRecognizerDirective()->SetName(
            TString{INNER_ANALYTICS_TYPE});
        actions.insert({TString{ACTION_ID}, action});

        TScenarioProtoDeserializer deserializer(GetSerializerMeta(), TRTLogger::StderrLogger(), std::move(actions));
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = false;

        {
            TSpeechKitResponseProto_TResponse_TButton expectedProto;
            UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(TString{THEMED_ACTION_BUTTON_WITH_ON_SUGGEST},
                                                                    &expectedProto, options)
                            .ok());
            const auto& actualProto = GetSpeechKitProtoModelSerializer().Serialize(
                *deserializer.Deserialize(button, /* fromSuggest= */ true));
            UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);
        }

        {
            TSpeechKitResponseProto_TResponse_TButton expectedProto;
            UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(TString{ACTION_BUTTON_WITH_ON_SUGGEST},
                                                                    &expectedProto, options)
                            .ok());
            const auto& actualProto = GetSpeechKitProtoModelSerializer().Serialize(
                *deserializer.Deserialize(button, /* fromSuggest= */ false));
            UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);
        }
    }

    Y_UNIT_TEST(TestDeserializeTActionButtonModelWithCallback) {
        TLayout_TButton button;
        button.SetTitle(TString{TITLE});
        button.SetActionId(TString{ACTION_ID});

        TFrameAction action;
        action.MutableCallback()->SetName(TString{TEXT});
        action.MutableCallback()->SetIsLedSilent(true);
        (*action.MutableCallback()->MutablePayload()->mutable_fields())[TString{KEY}].set_string_value(TString{VALUE});
        THashMap<TString, TFrameAction> actions{{TString{ACTION_ID}, action}};

        TScenarioProtoDeserializer deserializer(GetSerializerMeta(), TRTLogger::StderrLogger(), std::move(actions));

        TSpeechKitResponseProto_TResponse_TButton expectedProto;
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = false;
        UNIT_ASSERT(
            google::protobuf::util::JsonStringToMessage(TString{ACTION_BUTTON_WITH_CALLBACK}, &expectedProto, options)
                .ok());
        const auto& actualProto = GetSpeechKitProtoModelSerializer().Serialize(*deserializer.Deserialize(button));
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);
    }

    Y_UNIT_TEST(TestDeserializeTDiv2CardModel) {
        TLayout_TCard card;
        google::protobuf::Value value;
        *value.mutable_string_value() = TString{VALUE};
        card.mutable_div2card()->mutable_fields()->insert({TString{KEY}, value});

        ASSERT_DESERIALIZE_PROTO_MODEL(TSpeechKitResponseProto_TResponse_TCard, card, DIV2_CARD);
    }

    Y_UNIT_TEST(TestDeserializeTDiv2CardModelWithoutBorders) {
        TLayout_TCard card;
        google::protobuf::Value value;
        *value.mutable_string_value() = TString{VALUE};
        card.mutable_div2cardextended()->mutable_body()->mutable_fields()->insert({TString{KEY}, value});
        card.mutable_div2cardextended()->set_hideborders(true);

        ASSERT_DESERIALIZE_PROTO_MODEL(TSpeechKitResponseProto_TResponse_TCard, card, DIV2_CARD_WITHOUT_BORDERS);
    }

    Y_UNIT_TEST(TestDeserializeTDivCardModel) {
        TLayout_TCard card;
        google::protobuf::Value value;
        *value.mutable_string_value() = TString{VALUE};
        card.mutable_divcard()->mutable_fields()->insert({TString{KEY}, value});

        ASSERT_DESERIALIZE_PROTO_MODEL(TSpeechKitResponseProto_TResponse_TCard, card, DIV_CARD);
    }

    Y_UNIT_TEST(TestDeserializeTDivCardModelWithEmptyElements) {
        TLayout_TCard card;
        google::protobuf::Value value;
        auto* listElements = value.mutable_list_value();
        *listElements->add_values()->mutable_struct_value() = google::protobuf::Struct();
        auto* elem = listElements->add_values()->mutable_struct_value();
        google::protobuf::Value subvalue;
        subvalue.set_string_value("value");
        elem->mutable_fields()->insert({"key", subvalue});
        card.mutable_divcard()->mutable_fields()->insert({TString{KEY}, value});

        TScenarioProtoDeserializer deserializer(GetSerializerMeta(), TRTLogger::StderrLogger(), {});
        const auto& actual = deserializer.Deserialize(card);
        const auto* modelPtr = dynamic_cast<TDivCardModel*>(actual.Get());
        UNIT_ASSERT_MESSAGES_EQUAL(modelPtr->GetBody(), card.GetDivCard());
    }

    Y_UNIT_TEST(TestDeserializeTDivCardModelWithDeepLinks) {
        TLayout_TCard card;
        const TString deepLink = TString{MM_DEEPLINK_PLACEHOLDER} + TString{ACTION_ID};
        *card.mutable_divcard() = TProtoStructBuilder{}
                                      .Set(TString{URI}, deepLink)
                                      .Set(TString{KEY}, TProtoListBuilder{}.Add(deepLink).Build())
                                      .Build();

        THashMap<TString, TFrameAction> actions{};
        TFrameAction action;
        action.MutableDirectives()->AddList()->MutableStartImageRecognizerDirective()->SetName(
            TString{INNER_ANALYTICS_TYPE});
        actions.insert({TString{ACTION_ID}, action});

        TScenarioProtoDeserializer deserializer(GetSerializerMeta(), TRTLogger::StderrLogger(), std::move(actions));

        TSpeechKitResponseProto_TResponse_TCard expectedProto;
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = false;
        UNIT_ASSERT(
            google::protobuf::util::JsonStringToMessage(TString{DIV_CARD_WITH_DEEP_LINKS}, &expectedProto, options)
                .ok());
        const auto& actualProto = GetSpeechKitProtoModelSerializer().Serialize(*deserializer.Deserialize(card));
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);
    }

    Y_UNIT_TEST(TestDeserializeTDivCardModelWithComplexDeepLinks) {
        TLayout_TCard card;
        const TString deepLink = TString{MM_DEEPLINK_PLACEHOLDER} + TString{ACTION_ID};
        *card.mutable_divcard() =
            TProtoStructBuilder{}
                .Set(TString{URI}, deepLink)
                .Set(TString{KEY},
                     TProtoStructBuilder{}
                         .Set(TString{KEY}, TProtoListBuilder{}.Add(deepLink).Build())
                         .Set(TString{URI}, TProtoListBuilder{}
                                                .Add(TProtoStructBuilder{}
                                                         .Set(TString{KEY}, TProtoListBuilder{}.Add(deepLink).Build())
                                                         .Build())
                                                .Build())
                         .Set(TString{IMAGE_URL},
                              TProtoListBuilder{}.AddDouble(1).Add("1").AddBool(true).AddNull().Build())
                         .Build())
                .Set(TString{IMAGE_URL}, TProtoStructBuilder{}
                                             .SetDouble(TString{DIALOG_ID}, 1)
                                             .SetBool(TString{KEY}, true)
                                             .SetNull(TString{COMMAND})
                                             .Build())
                .Build();

        THashMap<TString, TFrameAction> actions{};
        TFrameAction action;
        action.MutableDirectives()->AddList()->MutableStartImageRecognizerDirective()->SetName(
            TString{INNER_ANALYTICS_TYPE});
        actions.insert({TString{ACTION_ID}, action});

        TScenarioProtoDeserializer deserializer(GetSerializerMeta(), TRTLogger::StderrLogger(), std::move(actions));

        TSpeechKitResponseProto_TResponse_TCard expectedProto;
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = false;
        UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(TString{DIV_CARD_WITH_COMPLEX_DEEP_LINKS},
                                                                &expectedProto, options)
                        .ok());
        const auto& actualProto = GetSpeechKitProtoModelSerializer().Serialize(*deserializer.Deserialize(card));
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);
    }

    Y_UNIT_TEST(TestDeserializeTDivCardModelWithInvalidDeepLinks) {
        TLayout_TCard card;
        const TString deepLink = TString{MM_DEEPLINK_PLACEHOLDER} + TString{URI};
        *card.mutable_divcard() = TProtoStructBuilder{}
                                      .Set(TString{URI}, deepLink)
                                      .Set(TString{KEY}, TProtoListBuilder{}.Add(deepLink).Build())
                                      .Build();

        THashMap<TString, TFrameAction> actions{};
        TFrameAction action;
        action.MutableDirectives()->AddList()->MutableStartImageRecognizerDirective()->SetName(
            TString{INNER_ANALYTICS_TYPE});
        actions.insert({TString{ACTION_ID}, action});

        TScenarioProtoDeserializer deserializer(GetSerializerMeta(), TRTLogger::StderrLogger(), std::move(actions));

        TSpeechKitResponseProto_TResponse_TCard expectedProto;
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = false;
        UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(TString{DIV_CARD_WITH_INVALID_DEEP_LINKS},
                                                                &expectedProto, options)
                        .ok());
        const auto& actualProto = GetSpeechKitProtoModelSerializer().Serialize(*deserializer.Deserialize(card));
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);
    }

    Y_UNIT_TEST(TestDeserializeTDivCardModelWithCustomEscapes) {
        TLayout_TCard card;
        const TString deepLink = TString{MM_DEEPLINK_PLACEHOLDER} + TString{ACTION_ID};
        *card.mutable_divcard() = TProtoStructBuilder{}.Set(TString{URI}, deepLink).Build();

        THashMap<TString, TFrameAction> actions{};
        TFrameAction action;
        auto* directive = action.MutableDirectives()->AddList()->MutableCallbackDirective();
        directive->SetName(TString{UNESCAPED_CHARS});
        *directive->MutablePayload() =
            TProtoStructBuilder().Set(TString{UNESCAPED_CHARS}, TString{UNESCAPED_CHARS}).Build();
        actions.insert({TString{ACTION_ID}, action});

        TScenarioProtoDeserializer deserializer(GetSerializerMeta(), TRTLogger::StderrLogger(), std::move(actions));

        TSpeechKitResponseProto_TResponse_TCard expectedProto;
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = false;
        UNIT_ASSERT(
            google::protobuf::util::JsonStringToMessage(TString{DIV_CARD_WITH_CUSTOM_ESCAPES}, &expectedProto, options)
                .ok());
        const auto& actualProto = GetSpeechKitProtoModelSerializer().Serialize(*deserializer.Deserialize(card));
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);
    }

    Y_UNIT_TEST(TestDeserializeTTextCardModel) {
        TLayout_TCard card;
        card.SetText(TString{TEXT});

        ASSERT_DESERIALIZE_PROTO_MODEL(TSpeechKitResponseProto_TResponse_TCard, card, TEXT_CARD);
    }

    Y_UNIT_TEST(TestDeserializeTTextWithButtonCardModel) {
        TLayout_TCard card;
        card.MutableTextWithButtons()->SetText(TString{TEXT});
        card.MutableTextWithButtons()->AddButtons()->SetTitle(TString{TITLE});

        ASSERT_DESERIALIZE_PROTO_MODEL(TSpeechKitResponseProto_TResponse_TCard, card,
                                       TEXT_WITH_BUTTON_WITH_ON_SUGGEST);
    }

    Y_UNIT_TEST(TestDeserializeTCallbackDirectiveModel) {
        google::protobuf::Struct payload;
        google::protobuf::Value value;
        *value.mutable_string_value() = TString{VALUE};
        (*payload.mutable_fields())[TString{KEY}] = value;

        TDirective directive;
        directive.MutableCallbackDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableCallbackDirective()->SetIgnoreAnswer(true);
        directive.MutableCallbackDirective()->SetIsLedSilent(true);
        *directive.MutableCallbackDirective()->MutablePayload() = payload;

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, CALLBACK_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTCloseDialogDirectiveModel) {
        TDirective directive;
        directive.MutableCloseDialogDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableCloseDialogDirective()->SetDialogId(TString{DIALOG_ID});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, CLOSE_DIALOG_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTCloseDialogDirectiveModelWithScreenId) {
        TDirective directive;
        directive.MutableCloseDialogDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableCloseDialogDirective()->SetDialogId(TString{DIALOG_ID});
        directive.mutable_closedialogdirective()->SetScreenId("test_screen_id");
        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, CLOSE_DIALOG_DIRECTIVE_WITH_SCREEN_ID);
    }

    Y_UNIT_TEST(TestDeserializeTEndDialogSessionDirectiveModel) {
        TDirective directive;
        directive.MutableEndDialogSessionDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableEndDialogSessionDirective()->SetDialogId(TString{DIALOG_ID});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, END_DIALOG_SESSION_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTMordoviaCommandDirectiveModel) {
        TDirective directive;
        directive.MutableMordoviaCommandDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableMordoviaCommandDirective()->SetCommand(TString{COMMAND});
        *directive.MutableMordoviaCommandDirective()->mutable_meta() =
            TProtoStructBuilder().Set("TestKey", "TestValue").Build();
        directive.MutableMordoviaCommandDirective()->SetViewKey(TString{VIEW_KEY});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MORDOVIA_COMMAND_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTMordoviaShowDirectiveModel) {
        TDirective directive;
        directive.MutableMordoviaShowDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableMordoviaShowDirective()->SetUrl(TString{URI});
        directive.MutableMordoviaShowDirective()->SetIsFullScreen(true);
        directive.MutableMordoviaShowDirective()->SetViewKey(TString{VIEW_KEY});
        directive.MutableMordoviaShowDirective()->SetSplashDiv(TString{SPLASH_DIV});
        TCallbackDirective& callbackDirective = *directive.MutableMordoviaShowDirective()->MutableCallbackPrototype();
        callbackDirective.SetName("some name");
        callbackDirective.SetIgnoreAnswer(true);
        callbackDirective.SetIsLedSilent(true);
        (*callbackDirective.MutablePayload()->mutable_fields())["key"].set_string_value("value");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MORDOVIA_SHOW_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeRemindersCreateDirecive) {
        TDirective directive;
        auto& reminderDirective = *directive.MutableRemindersSetDirective();
        reminderDirective.SetName(ToString(ANALYTICS_TYPE));
        reminderDirective.SetId("guid");
        reminderDirective.SetText("remind me this");
        reminderDirective.SetEpoch(ToString(12345678));
        reminderDirective.SetTimeZone("Europe/Moscow");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, REMINDERS_CREATE_DIRECTIVE);

        auto& onSuccess = *reminderDirective.MutableOnSuccessCallback();
        onSuccess.SetName("reminders_on_success_callback");
        onSuccess.SetIgnoreAnswer(false);
        onSuccess.SetIsLedSilent(false);
        (*onSuccess.MutablePayload()->mutable_fields())["key"].set_string_value("value");

        auto& onFail = *reminderDirective.MutableOnFailCallback();
        onFail.SetName("reminders_on_fail_callback");
        onFail.SetIgnoreAnswer(false);
        onFail.SetIsLedSilent(false);
        (*onFail.MutablePayload()->mutable_fields())["key1"].set_string_value("value1");

        auto& onShoot = *reminderDirective.MutableOnShootFrame();
        auto& onShootFrame = *onShoot.MutableTypedSemanticFrame()->MutableRemindersOnShootSemanticFrame();
        onShootFrame.MutableId()->SetStringValue("guid");
        onShootFrame.MutableText()->SetStringValue("remind me this");
        onShootFrame.MutableEpoch()->SetEpochValue("1234567");
        onShootFrame.MutableTimeZone()->SetStringValue("Europe/Moscow");
        auto& onShootAnalytics = *onShoot.MutableAnalytics();
        onShootAnalytics.SetProductScenario("reminders");
        onShootAnalytics.SetOrigin(TAnalyticsTrackingModule::Scenario);
        onShootAnalytics.SetPurpose("alice.reminders.shoot");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, REMINDERS_CREATE_DIRECTIVE_FULL);
    }

    Y_UNIT_TEST(TestDeserializeRemindersCancelDirecive) {
        TDirective directive;
        auto& reminderDirective = *directive.MutableRemindersCancelDirective();
        reminderDirective.SetName(ToString(ANALYTICS_TYPE));
        reminderDirective.SetAction("id");
        reminderDirective.MutableIds()->Add("guid");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, REMINDERS_CANCEL_DIRECTIVE);

        auto& onSuccess = *reminderDirective.MutableOnSuccessCallback();
        onSuccess.SetName("reminders_on_success_callback");
        onSuccess.SetIgnoreAnswer(false);
        onSuccess.SetIsLedSilent(false);
        (*onSuccess.MutablePayload()->mutable_fields())["key"].set_string_value("value");

        auto& onFail = *reminderDirective.MutableOnFailCallback();
        onFail.SetName("reminders_on_fail_callback");
        onFail.SetIgnoreAnswer(false);
        onFail.SetIsLedSilent(false);
        (*onFail.MutablePayload()->mutable_fields())["key1"].set_string_value("value1");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, REMINDERS_CANCEL_DIRECTIVE_FULL);
    }

    Y_UNIT_TEST(TestDeserializeTMusicPlayDirectiveModel) {
        TDirective directive;
        directive.MutableMusicPlayDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableMusicPlayDirective()->SetUid(TString{UID});
        directive.MutableMusicPlayDirective()->SetSessionId(TString{SESSION_ID});
        directive.MutableMusicPlayDirective()->SetOffset(42);
        directive.MutableMusicPlayDirective()->SetAlarmId("TestAlarmId");
        directive.MutableMusicPlayDirective()->SetFirstTrackId("TestFirstTrackId");
        directive.MutableMusicPlayDirective()->SetRoomId("room_1");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MUSIC_PLAY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTMusicPlayDirectiveModelWithEndpointId) {
        TDirective directive;
        directive.MutableMusicPlayDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableMusicPlayDirective()->SetUid(TString{UID});
        directive.MutableMusicPlayDirective()->SetSessionId(TString{SESSION_ID});
        directive.MutableMusicPlayDirective()->SetOffset(42);
        directive.MutableMusicPlayDirective()->SetAlarmId("TestAlarmId");
        directive.MutableMusicPlayDirective()->SetFirstTrackId("TestFirstTrackId");
        directive.MutableMusicPlayDirective()->SetRoomId("room_1");
        *directive.MutableEndpointId()->mutable_value() = "kekid";

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MUSIC_PLAY_DIRECTIVE_WITH_ENDPOINT_ID);
    }

    Y_UNIT_TEST(TestDeserializeTMusicPlayDirectiveModel2) {
        TDirective directive;
        directive.MutableMusicPlayDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableMusicPlayDirective()->SetUid(TString{UID});
        directive.MutableMusicPlayDirective()->SetSessionId(TString{SESSION_ID});
        directive.MutableMusicPlayDirective()->SetOffset(42);
        directive.MutableMusicPlayDirective()->SetRoomId("room_2");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MUSIC_PLAY_DIRECTIVE_2);
    }

    Y_UNIT_TEST(TestDeserializeTMusicPlayDirectiveModelWithLocationInfo) {
        TDirective directive;
        directive.MutableMusicPlayDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableMusicPlayDirective()->SetUid(TString{UID});
        directive.MutableMusicPlayDirective()->SetSessionId(TString{SESSION_ID});
        directive.MutableMusicPlayDirective()->SetOffset(42);

        auto& locationInfo = *directive.MutableMusicPlayDirective()->MutableLocationInfo();
        locationInfo.AddRoomsIds("room_1");
        locationInfo.AddGroupsIds("group_2");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MUSIC_PLAY_DIRECTIVE_WITH_LOCATION_INFO);
    }

    Y_UNIT_TEST(TestDeserializeTOpenDialogDirectiveModel) {
        TDirective directive;
        directive.MutableOpenDialogDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableOpenDialogDirective()->SetDialogId(TString{DIALOG_ID});
        directive.MutableOpenDialogDirective()->AddDirectives()->MutableStartImageRecognizerDirective()->SetName(
            TString{INNER_ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, OPEN_DIALOG_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTOpenSettingsDirectiveModel) {
        TDirective directive;
        directive.MutableOpenSettingsDirective()->SetName(TString{ANALYTICS_TYPE});
        int targetId = 0;
        for (const auto target : GetEnumAllValues<ESettingsTarget>()) {
            directive.MutableOpenSettingsDirective()->SetTarget(
                static_cast<NScenarios::TOpenSettingsDirective::ESettingsTarget>(targetId++));
            ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, GetOpenSettingsSkJsonString(target));
        }
    }

    Y_UNIT_TEST(TestDeserializeTOpenUriDirective) {
        TDirective directive;
        directive.MutableOpenUriDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableOpenUriDirective()->SetUri(TString{URI});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, OPEN_URI_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTOpenUriDirectiveWithScreenId) {
        TDirective directive;
        directive.MutableOpenUriDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableOpenUriDirective()->SetUri(TString{URI});
        directive.MutableOpenUriDirective()->SetScreenId(TString{SCREEN_ID});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, OPEN_URI_DIRECTIVE_WITH_SCREEN_ID);
    }

    Y_UNIT_TEST(TestDeserializeTPlayerRewindDirectiveModel) {
        TDirective directive;
        directive.MutablePlayerRewindDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutablePlayerRewindDirective()->SetAmount(30);
        directive.MutablePlayerRewindDirective()->SetType(TPlayerRewindDirective::Absolute);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PLAYER_REWIND_DIRECTIVE_ABSOLUTE);

        directive.MutablePlayerRewindDirective()->SetType(TPlayerRewindDirective::Backward);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PLAYER_REWIND_DIRECTIVE_BACKWARD);

        directive.MutablePlayerRewindDirective()->SetType(TPlayerRewindDirective::Forward);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PLAYER_REWIND_DIRECTIVE_FORWARD);
    }

    Y_UNIT_TEST(TestDeserializeTSetCookiesDirectiveModel) {
        TDirective directive;
        directive.MutableSetCookiesDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableSetCookiesDirective()->SetValue(
            R"({"uaas": "t123456.e1600000000.sDEADBEEF123456DEADBEEF123456DEAD"})");
        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SET_COOKIES_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTSetSearchFilterDirectiveModel) {
        TDirective directive;
        directive.MutableSetSearchFilterDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableSetSearchFilterDirective()->SetLevel(
            TSetSearchFilterDirective::ESearchLevel::TSetSearchFilterDirective_ESearchLevel_None);
        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SET_SEARCH_FILTER_DIRECTIVE_NONE);
        directive.MutableSetSearchFilterDirective()->SetLevel(
            TSetSearchFilterDirective::ESearchLevel::TSetSearchFilterDirective_ESearchLevel_Strict);
        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SET_SEARCH_FILTER_DIRECTIVE_STRICT);
        directive.MutableSetSearchFilterDirective()->SetLevel(
            TSetSearchFilterDirective::ESearchLevel::TSetSearchFilterDirective_ESearchLevel_Moderate);
        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SET_SEARCH_FILTER_DIRECTIVE_MODERATE);
    }

    Y_UNIT_TEST(TestDeserializeTSetTimerDirectiveModel) {
        TDirective directive;

        directive.MutableSetTimerDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableSetTimerDirective()->SetDuration(42);
        directive.MutableSetTimerDirective()->SetListeningIsPossible(true);
        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SET_TIMER_DIRECTIVE);

        directive.MutableSetTimerDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableSetTimerDirective()->SetTimestamp(42);
        *directive.MutableSetTimerDirective()->MutableOnSuccessCallbackPayload() =
            TProtoStructBuilder().Set(TString{KEY}, TString{VALUE}).Build();
        *directive.MutableSetTimerDirective()->MutableOnFailureCallbackPayload() =
            TProtoStructBuilder().Set(TString{TITLE}, TString{INNER_TITLE}).Build();

        auto& innerDirective = *directive.MutableSetTimerDirective()->AddDirectives();
        innerDirective.MutableTypeTextDirective()->SetName(TString{ANALYTICS_TYPE});
        innerDirective.MutableTypeTextDirective()->SetText(TString{TEXT});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SET_TIMER_TIMESTAMP_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTSoundSetLevelDirectiveModel) {
        TDirective directive;
        directive.MutableSoundSetLevelDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableSoundSetLevelDirective()->SetNewLevel(7);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SOUND_SET_LEVEL_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTSoundSetLevelInLocationDirectiveModel) {
        TDirective directive;
        directive.MutableSoundSetLevelDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableSoundSetLevelDirective()->SetNewLevel(6);
        directive.MutableSoundSetLevelDirective()->SetRoomId("kitchen");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SOUND_SET_LEVEL_DIRECTIVE_IN_LOCATION);
    }

    Y_UNIT_TEST(TestDeserializeTSoundSetLevelDirectiveModelWithMultiroom) {
        TDirective directive;
        directive.MutableSoundSetLevelDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableSoundSetLevelDirective()->SetNewLevel(7);
        directive.MutableSoundSetLevelDirective()->SetMultiroomSessionId("12345");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SOUND_SET_LEVEL_DIRECTIVE_WITH_MULTIROOM);
    }

    Y_UNIT_TEST(TestDeserializeTStartImageRecognizerDirectiveModel) {
        TDirective directive;
        auto& recognizer = *directive.MutableStartImageRecognizerDirective();
        recognizer.SetName(TString{ANALYTICS_TYPE});
        recognizer.SetCameraType("front");
        recognizer.SetImageSearchMode(1);
        recognizer.SetImageSearchModeName("market");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, START_IMAGE_RECOGNIZER_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTThereminPlayDirectiveModelWithExternalSet) {
        TDirective directive;
        directive.MutableThereminPlayDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableThereminPlayDirective()->MutableExternalSet()->SetStopOnCeil(true);
        directive.MutableThereminPlayDirective()->MutableExternalSet()->SetRepeatSoundInside(true);
        directive.MutableThereminPlayDirective()->MutableExternalSet()->SetNoOverlaySamples(true);
        directive.MutableThereminPlayDirective()->MutableExternalSet()->AddSamples()->SetUrl(TString{URI});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, THEREMIN_PLAY_DIRECTIVE_WITH_EXTERNAL_SET);
    }

    Y_UNIT_TEST(TestDeserializeTThereminPlayDirectiveModelWithInternalSet) {
        TDirective directive;
        directive.MutableThereminPlayDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableThereminPlayDirective()->MutableInternalSet()->SetMode(324);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, THEREMIN_PLAY_DIRECTIVE_WITH_INTERNAL_SET);
    }

    Y_UNIT_TEST(TestDeserializeTTypeTextDirectiveModel) {
        TDirective directive;
        directive.MutableTypeTextDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableTypeTextDirective()->SetText(TString{TEXT});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, TYPE_TEXT_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTTypeTextSilentDirectiveModel) {
        TDirective directive;
        directive.MutableTypeTextSilentDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableTypeTextSilentDirective()->SetText(TString{TEXT});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, TYPE_TEXT_SILENT_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTUpdateDialogInfoDirective) {
        TDirective directive;
        directive.MutableUpdateDialogInfoDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableUpdateDialogInfoDirective()->SetTitle(TString{TITLE});
        directive.MutableUpdateDialogInfoDirective()->SetUrl(TString{URI});
        directive.MutableUpdateDialogInfoDirective()->SetImageUrl(TString{IMAGE_URL});
        directive.MutableUpdateDialogInfoDirective()->SetAdBlockId(TString{AD_BLOCK_ID});

        auto menuItem = directive.MutableUpdateDialogInfoDirective()->AddMenuItems();
        menuItem->SetTitle(TString{INNER_TITLE});
        menuItem->SetUrl(TString{INNER_URL});

        auto style = directive.MutableUpdateDialogInfoDirective()->MutableStyle();
        style->SetSuggestBorderColor("TestSuggestBorderColor");
        style->SetUserBubbleFillColor("TestUserBubbleFillColor");
        style->SetSuggestTextColor("TestSuggestTextColor");
        style->SetSuggestFillColor("TestSuggestFillColor");
        style->SetUserBubbleTextColor("TestUserBubbleTextColor");
        style->SetSkillActionsTextColor("TestSkillActionsTextColor");
        style->SetSkillBubbleFillColor("TestSkillBubbleFillColor");
        style->SetSkillBubbleTextColor("TestSkillBubbleTextColor");
        style->SetOknyxLogo("TestOknyxLogo");

        style->AddOknyxErrorColors("TestOknyxErrorColor");
        style->AddOknyxNormalColors("TestOknyxNormalColor");

        auto darkStyle = directive.MutableUpdateDialogInfoDirective()->MutableDarkStyle();
        darkStyle->SetSuggestBorderColor("TestSuggestBorderColorDark");
        darkStyle->SetUserBubbleFillColor("TestUserBubbleFillColorDark");
        darkStyle->SetSuggestTextColor("TestSuggestTextColorDark");
        darkStyle->SetSuggestFillColor("TestSuggestFillColorDark");
        darkStyle->SetUserBubbleTextColor("TestUserBubbleTextColorDark");
        darkStyle->SetSkillActionsTextColor("TestSkillActionsTextColorDark");
        darkStyle->SetSkillBubbleFillColor("TestSkillBubbleFillColorDark");
        darkStyle->SetSkillBubbleTextColor("TestSkillBubbleTextColorDark");
        darkStyle->SetOknyxLogo("TestOknyxLogoDark");

        darkStyle->AddOknyxErrorColors("TestOknyxErrorColorDark");
        darkStyle->AddOknyxNormalColors("TestOknyxNormalColorDark");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, UPDATE_DIALOG_INFO_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeUnrecognizedDirectiveReturnsNullptr) {
        TDirective directive;
        UNIT_ASSERT(!GetScenarioProtoDeserializer().Deserialize(directive));
    }

    Y_UNIT_TEST(TestDeserializeNavigateBrowserDirectiveModel) {
        for (const auto& [command, commandName] : THashMap<NScenarios::TNavigateBrowserDirective::ECommand, TString>{
                 {NScenarios::TNavigateBrowserDirective_ECommand_ClearHistory, "clear_history"},
                 {NScenarios::TNavigateBrowserDirective_ECommand_CloseBrowser, "close_browser"},
                 {NScenarios::TNavigateBrowserDirective_ECommand_GoHome, "go_home"},
                 {NScenarios::TNavigateBrowserDirective_ECommand_NewTab, "new_tab"},
                 {NScenarios::TNavigateBrowserDirective_ECommand_OpenBookmarksManager, "open_bookmarks_manager"},
                 {NScenarios::TNavigateBrowserDirective_ECommand_OpenHistory, "open_history"},
                 {NScenarios::TNavigateBrowserDirective_ECommand_OpenIncognitoMode, "open_incognito_mode"},
                 {NScenarios::TNavigateBrowserDirective_ECommand_RestoreTab, "restore_tab"},
             }) {
            const auto expected = Sprintf(TString{NAVIGATE_BROWSER_DIRECTIVE_BASE}.c_str(), commandName.c_str());
            TDirective directive{};
            directive.MutableNavigateBrowserDirective()->SetName(TString{ANALYTICS_TYPE});
            directive.MutableNavigateBrowserDirective()->SetCommand(command);
            ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, TStringBuf(expected));
        }
    }

    Y_UNIT_TEST(TestDeserializeRadioPlayDirective) {
        TDirective directive{};
        auto& radioPlay = *directive.MutableRadioPlayDirective();
        radioPlay.SetName(TString{ANALYTICS_TYPE});
        radioPlay.SetIsActive(true);
        radioPlay.SetIsAvailable(true);
        radioPlay.SetColor("#0071BB");
        radioPlay.SetFrequency("102.9");
        radioPlay.SetImageUrl("avatars.mds.yandex.net/get-music-misc/28592/komsomolskaya_pravda-225/%%");
        radioPlay.SetOfficialSiteUrl("http://mariafm.ru");
        radioPlay.SetRadioId("nashe");
        radioPlay.SetScore(0.4435845617);
        radioPlay.SetShowRecognition(true);
        radioPlay.SetStreamUrl("https://strm.yandex.ru/fm/fm_dacha_main/fm_dacha_main0.m3u8");
        radioPlay.SetTitle("Радио Комсомольская правда");
        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, RADIO_PLAY_DIRECTIVE);

        radioPlay.SetAlarmId("deadface-4487-421e-866d-a3f087990a34");
        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, RADIO_PLAY_DIRECTIVE_WITH_ALARM_ID);
    }

    Y_UNIT_TEST(TestDeserializeGoDownDirective) {
        TDirective directive{};
        directive.MutableGoDownDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, GO_DOWN_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeGoTopDirective) {
        TDirective directive{};
        directive.MutableGoTopDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, GO_TOP_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeGoUpDirective) {
        TDirective directive{};
        directive.MutableGoUpDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, GO_UP_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializePlayerPauseDirective) {
        TDirective directive{};
        auto& playerPause = *directive.MutablePlayerPauseDirective();
        playerPause.SetName(TString{ANALYTICS_TYPE});
        playerPause.SetSmooth(true);
        playerPause.SetMultiroomSessionId("12345");
        playerPause.SetRoomId("kitchen");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PLAYER_PAUSE_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeScreenOnDirective) {
        TDirective directive{};
        auto& screenOn = *directive.MutableScreenOnDirective();
        screenOn.SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SCREEN_ON_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeScreenOffDirective) {
        TDirective directive{};
        auto& screenOff = *directive.MutableScreenOffDirective();
        screenOff.SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SCREEN_OFF_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeShowTvGallery) {
        TDirective directive{};
        auto& tvShowGalleryDirective = *directive.MutableShowTvGalleryDirective();
        tvShowGalleryDirective.SetName(TString{ANALYTICS_TYPE});
        auto& item = *tvShowGalleryDirective.MutableItems()->Add();
        item.SetChannelType("personal");
        item.SetDescription("Эфир этого канала формируется автоматически на основании "
                            "ваших предпочтений — того, что вы любите смотреть.");
        item.SetName("Мой Эфир");
        item.SetProviderItemId("4461546c4debdcffbab506fd75246e19");
        item.SetProviderName("strm");
        item.SetRelevance(100500);
        item.SetThumbnailUrl16x9("https://avatars.mds.yandex.net/"
                                 "get-vh/1583218/2a0000016ad564e3344385fcfa3a92eec7f4/640x360");
        item.SetThumbnailUrl16x9Small("https://avatars.mds.yandex.net/"
                                      "get-vh/1583218/2a0000016ad564e3344385fcfa3a92eec7f4/640x360");
        item.SetTvEpisodeName("Пол это Лава в Роблокс! "
                              "Котёнок Лайк против Котика Игромана / Roblox The Floor is Lava");
        auto& tvStreamInfo = *item.MutableTvStreamInfo();
        tvStreamInfo.SetChannelType("personal");
        tvStreamInfo.SetIsPersonal(1);
        tvStreamInfo.SetTvEpisodeId("45f85199853131d2b50b3c78410b5c59");
        tvStreamInfo.SetTvEpisodeName("Пол это Лава в Роблокс! "
                                      "Котёнок Лайк против Котика Игромана / Roblox The Floor is Lava");
        item.SetType("tv_stream");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SHOW_TV_GALLERY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeAlarmStop) {
        TDirective directive{};
        directive.MutableAlarmStopDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, ALARM_STOP);
    }

    Y_UNIT_TEST(TestDeserializeAlarmSetMaxLevel) {
        TDirective directive{};
        auto& alarmStop = *directive.MutableAlarmSetMaxLevelDirective();
        alarmStop.SetName(TString{ANALYTICS_TYPE});
        alarmStop.SetNewLevel(5);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, ALARM_SET_MAX_LEVEL);
    }

    Y_UNIT_TEST(TestDeserializeMusicRecognition) {
        TDirective directive{};
        auto& musicRecognition = *directive.MutableMusicRecognitionDirective();
        musicRecognition.SetName(TString{ANALYTICS_TYPE});

        auto& album = *musicRecognition.MutableAlbum();
        album.SetGenre("soundtrack");
        album.SetId("59592");
        album.SetTitle("The Matrix Reloaded: The Album");

        auto& artist = *musicRecognition.AddArtists();
        artist.SetComposer(true);
        artist.SetIsVarious(true);
        artist.SetId("1151");
        artist.SetName("Justin Timberlake");

        musicRecognition.SetCoverUri("https://avatars.yandex.net/get-music-content/28589/a86f9db5.a.59592-1/200x200");
        musicRecognition.SetId("555822");
        musicRecognition.SetSubtype("music");
        musicRecognition.SetTitle("When The World Ends");
        musicRecognition.SetType("track");
        musicRecognition.SetUri("https://music.yandex.ru/album/59592/track/555822/?from=alice&mob=0&play=1");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MUSIC_RECOGNITION);
    }

    Y_UNIT_TEST(TestDeserializeCallToRecipient) {
        TDirective directive{};

        NAlice::NScenarios::TMessengerCallDirective::TRecipient recipient;
        recipient.SetName("name");
        recipient.SetGuid("guid");

        NAlice::NScenarios::TMessengerCallDirective::TCallToRecipient action;
        *action.MutableRecipient() = recipient;
        *directive.MutableMessengerCallDirective()->MutableCallToRecipient() = action;

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MESSENGER_CALL_TO_RECIPIENT);
    }

    Y_UNIT_TEST(TestDeserializeAcceptCall) {
        TDirective directive{};

        NAlice::NScenarios::TMessengerCallDirective::TAcceptCall action;
        action.SetCallGuid("callguid");
        *directive.MutableMessengerCallDirective()->MutableAcceptCall() = action;

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MESSENGER_ACCEPT_CALL);
    }

    Y_UNIT_TEST(TestDeserializeDeclineCurrentCall) {
        TDirective directive{};

        NAlice::NScenarios::TMessengerCallDirective::TDeclineCurrentCall action;
        action.SetCallGuid("callguid");
        *directive.MutableMessengerCallDirective()->MutableDeclineCurrentCall() = action;

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MESSENGER_DECLINE_CURRENT_CALL);
    }

    Y_UNIT_TEST(TestDeserializeDeclineIncomingCall) {
        TDirective directive{};

        NAlice::NScenarios::TMessengerCallDirective::TDeclineIncomingCall action;
        action.SetCallGuid("callguid");
        *directive.MutableMessengerCallDirective()->MutableDeclineIncomingCall() = action;

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MESSENGER_DECLINE_INCOMING_CALL);
    }

    Y_UNIT_TEST(TestDeserializePlayerContinue) {
        TDirective directive{};
        directive.MutablePlayerContinueDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutablePlayerContinueDirective()->SetPlayer("music");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PLAYER_CONTINUE_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializePlayerDislike) {
        TDirective directive{};
        directive.MutablePlayerDislikeDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutablePlayerDislikeDirective()->SetUid(TString{UID});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PLAYER_DISLIKE_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializePlayerLike) {
        TDirective directive{};
        directive.MutablePlayerLikeDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutablePlayerLikeDirective()->SetUid(TString{UID});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PLAYER_LIKE_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializePlayerNextTrack) {
        TDirective directive{};
        directive.MutablePlayerNextTrackDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutablePlayerNextTrackDirective()->SetUid(TString{UID});
        directive.MutablePlayerNextTrackDirective()->SetPlayer("music");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PLAYER_NEXT_TRACK_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializePlayerPreviousTrack) {
        TDirective directive{};
        directive.MutablePlayerPreviousTrackDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutablePlayerPreviousTrackDirective()->SetPlayer("music");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PLAYER_PREVIOUS_TRACK_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializePlayerReplay) {
        TDirective directive{};
        directive.MutablePlayerReplayDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PLAYER_REPLAY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializePlayerShuffle) {
        TDirective directive{};
        directive.MutablePlayerShuffleDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PLAYER_SHUFFLE_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeAlarmSetSoundDirective) {
        TDirective directive{};
        directive.MutableAlarmSetSoundDirective()->SetName(TString{ANALYTICS_TYPE});
        auto& callback = *directive.MutableAlarmSetSoundDirective()->MutableCallback();
        callback.SetName(TString{INNER_ANALYTICS_TYPE});
        *callback.MutablePayload() = TProtoStructBuilder{}.Set(TString{KEY}, TString{VALUE}).Build();
        callback.SetIgnoreAnswer(true);
        callback.SetIsLedSilent(true);
        directive.MutableAlarmSetSoundDirective()->MutableSettings()->CopyFrom(
            GetAlarmSetSoundDirectiveModel().GetSettings());
        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, ALARM_SET_SOUND_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeDrawLedScreen) {
        TDirective directive{};
        directive.MutableDrawLedScreenDirective()->SetName(TString{ANALYTICS_TYPE});
        auto* item1 = directive.MutableDrawLedScreenDirective()->AddDrawItem();
        item1->SetFrontalLedImage("https://quasar.s3.yandex.net/led_screen/cloud-3.gif");
        auto* item2 = directive.MutableDrawLedScreenDirective()->AddDrawItem();
        item2->SetFrontalLedImage("https://quasar.s3.yandex.net/led_screen/cloud-4.gif");
        item2->SetEndless(true);
        directive.MutableDrawLedScreenDirective()->SetTillEndOfSpeech(true);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, DRAW_LED_SCREEN_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeSaveVoiceprint) {
        TDirective directive{};
        directive.MutableSaveVoiceprintDirective()->SetUserId("687820164");
        directive.MutableSaveVoiceprintDirective()->AddRequestIds("bf0d764f-064d-411f-916a-98f4d4fc8b60");
        directive.MutableSaveVoiceprintDirective()->AddRequestIds("f3ec7027-60f7-4972-93ef-d6a6ce8a984b");
        directive.MutableSaveVoiceprintDirective()->AddRequestIds("d3421dfa-84eb-4d20-8b40-b559b9c21da0");
        directive.MutableSaveVoiceprintDirective()->AddRequestIds("9db13883-fd5d-4ea3-abe6-7f6fa2bb7838");
        directive.MutableSaveVoiceprintDirective()->AddRequestIds("e57a7c3e-a248-4737-a602-8e6352b0f1d9");
        directive.MutableSaveVoiceprintDirective()->SetUserType(NScenarios::TSaveVoiceprintDirective_EUserType_Guest);
        directive.MutableSaveVoiceprintDirective()->SetPersId("PersId-4e978544-ef24409e-5880270f-46853b36");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SAVE_VOICEPRINT);
    }

    Y_UNIT_TEST(TestDeserializeRemoveVoiceprint) {
        TDirective directive{};
        directive.MutableRemoveVoiceprintDirective()->SetUserId("851083725");
        directive.MutableRemoveVoiceprintDirective()->SetPersId("PersId-4e978544-ef24409e-5880270f-46853b36");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, REMOVE_VOICEPRINT);
    }

    Y_UNIT_TEST(TestDeserializeMultiaccountRemoveAccount) {
        TDirective directive{};
        directive.MutableMultiaccountRemoveAccountDirective()->SetPuid(851083725);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MULTIACCOUNT_REMOVE_ACCOUNT);
    }

    Y_UNIT_TEST(TestDeserializeNotificationSubscribe) {
        TDirective directive{};
        directive.MutableUpdateNotificationSubscriptionDirective()->SetUnsubscribe(true);
        directive.MutableUpdateNotificationSubscriptionDirective()->SetSubscriptionId(123);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, UPDATE_NOTIFICATION_SUBSCRIPTION);

        directive.MutableUpdateNotificationSubscriptionDirective()->SetUnsubscribe(false);
        directive.MutableUpdateNotificationSubscriptionDirective()->SetSubscriptionId(0);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, UPDATE_NOTIFICATION_SUBSCRIPTION_DEFAULTS);
    }

    Y_UNIT_TEST(TestDeserializeMarkNotificationAsRead) {
        TDirective directive{};
        directive.MutableMarkNotificationAsReadDirective()->SetNotificationId("123");
        directive.MutableMarkNotificationAsReadDirective()->AddNotificationIds("1");
        directive.MutableMarkNotificationAsReadDirective()->AddNotificationIds("2");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MARK_NOTIFICATION_AS_READ);
    }

    Y_UNIT_TEST(TestDeserializeSearchButton) {
        TLayout_TSuggest suggest{};
        auto& searchBtn = *suggest.MutableSearchButton();
        searchBtn.SetTitle("My button title");
        searchBtn.SetQuery("Search text");

        ASSERT_DESERIALIZE_PROTO_MODEL(TSpeechKitResponseProto_TResponse_TButton, suggest,
                                       SEARCH_ACTION_BUTTON_WITH_ON_SUGGEST);
    }

    Y_UNIT_TEST(TestDeserializeSimpleSuggestButton) {
        TLayout_TSuggest suggest{};

        auto& button = *suggest.MutableActionButton();
        button.SetTitle(TString{TITLE});
        button.SetActionId(TString{ACTION_ID});

        THashMap<TString, TFrameAction> actions{};
        TFrameAction action;
        action.MutableDirectives()->AddList()->MutableStartImageRecognizerDirective()->SetName(
            TString{INNER_ANALYTICS_TYPE});
        actions.insert({TString{ACTION_ID}, action});

        TScenarioProtoDeserializer deserializer(GetSerializerMeta(), TRTLogger::StderrLogger(), std::move(actions));

        TSpeechKitResponseProto_TResponse_TButton expectedProto;
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = false;
        UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(TString{ACTION_BUTTON_WITH_ON_SUGGEST}, &expectedProto,
                                                                options)
                        .ok());
        const auto& actualProto = GetSpeechKitProtoModelSerializer().Serialize(*deserializer.Deserialize(suggest));
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);
    }

    Y_UNIT_TEST(TestDeserializeTThemedSuggestButtonWithDirectives) {
        TLayout_TSuggest suggest{};
        auto& button = *suggest.MutableActionButton();
        button.SetTitle(TString{TITLE});
        button.SetActionId(TString{ACTION_ID});
        button.MutableTheme()->SetImageUrl(TString{IMAGE_URL});

        THashMap<TString, TFrameAction> actions{};
        TFrameAction action;
        action.MutableDirectives()->AddList()->MutableStartImageRecognizerDirective()->SetName(
            TString{INNER_ANALYTICS_TYPE});
        actions.insert({TString{ACTION_ID}, action});

        TScenarioProtoDeserializer deserializer(GetSerializerMeta(), TRTLogger::StderrLogger(), std::move(actions));
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = false;

        TSpeechKitResponseProto_TResponse_TButton expectedProto;
        UNIT_ASSERT(google::protobuf::util::JsonStringToMessage(TString{THEMED_ACTION_BUTTON_WITH_ON_SUGGEST},
                                                                &expectedProto, options)
                        .ok());
        const auto& actualProto = GetSpeechKitProtoModelSerializer().Serialize(*deserializer.Deserialize(suggest));
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);
    }

    Y_UNIT_TEST(TestDeserializeTChangeAudioDirectiveModel) {
        TDirective directive;
        directive.MutableChangeAudioDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableChangeAudioDirective()->SetLanguage("eng");
        directive.MutableChangeAudioDirective()->SetTitle("Английская");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, CHANGE_AUDIO_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTChangeSubtitlesDirectiveModelOff) {
        TDirective directive;
        directive.MutableChangeSubtitlesDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableChangeSubtitlesDirective()->SetEnable(false);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, CHANGE_SUBTITLES_DIRECTIVE_OFF);
    }

    Y_UNIT_TEST(TestDeserializeTChangeSubtitlesDirectiveModelOn) {
        TDirective directive;
        directive.MutableChangeSubtitlesDirective()->SetName(TString{ANALYTICS_TYPE});
        google::protobuf::StringValue language;
        language.set_value("eng");
        google::protobuf::StringValue title;
        title.set_value("Английские");
        *directive.MutableChangeSubtitlesDirective()->MutableLanguage() = language;
        *directive.MutableChangeSubtitlesDirective()->MutableTitle() = title;
        directive.MutableChangeSubtitlesDirective()->SetEnable(true);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, CHANGE_SUBTITLES_DIRECTIVE_ON);
    }

    Y_UNIT_TEST(TestDeserializeTShowVideoSettings) {
        TDirective directive;
        directive.MutableShowVideoSettingsDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SHOW_VIDEO_SETTINGS_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTSendBugReportDirective) {
        TDirective directive;
        directive.MutableSendBugReportDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableSendBugReportDirective()->SetRequestId("abacabadabacaba");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SEND_BUG_REPORT_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTOpenDiskDirective) {
        TDirective directive;
        directive.MutableOpenDiskDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableOpenDiskDirective()->SetDisk("f");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, OPEN_DISK);
    }

    Y_UNIT_TEST(TestDeserializeDelicateNotifyDirective) {
        TDirective directive;
        directive.MutableNotifyDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableNotifyDirective()->SetVersionId("123123123123");
        directive.MutableNotifyDirective()->SetRing(TNotifyDirective::Delicate);

        auto notification = directive.MutableNotifyDirective()->AddNotifications();
        notification->SetText("Новости дня: Орел убил кукушку и узнал страшное...Читайте дальше на нашем сайте");
        notification->SetId("bf0d159a-064d-411f-916a-98f4d4fc8b60");
        notification->SetSubscriptionId("42");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, NOTIFY_DIRECTIVE_DELICATE);
    }

    Y_UNIT_TEST(TestDeserializeProactiveNotifyDirective) {
        TDirective directive;
        directive.MutableNotifyDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableNotifyDirective()->SetVersionId("123123123123");
        directive.MutableNotifyDirective()->SetRing(TNotifyDirective::Proactive);

        auto notification = directive.MutableNotifyDirective()->AddNotifications();
        notification->SetText("Новости дня: Орел убил кукушку и узнал страшное...Читайте дальше на нашем сайте");
        notification->SetId("bf0d159a-064d-411f-916a-98f4d4fc8b60");
        notification->SetSubscriptionId("42");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, NOTIFY_DIRECTIVE_PROACTIVE);
    }

    Y_UNIT_TEST(TestDeserializeNoSoundNotifyDirective) {
        TDirective directive;
        directive.MutableNotifyDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableNotifyDirective()->SetVersionId("123123123123");
        directive.MutableNotifyDirective()->SetRing(TNotifyDirective::NoSound);

        auto notification = directive.MutableNotifyDirective()->AddNotifications();
        notification->SetText("Новости дня: Орел убил кукушку и узнал страшное...Читайте дальше на нашем сайте");
        notification->SetId("bf0d159a-064d-411f-916a-98f4d4fc8b60");
        notification->SetSubscriptionId("42");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, NOTIFY_DIRECTIVE_NOSOUND);
    }

    Y_UNIT_TEST(TestDeserializeTAudioRewindDirective) {
        TDirective directive;
        directive.MutableAudioRewindDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableAudioRewindDirective()->SetType(TAudioRewindDirective::Absolute);
        directive.MutableAudioRewindDirective()->SetAmountMs(10);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, AUDIO_REWIND_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTAudioPlayDirectiveModelWithoutGlagolMetadata) {
        TDirective directive = GetAudioPlayDirective();

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, AUDIO_PLAY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTAudioPlayDirectiveModelNoCallbacks) {
        TDirective directive = GetAudioPlayDirective(/* noCallacks= */ true);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, AUDIO_PLAY_DIRECTIVE_NO_CALLBACKS);
    }

    Y_UNIT_TEST(TestDeserializeTAudioPlayDirectiveModelSomeCallbacks) {
        TDirective directive = GetAudioPlayDirective(/* noCallacks= */ false, /* someCallbacks= */ true);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, AUDIO_PLAY_DIRECTIVE_SOME_CALLBACKS);
    }

    Y_UNIT_TEST(TestDeserializeTAudioPlayDirectiveModel) {
        TDirective directive = GetAudioPlayDirective();

        auto& metadata = *directive.MutableAudioPlayDirective()->MutableAudioPlayMetadata();
        auto& musicMetadata = *metadata.MutableGlagolMetadata()->MutableMusicMetadata();
        musicMetadata.SetId("12345");
        musicMetadata.SetType(NScenarios::TAudioPlayDirective_TAudioPlayMetadata_EContentType_Track);
        musicMetadata.SetDescription("Описание");
        auto& prevTrackInfo = *musicMetadata.MutablePrevTrackInfo();
        prevTrackInfo.SetId("12344");
        prevTrackInfo.SetStreamType(NScenarios::TAudioPlayDirective_TStream_TStreamType_Track);
        auto& nextTrackInfo = *musicMetadata.MutableNextTrackInfo();
        nextTrackInfo.SetId("12346");
        nextTrackInfo.SetStreamType(NScenarios::TAudioPlayDirective_TStream_TStreamType_Track);
        musicMetadata.SetShuffled(true);
        musicMetadata.SetRepeatMode(NScenarios::TAudioPlayDirective_TAudioPlayMetadata_ERepeatMode_All);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, AUDIO_PLAY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeStartBroadcast) {
        TDirective directive{};
        directive.MutableStartBroadcastDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableStartBroadcastDirective()->SetTimeoutMs(30000);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, START_BROADCAST_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeStopBroadcast) {
        TDirective directive{};
        directive.MutableStopBroadcastDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, STOP_BROADCAST_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeIoTDiscoveryStart) {
        TDirective directive{};
        directive.MutableIoTDiscoveryStartDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableIoTDiscoveryStartDirective()->SetTimeoutMs(30000);
        directive.MutableIoTDiscoveryStartDirective()->SetDeviceType("devices.types.light");
        directive.MutableIoTDiscoveryStartDirective()->SetSSID("my_ssid");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, IOT_DISCOVERY_START_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeIoTDiscoveryCredentials) {
        TDirective directive{};
        directive.MutableIoTDiscoveryCredentialsDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableIoTDiscoveryCredentialsDirective()->SetSSID("my_ssid");
        directive.MutableIoTDiscoveryCredentialsDirective()->SetPassword("my_password");
        directive.MutableIoTDiscoveryCredentialsDirective()->SetToken("token");
        directive.MutableIoTDiscoveryCredentialsDirective()->SetCipher("cipher");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, IOT_DISCOVERY_CREDENTIALS_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeIoTDiscoveryStop) {
        TDirective directive{};
        directive.MutableIoTDiscoveryStopDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, IOT_DISCOVERY_STOP_DIRECTIVE);
    }

    Y_UNIT_TEST(TestTtsPlayPlaceholderDirective) {
        TDirective directive{};
        directive.MutableTtsPlayPlaceholderDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, TTS_PLAY_PLACEHOLDER);
    }

    Y_UNIT_TEST(TestDeserializeTSetupRcuDirective) {
        TDirective directive;
        directive.MutableSetupRcuDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SETUP_RCU_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTSetupRcuAutoDirective) {
        TDirective directive;
        directive.MutableSetupRcuAutoDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableSetupRcuAutoDirective()->SetTvModel(TString{VALUE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SETUP_RCU_AUTO_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTSetupRcuCheckDirective) {
        TDirective directive;
        directive.MutableSetupRcuCheckDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SETUP_RCU_CHECK_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTSetupRcuManualDirective) {
        TDirective directive;
        directive.MutableSetupRcuManualDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SETUP_RCU_MANUAL_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTSetupRcuAdvancedDirective) {
        TDirective directive;
        directive.MutableSetupRcuAdvancedDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SETUP_RCU_ADVANCED_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTFindRcuDirective) {
        TDirective directive;
        directive.MutableFindRcuDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, FIND_RCU_DIRECTIVE);
    }

    Y_UNIT_TEST(TForceDisplayCardsDirective) {
        TDirective directive;
        directive.MutableForceDisplayCardsDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, FORCE_DISPLAY_CARDS_DIRECTIVE);
    }

    Y_UNIT_TEST(TListenDirectiveDefault) {
        TDirective directive;
        directive.MutableListenDirective()->SetName(TString{ANALYTICS_TYPE});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, LISTEN_DIRECTIVE_DEFAULT);
    }

    Y_UNIT_TEST(TListenDirectiveWithTimeout) {
        TDirective directive;
        directive.MutableListenDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableListenDirective()->SetStartingSilenceTimeoutMs(10000);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, LISTEN_DIRECTIVE_TIMEOUT);
    }

    Y_UNIT_TEST(TestDeserializeTUpdateDatasyncDirective) {
        TServerDirective directive;
        auto& datasyncDirective = *directive.MutableUpdateDatasyncDirective();
        datasyncDirective.SetKey(TString{"Test/Key"});
        datasyncDirective.SetMethod(
            TUpdateDatasyncDirective_EDataSyncMethod::TUpdateDatasyncDirective_EDataSyncMethod_Put);
        datasyncDirective.SetStringValue(TString{"TestValue"});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, UPDATE_DATASYNC_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTUpdateDatasyncDirectiveWithApplyFor) {
        static const TVector<std::pair<TServerDirective_TMeta_EApplyFor, TStringBuf>> TEST_CASES = {
            {TServerDirective::TMeta::EApplyFor::TServerDirective_TMeta_EApplyFor_DeviceOwner, UPDATE_DATASYNC_DIRECTIVE_APPLY_FOR_OWNER},
            {TServerDirective::TMeta::EApplyFor::TServerDirective_TMeta_EApplyFor_CurrentUser, UPDATE_DATASYNC_DIRECTIVE_APPLY_FOR_CURRENT_USER},
        };

        for (const auto& [applyFor, json] : TEST_CASES) {
            TServerDirective directive;
            directive.MutableMeta()->SetApplyFor(applyFor);

            auto& datasyncDirective = *directive.MutableUpdateDatasyncDirective();
            datasyncDirective.SetKey(TString{"Test/Key"});
            datasyncDirective.SetMethod(
                TUpdateDatasyncDirective_EDataSyncMethod::TUpdateDatasyncDirective_EDataSyncMethod_Put);
            datasyncDirective.SetStringValue(TString{"TestValue"});

            ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, json);
        }
    }

    Y_UNIT_TEST(TestDeserializeTUpdateDatasyncDirectiveWithStruct) {
        TServerDirective directive;
        auto& datasyncDirective = *directive.MutableUpdateDatasyncDirective();
        datasyncDirective.SetKey(TString{"Test/Key"});
        datasyncDirective.SetMethod(
            TUpdateDatasyncDirective_EDataSyncMethod::TUpdateDatasyncDirective_EDataSyncMethod_Put);
        auto* valuePtr = datasyncDirective.MutableStructValue();
        google::protobuf::util::JsonStringToMessage(
            R"({
                "subkey": "value",
                "subkey2": 123
            })",
            valuePtr
        );

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, UPDATE_DATASYNC_DIRECTIVE_STRUCT);
    }

    Y_UNIT_TEST(TestDeserializePushMessage) {
        TServerDirective directive{};
        directive.MutablePushMessageDirective()->SetTitle("dummy title");
        directive.MutablePushMessageDirective()->SetBody("open me");
        directive.MutablePushMessageDirective()->SetLink("yandex.ru");
        directive.MutablePushMessageDirective()->SetPushId("test_id");
        directive.MutablePushMessageDirective()->SetPushTag("test_id");
        directive.MutablePushMessageDirective()->SetThrottlePolicy("alice_push_policy");
        directive.MutablePushMessageDirective()->AddAppTypes(NAlice::EAppType::AT_SEARCH_APP);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PUSH_MESSAGE_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializePushMessageWithLinkPlaceholder) {
        TServerDirective directive{};
        directive.MutablePushMessageDirective()->SetTitle("dummy title");
        directive.MutablePushMessageDirective()->SetBody("open me");
        directive.MutablePushMessageDirective()->SetLink(TString{MM_DEEPLINK_PLACEHOLDER} + TString{ACTION_ID});
        directive.MutablePushMessageDirective()->SetPushId("test_id");
        directive.MutablePushMessageDirective()->SetPushTag("test_id");
        directive.MutablePushMessageDirective()->SetThrottlePolicy("alice_push_policy");
        directive.MutablePushMessageDirective()->AddAppTypes(NAlice::EAppType::AT_SEARCH_APP);

        THashMap<TString, TFrameAction> actions{};
        TFrameAction action;
        action.MutableDirectives()->AddList()->MutableStartImageRecognizerDirective()->SetName(
            TString{INNER_ANALYTICS_TYPE});
        actions.insert({TString{ACTION_ID}, action});

        TScenarioProtoDeserializer deserializer(GetSerializerMeta(), TRTLogger::StderrLogger(), std::move(actions));
        NSpeechKit::TDirective expectedProto;
            google::protobuf::util::JsonParseOptions options;
            options.ignore_unknown_fields = false;
        UNIT_ASSERT(
            google::protobuf::util::JsonStringToMessage(TString{PUSH_MESSAGE_DIRECTIVE_WITH_PLACEHOLDER},
                                                        &expectedProto, options).ok());
        const auto& actualProto =
                GetSpeechKitProtoModelSerializer().Serialize(*deserializer.Deserialize(directive));
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);
    }

    Y_UNIT_TEST(TestDeserializeTPersonalCardsDirective) {
        TServerDirective directive;
        auto& personalCardsDirective = *directive.MutablePersonalCardsDirective();

        auto& card = *personalCardsDirective.MutableCard();
        card.SetCardId("station_billing_12345");
        card.SetButtonUrl("https://yandex.ru/quasar/id/kinopoisk/promoperiod");
        card.SetText("Активировать Яндекс.Плюс");
        card.SetDateFrom(1596398659);
        card.SetDateTo(1596405859);

        auto& cardData = *card.MutableYandexStationFilmData();
        cardData.SetMinPrice(0);

        personalCardsDirective.SetRemoveExistingCards(true);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PERSONAL_CARDS_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTMementoChangeUserObjectsDirective) {
        TServerDirective directive;
        auto& mementoChangeUserObjectsDirective = *directive.MutableMementoChangeUserObjectsDirective();

        auto* userObjects = mementoChangeUserObjectsDirective.MutableUserObjects();

        auto* userConfigs = userObjects->AddUserConfigs();
        userConfigs->SetKey(ru::yandex::alice::memento::proto::EConfigKey::CK_MORNING_SHOW);
        google::protobuf::Value value;
        value.set_string_value("value");
        userConfigs->MutableValue()->PackFrom(value);

        const auto data = TProtoStructBuilder{}.Set("key", "value").Build();
        userObjects->MutableScenarioData()->PackFrom(data);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MEMENTO_CHANGE_USER_OBJECTS_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTPushTypedSemanticFrameDirective) {
        TServerDirective directive{};

        auto& dir = *directive.MutablePushTypedSemanticFrameDirective();
        dir.SetPuid("13071999");
        dir.SetDeviceId("MEGADEVICE_GOBLIN_3000");
        dir.SetTtl(228);

        auto& data = *dir.MutableSemanticFrameRequestData();
        data.MutableTypedSemanticFrame()->MutableWeatherSemanticFrame()->MutableWhen()->SetDateTimeValue("13:07:1999");

        auto& analytics = *data.MutableAnalytics();
        analytics.SetOrigin(TAnalyticsTrackingModule::EOrigin::TAnalyticsTrackingModule_EOrigin_Scenario);
        analytics.SetProductScenario("Weather");

        // scenario-created TOrigin will be ignored
        auto& origin = *data.MutableOrigin();
        origin.SetDeviceId("will_be_ignored_device_id");
        origin.SetUuid("will_be_ignored_uuid");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PUSH_TYPED_SEMANTIC_FRAME_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTMultiroomSemanticFrameDirective) {
        TDirective directive;

        auto& dir = *directive.MutableMultiroomSemanticFrameDirective();
        dir.SetDeviceId("device_id_2");

        auto& body = *dir.MutableBody();
        body.MutableTypedSemanticFrame()->MutableWeatherSemanticFrame()->MutableWhen()->SetDateTimeValue("13:07:1999");

        auto& analytics = *body.MutableAnalytics();
        analytics.SetOrigin(TAnalyticsTrackingModule::EOrigin::TAnalyticsTrackingModule_EOrigin_Scenario);
        analytics.SetProductScenario("Weather");

        // scenario-created TOrigin will be ignored
        auto& origin = *body.MutableOrigin();
        origin.SetDeviceId("will_be_ignored_device_id");
        origin.SetUuid("will_be_ignored_uuid");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, MULTIROOM_SEMANTIC_FRAME_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTStartMultiroomDirectiveWithRoomId) {
        TDirective directive;
        directive.MutableStartMultiroomDirective()->SetRoomId("room_1");
        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, START_MULTIROOM_DIRECTIVE_WITH_ROOM_ID);
    }

    Y_UNIT_TEST(TestDeserializeTStartMultiroomDirectiveWithLocationInfo) {
        TDirective directive;
        auto& locationInfo = *directive.MutableStartMultiroomDirective()->MutableLocationInfo();
        locationInfo.AddGroupsIds("group_2");
        locationInfo.AddGroupsIds("group_3");
        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, START_MULTIROOM_DIRECTIVE_WITH_LOCATION_INFO);
    }

    Y_UNIT_TEST(TestDeserializeTStartMultiroomDirectiveWithLocationInfoAndIncludeCurrentDeviceSetTrue) {
        TDirective directive;
        directive.MutableStartMultiroomDirective()->SetName(TString{ANALYTICS_TYPE});
        auto& locationInfo = *directive.MutableStartMultiroomDirective()->MutableLocationInfo();
        locationInfo.AddRoomsIds("room_2");
        locationInfo.SetIncludeCurrentDeviceId(true);
        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, START_MULTIROOM_DIRECTIVE_WITH_LOCATION_INFO_2);
    }

    Y_UNIT_TEST(TestDeserializeTStartMultiroomDirectiveWithToken) {
        TDirective directive;
        auto& d = *directive.MutableStartMultiroomDirective();
        d.MutableLocationInfo()->SetEverywhere(true);
        d.SetMultiroomToken(TEST_MULTIROOM_TOKEN.data(), TEST_MULTIROOM_TOKEN.size());
        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, START_MULTIROOM_DIRECTIVE_WITH_LOCATION_INFO_3);
    }

    Y_UNIT_TEST(TestDeserializeTAddScheduleActionDirective) {
        TServerDirective directive;

        auto& scheduleAction = *directive.MutableAddScheduleActionDirective()->MutableScheduleAction();
        scheduleAction.SetId("delivery_action");
        scheduleAction.SetPuid("339124070");
        scheduleAction.SetDeviceId("MOCK_DEVICE_ID");
        scheduleAction.MutableStartPolicy()->SetStartAtTimestampMs(123);

        auto& retryPolicy = *scheduleAction.MutableSendPolicy()->MutableSendOncePolicy()->MutableRetryPolicy();
        retryPolicy.SetMaxRetries(1);
        retryPolicy.SetRestartPeriodScaleMs(200);
        retryPolicy.SetRestartPeriodBackOff(2);
        retryPolicy.SetMinRestartPeriodMs(10000);
        retryPolicy.SetMaxRestartPeriodMs(100000);

        auto& delivery = *scheduleAction.MutableAction()->MutableOldNotificatorRequest()->MutableDelivery();
        delivery.SetPuid("339124070");
        delivery.SetDeviceId("MOCK_DEVICE_ID");
        delivery.SetTtl(1);
        delivery.MutableSemanticFrameRequestData()->MutableTypedSemanticFrame()->MutableIoTBroadcastStartSemanticFrame()->MutablePairingToken()->SetStringValue("token");
        delivery.MutableSemanticFrameRequestData()->MutableAnalytics()->SetPurpose("video");
    }

    Y_UNIT_TEST(TestDeserializeTAddCardDirectiveWithoutActionSpace) {
        TDirective directive;

        auto* addCardDirective = directive.MutableAddCardDirective();
        addCardDirective->SetName(TString{ANALYTICS_TYPE});
        addCardDirective->SetCarouselId("id1");
        addCardDirective->SetCardId("id2");
        addCardDirective->SetCardShowTimeSec(10);
        addCardDirective->SetTitle("title");
        addCardDirective->SetImageUrl("image_url");
        addCardDirective->SetType("type");
        addCardDirective->SetActionSpaceId("1");

        auto* teaserConfig = addCardDirective->MutableTeaserConfig();
        teaserConfig->SetTeaserType("type");
        teaserConfig->SetTeaserId("id");

        auto* div2Card = addCardDirective->MutableDiv2Card();
        div2Card->SetHideBorders(true);
        div2Card->MutableBody();

        addCardDirective->MutableDiv2Templates();

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, ADD_CARD_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTShowViewDirective) {
        TDirective directive;

        auto* showViewDirective = directive.MutableShowViewDirective();
        showViewDirective->SetName(TString{ANALYTICS_TYPE});

        auto* div2Card = showViewDirective->MutableDiv2Card();
        div2Card->SetHideBorders(true);
        div2Card->MutableBody();

        showViewDirective->MutableLayer()->MutableContent();

        showViewDirective->SetInactivityTimeout(NScenarios::TShowViewDirective_EInactivityTimeout_Long);
        showViewDirective->SetActionSpaceId("1");
        showViewDirective->SetKeepStashedIfPossible(true);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SHOW_VIEW_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTPatchViewDirective) {
        TDirective directive;

        auto* patchViewDirective = directive.MutablePatchViewDirective();
        patchViewDirective->SetName(TString{ANALYTICS_TYPE});

        auto* patch = patchViewDirective->MutableDiv2Patch();
        patch->MutableBody();

        auto* applyTo = patchViewDirective->MutableApplyTo();
        applyTo->SetCardName("A");
        applyTo->SetCardId("1");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, PATCH_VIEW_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTStashViewDirective) {
        TDirective directive;

        auto* stashViewDirective = directive.MutableStashViewDirective();
        stashViewDirective->SetName(TString{ANALYTICS_TYPE});

        auto* criteria = stashViewDirective->MutableCardSearchCriteria();
        criteria->SetCardName("A");
        criteria->SetCardId("1");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, STASH_VIEW_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTUnstashViewDirective) {
        TDirective directive;

        auto* unstashViewDirective = directive.MutableUnstashViewDirective();
        unstashViewDirective->SetName(TString{ANALYTICS_TYPE});

        auto* criteria = unstashViewDirective->MutableCardSearchCriteria();
        criteria->SetCardName("A");
        criteria->SetCardId("1");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, UNSTASH_VIEW_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTDivUIShowViewDirective) {
        TDirective directive;

        auto* showViewDirective = directive.MutableDivUIShowViewDirective();
        showViewDirective->SetName(TString{ANALYTICS_TYPE});

        auto* div2Card = showViewDirective->MutableDiv2Card();
        div2Card->SetHideBorders(true);
        div2Card->MutableBody();

        showViewDirective->MutableLayer()->MutableContent();

        showViewDirective->SetInactivityTimeout(NAlice::TLayeredDivUICapability_TDivUIShowViewDirective_EInactivityTimeout_Long);
        showViewDirective->SetActionSpaceId("1");
        showViewDirective->SetStashInteraction(NAlice::TLayeredDivUICapability_TDivUIShowViewDirective_EStashInteraction_ShowUnstashed);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, DIVUI_SHOW_VIEW_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTDivUIPatchViewDirective) {
        TDirective directive;

        auto* patchViewDirective = directive.MutableDivUIPatchViewDirective();
        patchViewDirective->SetName(TString{ANALYTICS_TYPE});

        auto* patch = patchViewDirective->MutableDiv2Patch();
        patch->MutableBody();

        auto* applyTo = patchViewDirective->MutableApplyTo();
        applyTo->SetCardName("A");
        applyTo->SetCardId("1");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, DIVUI_PATCH_VIEW_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTDivUIStashViewDirective) {
        TDirective directive;

        auto* stashViewDirective = directive.MutableDivUIStashViewDirective();
        stashViewDirective->SetName(TString{ANALYTICS_TYPE});

        auto* criteria = stashViewDirective->MutableCardSearchCriteria();
        criteria->SetCardName("A");
        criteria->SetCardId("1");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, DIVUI_STASH_VIEW_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTDivUIUnstashViewDirective) {
        TDirective directive;

        auto* unstashViewDirective = directive.MutableDivUIUnstashViewDirective();
        unstashViewDirective->SetName(TString{ANALYTICS_TYPE});

        auto* criteria = unstashViewDirective->MutableCardSearchCriteria();
        criteria->SetCardName("A");
        criteria->SetCardId("1");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, DIVUI_UNSTASH_VIEW_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTUpperShutterDirective) {
        TDirective directive;

        auto* setUpperShutterDirective = directive.MutableSetUpperShutterDirective();
        setUpperShutterDirective->SetName(TString{ANALYTICS_TYPE});

        auto* div2Card = setUpperShutterDirective->MutableDiv2Card();
        div2Card->SetHideBorders(true);
        div2Card->MutableBody();

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SET_UPPER_SHUTTER_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTAddCardDirectiveWithDeepLinks) {
        TDirective directive;
        const TString actionSpaceId{"ACTION_SPACE_1"};
        const TString actionId{ACTION_ID};

        auto* addCardDirective = directive.MutableAddCardDirective();
        addCardDirective->SetName(TString{ANALYTICS_TYPE});
        addCardDirective->SetCarouselId("id1");
        addCardDirective->SetCardId("id2");
        addCardDirective->SetCardShowTimeSec(10);
        addCardDirective->SetTitle("title");
        addCardDirective->SetImageUrl("image_url");
        addCardDirective->SetType("type");
        addCardDirective->SetActionSpaceId(actionSpaceId);

        auto* teaserConfig = addCardDirective->MutableTeaserConfig();
        teaserConfig->SetTeaserType("type");
        teaserConfig->SetTeaserId("id");

        auto* div2Card = addCardDirective->MutableDiv2Card();
        div2Card->SetHideBorders(true);

        const auto data =
            std::move(TProtoStructBuilder{}.Set(TString{KEY}, TString{MM_DEEPLINK_PLACEHOLDER} + actionId))
                .Build();
        *div2Card->MutableBody() = data;
        *addCardDirective->MutableDiv2Templates() = data;

        THashMap<TString, NScenarios::TActionSpace> actionSpaces{};
        auto& space = actionSpaces[actionSpaceId];
        auto& action = (*space.MutableActions())[actionId];
        action.MutableSemanticFrame()
            ->MutableTypedSemanticFrame()
            ->MutableSearchSemanticFrame()
            ->MutableQuery()
            ->SetStringValue("query_text");

        TScenarioProtoDeserializer deserializer{GetSerializerMeta(), TRTLogger::StderrLogger(), /* actions= */ {},
                                                std::move(actionSpaces)};

        NSpeechKit::TDirective expectedProto;
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = false;
        UNIT_ASSERT(
            google::protobuf::util::JsonStringToMessage(TString{ADD_CARD_WITH_ACTION_SPACE}, &expectedProto, options)
                .ok());
        const auto& actualProto = GetSpeechKitProtoModelSerializer().Serialize(*deserializer.Deserialize(directive));
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);
    }

    Y_UNIT_TEST(TestDeserializeTRotateCardsDirective) {
        TDirective directive;

        auto* rotateCardsDirective = directive.MutableRotateCardsDirective();
        rotateCardsDirective->SetName(TString{ANALYTICS_TYPE});
        rotateCardsDirective->SetCarouselId(TString{UID});
        rotateCardsDirective->SetCarouselShowTimeSec(100);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, ROTATE_CARDS_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTSetMainScreenDirective) {
        TDirective directive;

        auto* setMainScreenDirective = directive.MutableSetMainScreenDirective();
        setMainScreenDirective->SetName(TString{ANALYTICS_TYPE});

        auto* tab = setMainScreenDirective->AddTabs();
        tab->SetId("tab-1");
        tab->SetTitle("Мой экран");

        auto* block = tab->AddBlocks();
        block->SetId("block-1");
        block->SetTitle("Послушать");
        auto* galleryBlock = block->MutableHorizontalMediaGalleryBlock();
        galleryBlock->SetHeight(244);
        {
            auto* card = galleryBlock->AddCards();
            card->SetId("card-1");
            card->SetWidth(244);
            auto& div2Card = *card->MutableCard();
            const auto data =
                std::move(TProtoStructBuilder{}.Set(TString{KEY}, TString{MM_DEEPLINK_PLACEHOLDER} + ACTION_ID))
                    .Build();
            *div2Card.MutableBody() = data;
        }
        {
            auto& div2Card = *galleryBlock->AddCards()->MutableCard();
            const auto data =
                std::move(TProtoStructBuilder{}.Set(TString{KEY}, TString{MM_DEEPLINK_PLACEHOLDER} + ANOTHER_ACTION_ID))
                    .Build();
            *div2Card.MutableBody() = data;
        }

        THashMap<TString, TFrameAction> actions{};
        TFrameAction action;
        action.MutableDirectives()->AddList()->MutableStartImageRecognizerDirective()->SetName(
            TString{INNER_ANALYTICS_TYPE});
        actions.insert({TString{ACTION_ID}, action});
        TFrameAction actionWithParams;
        actionWithParams.MutableParsedUtterance()->MutableParams()->SetDisableOutputSpeech(true);
        actionWithParams.MutableParsedUtterance()->MutableTypedSemanticFrame()->MutableOpenSmartDeviceExternalAppFrame();
        actions.insert({TString{ANOTHER_ACTION_ID}, actionWithParams});

        TScenarioProtoDeserializer deserializer(GetSerializerMeta(), TRTLogger::StderrLogger(), std::move(actions));

        NSpeechKit::TDirective expectedProto;
        google::protobuf::util::JsonParseOptions options;
        options.ignore_unknown_fields = false;
        UNIT_ASSERT(
            google::protobuf::util::JsonStringToMessage(TString{SET_MAIN_SCREEN_DIRECTIVE}, &expectedProto, options)
                .ok());
        const auto& actualProto = GetSpeechKitProtoModelSerializer().Serialize(*deserializer.Deserialize(directive));
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);
    }

    Y_UNIT_TEST(TestDeserializeTSetSmartTvCategoriesDirective) {
        TDirective directive;

        auto* setSmartTvCategoriesDirective = directive.MutableSetSmartTvCategoriesDirective();
        setSmartTvCategoriesDirective->SetName(TString{ANALYTICS_TYPE});

        auto* category = setSmartTvCategoriesDirective->AddCategories();
        category->SetCategoryId("id1");
        category->SetRank(1);
        category->SetIcon("http://mds/icon-1.png");
        category->SetTitle("Category title 1");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SET_SMART_TV_CATEGORIES);
    }

    Y_UNIT_TEST(TestDeserializeTTvOpenSearchScreenDirective) {
        TDirective directive;

        auto* openSearchScreenDirective = directive.MutableTvOpenSearchScreenDirective();
        openSearchScreenDirective->SetName(TString{ANALYTICS_TYPE});

        openSearchScreenDirective->SetSearchQuery("фильмы про спорт");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, TV_OPEN_SEARCH_SCREEN_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTTvOpenDetailsScreenDirective) {
        TDirective directive;
        auto* openDetailsScreenDirective = directive.MutableTvOpenDetailsScreenDirective();
        openDetailsScreenDirective->SetName(TString{ANALYTICS_TYPE});

        openDetailsScreenDirective->SetContentType("MOVIE");
        openDetailsScreenDirective->SetVhUuid("4a5bf03fd28452d1abe53f3801ff5e99");
        openDetailsScreenDirective->SetSearchQuery("экипаж фильм 2016");

        auto* detailsData = openDetailsScreenDirective->MutableData();
        detailsData->SetName("Экипаж");
        detailsData->SetDescription("Талантливый молодой лётчик Алексей Гущин не признаёт авторитетов, предпочитая поступать в соответствии с личным кодексом чести. За невыполнение абсурдного приказа его выгоняют из военной авиации, и только чудом он получает шанс летать на гражданских самолётах. Гущин начинает свою лётную жизнь сначала. Его наставник — командир воздушного судна — суровый и принципиальный Леонид Зинченко. Его коллега — второй пилот, неприступная красавица Александра. Отношения складываются непросто. Но на грани жизни и смерти, когда земля уходит из-под ног, вокруг — огонь и пепел, и только в небе есть спасение, Гущин показывает всё, на что он способен. Только вместе экипаж сможет совершить подвиг и спасти сотни жизней.");
        detailsData->SetHintDescription("2016, драма, триллер");

        auto* thumbnail = detailsData->MutableThumbnail();
        thumbnail->SetBaseUrl("https://avatars.mds.yandex.net/get-kinopoisk-image/1946459/bd25426a-073e-45e8-8c74-97d71c05ae64/");
        thumbnail->AddSizes("orig");

        auto* poster = detailsData->MutablePoster();
        poster->SetBaseUrl("http://avatars.mds.yandex.net/get-kino-vod-films-gallery/33804/2a000001528e2f061f0c6f28f5580ddb3ee4/");
        poster->AddSizes("360x540");
        poster->AddSizes("orig");

        detailsData->SetMinAge(6);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, TV_OPEN_DETAILS_SCREEN_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTTvOpenSeriesScreenDirective) {
        TDirective directive;
        auto* openSeriesScreenDirective = directive.MutableTvOpenSeriesScreenDirective();
        openSeriesScreenDirective->SetName(TString{ANALYTICS_TYPE});

        openSeriesScreenDirective->SetVhUuid("4faef5f0c695c590a0727142ee2f0a39");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, TV_OPEN_SERIES_SCREEN_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTTvOpenPersonScreenDirective) {
        TDirective directive;
        auto* openPersonScreenDirective = directive.MutableTvOpenPersonScreenDirective();
        openPersonScreenDirective->SetName(TString{ANALYTICS_TYPE});

        openPersonScreenDirective->SetKpId("1054956");

        auto* personData = openPersonScreenDirective->MutableData();
        personData->SetName("Данила Козловский");
        personData->SetSubtitle("Российский актёр");

        auto* image = personData->MutableImage();
        image->SetBaseUrl("http://avatars.mds.yandex.net/get-kino-vod-persons-gallery/33886/2a00000151ca1a244f419b4191c2663771fc/");
        image->AddSizes("orig");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, TV_OPEN_PERSON_SCREEN_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTTvOpenCollectionScreenDirective) {
        TDirective directive;
        auto* openCollectionScreenDirective = directive.MutableTvOpenCollectionScreenDirective();
        openCollectionScreenDirective->SetName(TString{ANALYTICS_TYPE});

        openCollectionScreenDirective->SetSearchQuery("Фильмы фестиваля Кинотавр");
        openCollectionScreenDirective->SetEntref("0oEg9sc3QtNGI5ODUzM2IuLjAYAgjH4DQ");

        auto* collectionData = openCollectionScreenDirective->MutableData();
        collectionData->SetTitle("Фильмы фестиваля Кинотавр");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, TV_OPEN_COLLECTION_SCREEN_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeTShowButtonsDirective) {
        // Build FrameActions
        THashMap<TString, TFrameAction> actions;
        for (int i = 0; i < 2; ++i) {
            TFrameAction action;
            auto& directives = *action.MutableDirectives()->MutableList();

            // add "type" directive
            auto& typeTextDirective = *directives.Add()->MutableTypeTextDirective();
            typeTextDirective.SetText(TYPE_TEXT.data());

            // add simple callback directive
            auto& callbackDirective = *directives.Add()->MutableCallbackDirective();
            callbackDirective.SetName("external_source_action");
            callbackDirective.SetIgnoreAnswer(true);
            *callbackDirective.MutablePayload() = TProtoStructBuilder()
                .Set("utm_source", "Yandex_Alisa")
                .Build();

            actions.insert({TString::Join(ACTION_ID, "_", ToString(i)), action});
        }

        TScenarioProtoDeserializer deserializer(GetSerializerMeta(), TRTLogger::StderrLogger(), std::move(actions));

        // Build directive
        TDirective directive;
        directive.MutableShowButtonsDirective()->SetName(TString{ANALYTICS_TYPE});
        directive.MutableShowButtonsDirective()->SetScreenId(TString{SCREEN_ID});

        for (int i = 0; i < 2; ++i) {
            const TString num = ToString(i);
            auto& button = *directive.MutableShowButtonsDirective()->MutableButtons()->Add();
            button.SetTitle(TString::Join(TITLE, "_", num));
            button.SetText(TString::Join(TEXT, "_", num));
            button.SetActionId(TString::Join(ACTION_ID, "_", num));
            button.MutableTheme()->SetImageUrl(TString::Join(IMAGE_URL, "_", num));
        }

        ASSERT_DESERIALIZE_PROTO_MODEL_CUSTOM_DESERIALIZER(deserializer, NSpeechKit::TDirective, directive, SHOW_BUTTONS_DIRECTIVE);
    }

    Y_UNIT_TEST(TestRequestPermissionsDirective) {
        TDirective directive;
        auto& requestPermissionsDirective = *directive.MutableRequestPermissionsDirective();
        requestPermissionsDirective.SetName(TString{ANALYTICS_TYPE});

        requestPermissionsDirective.AddPermissions(NAlice::TPermissions::Location);
        requestPermissionsDirective.AddPermissions(NAlice::TPermissions::ReadContacts);

        auto& onSuccess = *requestPermissionsDirective.MutableOnSuccess()->MutableTypeTextSilentDirective();
        onSuccess.SetName(TString{ANALYTICS_TYPE});
        onSuccess.SetText("Success!!!");

        auto& onFail = *requestPermissionsDirective.MutableOnFail()->MutableTypeTextSilentDirective();
        onFail.SetName(TString{ANALYTICS_TYPE});
        onFail.SetText("Fail!!!");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, REQUEST_PERMISSIONS_DIRECTIVE);
    }

    Y_UNIT_TEST(TestRequestPermissionsDirectiveWithoutSubdirectives) {
        TDirective directive;
        auto& requestPermissionsDirective = *directive.MutableRequestPermissionsDirective();
        requestPermissionsDirective.SetName(TString{ANALYTICS_TYPE});

        requestPermissionsDirective.AddPermissions(NAlice::TPermissions::CallPhone);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, REQUEST_PERMISSIONS_DIRECTIVE_WITHOUT_SUBDIRECTIVES);
    }

    Y_UNIT_TEST(TestSendAndroidAppIntentDirective) {
        TDirective directive;
        auto& sendAndroidAppIntentDirective = *directive.MutableSendAndroidAppIntentDirective();
        sendAndroidAppIntentDirective.SetName(TString{ANALYTICS_TYPE});

        auto& component = *sendAndroidAppIntentDirective.MutableComponent();
        component.SetPkg(TString{"com.yandex.test"});
        component.SetCls(TString{"com.yandex.test.Test"});

        auto& flags = *sendAndroidAppIntentDirective.MutableFlags();
        flags.SetFlagActivityNewTask(true);

        sendAndroidAppIntentDirective.SetAction("android.intent.action.VIEW");
        sendAndroidAppIntentDirective.SetUri("vnd.youtube:dQw4w9WgXcQ");

        auto& analytics = *sendAndroidAppIntentDirective.MutableAnalytics();
Y_UNUSED(analytics);
        auto& appLaunchMetadata = *analytics.MutableAppLaunch();
        appLaunchMetadata.SetPackageName("com.yandex.test");
        appLaunchMetadata.SetVisibleName("Test");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SEND_ANDROID_APP_INTENT_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSendAndroidAppIntentDirectiveWithoutUri) {
        TDirective directive;
        auto& sendAndroidAppIntentDirective = *directive.MutableSendAndroidAppIntentDirective();
        sendAndroidAppIntentDirective.SetName(TString{ANALYTICS_TYPE});

        auto& component = *sendAndroidAppIntentDirective.MutableComponent();
        component.SetPkg(TString{"com.yandex.test"});
        component.SetCls(TString{"com.yandex.test.Test"});

        auto& flags = *sendAndroidAppIntentDirective.MutableFlags();
        flags.SetFlagActivityNewTask(true);

        sendAndroidAppIntentDirective.SetAction("android.intent.action.VIEW");

        auto& analytics = *sendAndroidAppIntentDirective.MutableAnalytics();
Y_UNUSED(analytics);
        auto& appLaunchMetadata = *analytics.MutableAppLaunch();
        appLaunchMetadata.SetPackageName("com.yandex.test");
        appLaunchMetadata.SetVisibleName("Test");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, SEND_ANDROID_APP_INTENT_DIRECTIVE_WITHOUT_URI);
    }

    Y_UNIT_TEST(TestDeserializeDrawScledAnimationDirectiveWithoutStopPolicy) {


        TDirective directive;
        ::NAlice::NScenarios::TDrawScledAnimationsDirective& segmentAnimationDirective = *directive.MutableDrawScledAnimationsDirective();
        segmentAnimationDirective.SetName("draw_scled_animations");
        // segmentAnimationDirective.SetAnimationStopPolicy();

        auto* animationsDirective = segmentAnimationDirective.AddAnimations();

        animationsDirective->SetName("animation_1");
        animationsDirective->SetBase64EncodedValue("15 25 0");
        animationsDirective->SetCompression(TDrawScledAnimationsDirective_TAnimation_ECompressionType_None);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, REQUEST_DRAW_SCLED_ANIMATIONS_DIRECTIVE_WITHOUT_STOP_POLICY);
    }

    Y_UNIT_TEST(TestDeserializeDrawScledAnimationDirectiveWithStopPolicy) {


        TDirective directive;
        ::NAlice::NScenarios::TDrawScledAnimationsDirective& segmentAnimationDirective = *directive.MutableDrawScledAnimationsDirective();
        segmentAnimationDirective.SetName("draw_scled_animations");
        segmentAnimationDirective.SetAnimationStopPolicy(TDrawScledAnimationsDirective_EAnimationStopPolicy_PlayOnce);

        auto* animationsDirective = segmentAnimationDirective.AddAnimations();

        animationsDirective->SetName("animation_1");
        animationsDirective->SetBase64EncodedValue("15 25 0 155 \n15 5 14\n");
        animationsDirective->SetCompression( TDrawScledAnimationsDirective_TAnimation_ECompressionType_None);
        animationsDirective = segmentAnimationDirective.AddAnimations();

        animationsDirective->SetName("animation_2");
        animationsDirective->SetBase64EncodedValue("11 0 155 \n..... 2 5 14\n");
        animationsDirective->SetCompression( TDrawScledAnimationsDirective_TAnimation_ECompressionType_None);

        segmentAnimationDirective.SetSpeakingAnimationPolicy(TDrawScledAnimationsDirective_ESpeakingAnimationPolicy_ShowClockImmediately);

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, REQUEST_DRAW_SCLED_ANIMATIONS_DIRECTIVE_WITH_STOP_POLICY);
    }

    Y_UNIT_TEST(TestDeserializeStopMultiroomDirective) {
        TDirective directive{};
        auto& stopMultiroom = *directive.MutableStopMultiroomDirective();
        stopMultiroom.SetName(TString{ANALYTICS_TYPE});
        stopMultiroom.SetMultiroomSessionId("12345");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, STOP_MULTIROOM_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeFillCloudUiDirective) {
        TDirective directive{};
        auto& fillCloudUi = *directive.MutableFillCloudUiDirective();
        fillCloudUi.SetName(TString{ANALYTICS_TYPE});
        fillCloudUi.SetText(TString{TEXT});

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, FILL_CLOUD_UI_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeWebOSLaunchAppDirective) {
        TDirective directive{};
        auto& webOSLaunchApp = *directive.MutableWebOSLaunchAppDirective();
        webOSLaunchApp.SetName(TString{ANALYTICS_TYPE});
        webOSLaunchApp.SetAppId("some.app.id");
        webOSLaunchApp.SetParamsJson("{}");

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, WEB_OS_LAUNCH_APP_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeWebOSShowGalleryDirective) {
        TDirective directive{};
        auto& webOSShowGallery = *directive.MutableWebOSShowGalleryDirective();
        webOSShowGallery.SetName(TString{ANALYTICS_TYPE});
        auto& item1 = *webOSShowGallery.AddItemsJson();
        item1 = "{}";
        auto& item2 = *webOSShowGallery.AddItemsJson();
        item2 = "{}";

        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, WEB_OS_SHOW_GALLERY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestDeserializeCancelScheduleActionDirective) {
        const TString actionId{"my-lovely-action-id"};

        TServerDirective sdr;
        sdr.MutableMeta()->SetApplyFor(TServerDirective::TMeta::EApplyFor::TServerDirective_TMeta_EApplyFor_DeviceOwner);

        // Cancel action from scenario.
        auto& dr = *sdr.MutableCancelScheduledActionDirective();
        dr.MutableRemoveScheduledActionRequest()->SetActionId(actionId);

        // MM Deserizalier.
        TScenarioProtoDeserializer ds{GetSerializerMeta(), TRTLogger::StderrLogger()};
        auto model = ds.Deserialize(sdr);
        UNIT_ASSERT(model);
        UNIT_ASSERT_VALUES_EQUAL(model->GetType(), EDirectiveType::ProtobufUniproxyAction);

        // Must be TProtobufUniproxyDirectiveModel
        const auto* uniproxyModel = dynamic_cast<TProtobufUniproxyDirectiveModel*>(model.Get());
        UNIT_ASSERT(uniproxyModel);

        // Must have TProtobufUniproxyDirective.
        const auto& directives = uniproxyModel->Directives();
        const auto* csad = std::get_if<NScenarios::TCancelScheduledActionDirective>(&directives);
        UNIT_ASSERT(csad);
        UNIT_ASSERT_VALUES_EQUAL(csad->GetRemoveScheduledActionRequest().GetActionId(), actionId);

        // Must have uniproxy directive meta (because it has ApplyFor)
        UNIT_ASSERT(uniproxyModel->GetUniproxyDirectiveMeta());
    }

    Y_UNIT_TEST(TestDeserializeTAudioMultiroomAttach) {
        TDirective directive;
        directive.MutableAudioMultiroomAttach()->SetMultiroomToken(TEST_MULTIROOM_TOKEN.data(), TEST_MULTIROOM_TOKEN.size());
        ASSERT_DESERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, directive, AUDIO_MULTIROOM_ATTACH_DIRECTIVE);
    }
}

} // namespace NAlice::NMegamind
