#include "shared_ut.h"

#include "shared_resources.h"

#include <alice/megamind/library/testing/fake_guid_generator.h>
#include <alice/megamind/protos/common/smart_home.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>

#include <alice/library/frame/builder.h>
#include <alice/library/json/json.h>

#include <alice/protos/data/video/video.pb.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/hash.h>
#include <util/generic/serialized_enum.h>

namespace NAlice::NMegamind {

namespace {

TUpdateDialogInfoDirectiveStyleModel GetUpdateDialogInfoDirectiveModelStyle(const TString& colorSuffix) {
    return TUpdateDialogInfoDirectiveStyleModelBuilder()
        .AddOknyxNormalColor("TestOknyxNormalColor" + colorSuffix)
        .AddOknyxErrorColor("TestOknyxErrorColor" + colorSuffix)
        .SetUserBubbleFillColor("TestUserBubbleFillColor" + colorSuffix)
        .SetSuggestBorderColor("TestSuggestBorderColor" + colorSuffix)
        .SetOknyxLogo("TestOknyxLogo" + colorSuffix)
        .SetSkillActionsTextColor("TestSkillActionsTextColor" + colorSuffix)
        .SetSkillBubbleFillColor("TestSkillBubbleFillColor" + colorSuffix)
        .SetSkillBubbleTextColor("TestSkillBubbleTextColor" + colorSuffix)
        .SetSuggestFillColor("TestSuggestFillColor" + colorSuffix)
        .SetSuggestTextColor("TestSuggestTextColor" + colorSuffix)
        .SetUserBubbleTextColor("TestUserBubbleTextColor" + colorSuffix)
        .Build();
}

TUniversalClientDirectiveModel GetStartImageRecognizer() {
    return TUniversalClientDirectiveModel(/* name= */ "start_image_recognizer",
                                          /* analyticsType= */ TString{INNER_ANALYTICS_TYPE}, /* payload= */ {});
}

TClientInfoProto GetApplication() {
    TClientInfoProto proto;
    proto.SetDeviceId("device_id_1");
    proto.SetUuid("uuid_1");
    proto.SetAppId("ru.yandex.centaur");
    return proto;
}

TSpeechKitRequestProto::TRequest::TAdditionalOptions GetAdditionalOptions() {
    TSpeechKitRequestProto::TRequest::TAdditionalOptions additionalOptions;
    additionalOptions.SetPuid(OWNER_PUID.data(), OWNER_PUID.size());
    additionalOptions.MutableGuestUserOptions()->SetYandexUID(GUEST_PUID.data(), GUEST_PUID.size());
    return additionalOptions;
}

} // namespace

TStringBuf GetOpenSettingsSkJsonString(ESettingsTarget target) {
    static const auto baseString = TString{OPEN_SETTINGS_DIRECTIVE_MODEL_BASE};
    static THashMap<ESettingsTarget, TString> jsonStrings{};
    if (!jsonStrings.contains(target)) {
        jsonStrings[target] = Sprintf(baseString.c_str(), ToString(target).c_str());
    }
    return jsonStrings[target];
}

TFixture::TFixture()
    : SerializerMeta(TString{SCENARIO_NAME}, TString{REQUEST_ID}, TClientInfo{GetApplication()},
                     JsonToProto<TIoTUserInfo>(JsonFromString(IOT_USER_INFO)),
                     TSmartHomeInfo{},
                     GetAdditionalOptions(),
                     TFakeGuidGenerator(TString{FAKE_GUID}))
    , ScenarioProtoDeserializer(SerializerMeta, TRTLogger::StderrLogger())
    , SpeechKitProtoModelSerializer(SerializerMeta)
    , SpeechKitStructModelSerializer(SerializerMeta)
{
}

const TSerializerMeta& TFixture::GetSerializerMeta() const {
    return SerializerMeta;
}

const TScenarioProtoDeserializer& TFixture::GetScenarioProtoDeserializer() const {
    return ScenarioProtoDeserializer;
}

const TSpeechKitProtoSerializer& TFixture::GetSpeechKitProtoModelSerializer() const {
    return SpeechKitProtoModelSerializer;
}

const TSpeechKitStructSerializer& TFixture::GetSpeechKitStructModelSerializer() const {
    return SpeechKitStructModelSerializer;
}

const TActionButtonModel& TFixture::GetActionButtonModel() const {
    static const auto model = TActionButtonModelBuilder()
                                  .SetTitle(TString{TITLE})
                                  .AddDirective(GetStartImageRecognizer())
                                  .Build();
    return model;
}

const TActionButtonModel& TFixture::GetThemedActionButtonModel() const {
    static const auto model =
        TActionButtonModel(GetActionButtonModel().GetTitle(), GetActionButtonModel().GetDirectives(),
                           TActionButtonModel::TTheme(TString{IMAGE_URL}), TString{TEXT});
    return model;
}

const TDiv2CardModel& TFixture::GetDiv2CardModel() const {
    static const auto model = TDiv2CardModel(TProtoStructBuilder().Set(TString{KEY}, TString{VALUE}).Build());
    return model;
}

const TDivCardModel& TFixture::GetDivCardModel() const {
    static const auto model = TDivCardModel(TProtoStructBuilder().Set(TString{KEY}, TString{VALUE}).Build());
    return model;
}

const TTextCardModel& TFixture::GetTextCardModel() const {
    static const auto model = TTextCardModel(TString{TEXT});
    return model;
}

const TTextWithButtonCardModel& TFixture::GetTextWithButtonCardModel() const {
    static const auto model = TTextWithButtonCardModelBuilder()
                                  .SetText(TString{TEXT})
                                  .AddButton(TActionButtonModelBuilder().SetTitle(TString{TITLE}).Build())
                                  .Build();
    return model;
}

const TAlarmNewDirectiveModel& TFixture::GetAlarmNewDirectiveModel() const {
    static const auto model = TAlarmNewDirectiveModel(
        TString{ANALYTICS_TYPE}, TString{TEXT}, TProtoStructBuilder().Set(TString{KEY}, TString{VALUE}).Build(),
        TProtoStructBuilder().Set(TString{TITLE}, TString{INNER_TITLE}).Build());
    return model;
}

const TAlarmSetSoundDirectiveModel& TFixture::GetAlarmSetSoundDirectiveModel() const {
    static const auto model = TAlarmSetSoundDirectiveModel(
        TString{ANALYTICS_TYPE},
        TCallbackDirectiveModel(TString{INNER_ANALYTICS_TYPE}, /* ignoreAnswer= */ true,
                                TProtoStructBuilder().Set(TString{KEY}, TString{VALUE}).Build(),
                                /* isLedSilent= */ true),
        JsonToProto<TAlarmSetSoundDirectiveModel::TSettings>(
            JsonFromString(ALARM_SET_SOUND_DIRECTIVE)["payload"]["sound_alarm_setting"]));
    return model;
}

const TCloseDialogDirectiveModel& TFixture::GetCloseDialogDirectiveModel() const {
    static const auto model = TCloseDialogDirectiveModel(TString{ANALYTICS_TYPE}, TString{DIALOG_ID}, Nothing());
    return model;
}

const TCloseDialogDirectiveModel& TFixture::GetCloseDialogDirectiveModelWithScreenId() const {
    TMaybe<TString> screenId = "test_screen_id";
    static const auto model =
        TCloseDialogDirectiveModel(TString{ANALYTICS_TYPE}, TString{DIALOG_ID}, std::move(screenId));
    return model;
}

const TEndDialogSessionDirectiveModel& TFixture::GetEndDialogSessionDirectiveModel() const {
    static const auto model = TEndDialogSessionDirectiveModel(TString{ANALYTICS_TYPE}, TString{DIALOG_ID});
    return model;
}

const TFindContactsDirectiveModel& TFixture::GetFindContactsDirectiveModel() const {
    static const auto model = TFindContactsDirectiveModel(
        TString{ANALYTICS_TYPE}, TVector<TString>({TString{ACTION_ID}}), TVector<TString>({TString{AD_BLOCK_ID}}),
        TProtoStructBuilder().Set(TString{KEY}, TString{VALUE}).Build(),
        TVector<TFindContactsDirectiveModel::TRequestPart>(
            {TFindContactsDirectiveModel::TRequestPart(TString{DIALOG_ID}).AddValue(TString{VALUE})}),
        TVector<TString>({TString{TEXT}}), TString{VALUE});
    return model;
}

const TOpenDialogDirectiveModel& TFixture::GetOpenDialogDirectiveModel() const {
    static const auto model = TOpenDialogDirectiveModelBuilder()
                                  .SetAnalyticsType(TString{ANALYTICS_TYPE})
                                  .SetDialogId(TString{DIALOG_ID})
                                  .AddDirective(GetStartImageRecognizer())
                                  .Build();
    return model;
}

const TOpenSettingsDirectiveModel& TFixture::GetOpenSettingsDirectiveModel(ESettingsTarget target) const {
    static const auto models = []() {
        TVector<TOpenSettingsDirectiveModel> models{};
        for (const auto target : GetEnumAllValues<ESettingsTarget>()) {
            models.push_back(TOpenSettingsDirectiveModel(TString{ANALYTICS_TYPE}, target));
        }
        return models;
    }();
    return models.at(static_cast<int>(target));
}

const TUniversalClientDirectiveModel& TFixture::GetOpenUriDirectiveModel() const {
    static const auto payload = TProtoStructBuilder()
        .Set("uri", TString{URI})
        .Build();
    static const auto model = TUniversalClientDirectiveModel(/* name= */ "open_uri",
                                                             /* analyticsType= */  TString{ANALYTICS_TYPE},
                                                             /* payload= */ payload);
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetOpenUriDirectiveModelWithScreenId() const {
    static const auto payload = TProtoStructBuilder()
        .Set("uri", TString{URI})
        .Set("screen_id", TString{SCREEN_ID})
        .Build();
    static const auto model = TUniversalClientDirectiveModel(/* name= */ "open_uri",
                                                             /* analyticsType= */  TString{ANALYTICS_TYPE},
                                                             /* payload= */ payload);
    return model;
}

const TPlayerRewindDirectiveModel& TFixture::GetPlayerRewindDirectiveModel(EPlayerRewindType rewindType) const {
    static const auto models = []() {
        TVector<TPlayerRewindDirectiveModel> models{};
        for (const auto type : GetEnumAllValues<EPlayerRewindType>()) {
            models.push_back(TPlayerRewindDirectiveModel(TString{ANALYTICS_TYPE}, /* amount= */ 30, type));
        }
        return models;
    }();
    return models.at(static_cast<int>(rewindType));
}

const TUniversalClientDirectiveModel& TFixture::GetShowGalleryDirectiveModel() const {
    const auto directive = NAlice::JsonFromString(SHOW_GALLERY_DIRECTIVE);
    TProtoListBuilder listBuilder;
    for (const auto& item : directive["payload"]["items"].GetArray()) {
        listBuilder.Add(JsonToProto<google::protobuf::Struct>(item));
    }
    static const auto model = TUniversalClientDirectiveModel(
        "show_gallery", "video_show_gallery", TProtoStructBuilder().Set("items", listBuilder.Build()).Build());
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetShowPayPushScreenDirectiveModel() const {
    const auto directive = NAlice::JsonFromString(SHOW_PAY_PUSH_SCREEN_DIRECTIVE);
    auto itemStruct = JsonToProto<google::protobuf::Struct>(directive["payload"]["item"]);
    static const auto model = TUniversalClientDirectiveModel("show_pay_push_screen", "video_show_pay_push_screen",
                                                             TProtoStructBuilder().Set("item", itemStruct).Build());
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetShowSeasonGalleryDirectiveModel() const {
    const auto directive = NAlice::JsonFromString(SHOW_SEASON_GALLERY_DIRECTIVE);
    TProtoListBuilder listBuilder;
    for (const auto& item : directive["payload"]["items"].GetArray()) {
        listBuilder.Add(JsonToProto<google::protobuf::Struct>(item));
    }
    auto tvShowItemStruct = JsonToProto<google::protobuf::Struct>(directive["payload"]["tv_show_item"]);
    static const auto model =
        TUniversalClientDirectiveModel("show_season_gallery", "video_show_season_gallery",
                                       TProtoStructBuilder()
                                           .Set("items", listBuilder.Build())
                                           .Set("tv_show_item", tvShowItemStruct)
                                           .SetUInt("season", directive["payload"]["season"].GetUInteger())
                                           .Build());
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetShowVideoDescriptionDirectiveModel() const {
    const auto directive = NAlice::JsonFromString(SHOW_VIDEO_DESCRIPTION_DIRECTIVE);
    auto itemStruct = JsonToProto<google::protobuf::Struct>(directive["payload"]["item"]);
    static const auto model = TUniversalClientDirectiveModel("show_description", "video_show_description",
                                                             TProtoStructBuilder().Set("item", itemStruct).Build());
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetVideoPlayDirectiveModel() const {
    const auto directive = NAlice::JsonFromString(VIDEO_PLAY_DIRECTIVE);
    auto itemStruct = JsonToProto<google::protobuf::Struct>(directive["payload"]["item"]);
    auto nextItemStruct = JsonToProto<google::protobuf::Struct>(directive["payload"]["next_item"]);
    static const auto model = TUniversalClientDirectiveModel("video_play", "video_play",
                                                             TProtoStructBuilder()
                                                                 .Set("item", itemStruct)
                                                                 .Set("next_item", nextItemStruct)
                                                                 .Set("uri", directive["payload"]["uri"].GetString())
                                                                 .Build());
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetCarDirectiveModel() const {
    static const auto model =
        TUniversalClientDirectiveModel("car", TString{ANALYTICS_TYPE},
                                       TProtoStructBuilder()
                                           .Set("application", "car")
                                           .Set("intent", "media_select")
                                           .Set("params", TProtoStructBuilder().Set("radio", "Эхо Москвы").Build())
                                           .Build());
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetSoundSetLevelDirectiveModel() const {
    static const auto model = TUniversalClientDirectiveModel(
        "sound_set_level",
        TString{ANALYTICS_TYPE},
        TProtoStructBuilder().SetInt("new_level", 7).Build(),
        /* multiroomSessionId= */ Nothing()
    );
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetSoundSetLevelInLocationDirectiveModel() const {
    static const auto model = TUniversalClientDirectiveModel(
        "sound_set_level",
        TString{ANALYTICS_TYPE},
        TProtoStructBuilder().SetInt("new_level", 6).Set("room_id", "kitchen").Build(),
        /* multiroomSessionId= */ Nothing(),
        /* roomId */ "kitchen"
    );
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetSoundSetLevelDirectiveModelWithMultiroom() const {
    static const auto model = TUniversalClientDirectiveModel(
        "sound_set_level",
        TString{ANALYTICS_TYPE},
        TProtoStructBuilder().SetInt("new_level", 7).Build(),
        /* multiroomSessionId= */ TMaybe<TString>{"12345"}
    );
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetStartMultiroomDirectiveModelWithLocationInfoAndIncludeCurrentDevice() const {
    const auto roomDeviceIds = TProtoListBuilder().Add("device_id_1").Add("device_id_3").Add("device_id_4").Build();
    const auto payload = TProtoStructBuilder().Set("room_device_ids", roomDeviceIds).Set("room_id", "room_2").Build();

    static const auto model = TUniversalClientDirectiveModel(
        "start_multiroom",
        TString{ANALYTICS_TYPE},
        payload,
        Nothing(),
        Nothing(),
        // LocationInfo is Nothing(), since we want the room_device_ids to be filled only in the payload, not in the root
        // We get this kind of directive_model from deserializer anyway;
        // TODO(igor-darov, sparkle): Maybe introduce special directive_model for the start_multiroom directive
        Nothing()
    );
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetYandexNaviDirectiveModel() const {
    static const auto model =
        TUniversalClientDirectiveModel("yandexnavi", TString{ANALYTICS_TYPE},
                                       TProtoStructBuilder()
                                           .Set("application", "yandexnavi")
                                           .Set("intent", "map_search")
                                           .Set("params", TProtoStructBuilder().Set("text", TString{TEXT}).Build())
                                           .Build());
    return model;
}

const TThereminPlayDirectiveModel& TFixture::GetThereminPlayDirectiveModelWithExternalSet() const {
    static const auto model = TThereminPlayDirectiveModel(
        TString{ANALYTICS_TYPE},
        MakeIntrusive<TThereminPlayDirectiveExternalSetModel>(
            /* noOverlaySamples= */ true, /* repeatSoundInside= */ true, /* stopOnCeil= */ true,
            /* samples= */ TVector{TThereminPlayDirectiveExternalSetSampleModel(TString{URI})}));
    return model;
}

const TThereminPlayDirectiveModel& TFixture::GetThereminPlayDirectiveModelWithInternalSet() const {
    static const auto model = TThereminPlayDirectiveModel(
        TString{ANALYTICS_TYPE}, MakeIntrusive<TThereminPlayDirectiveInternalSetModel>(/* mode= */ 324));
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetTypeTextDirectiveModel() const {
    static const auto model = TUniversalClientDirectiveModel(/* name= */ "type",
                                                             /* analyticsType= */ TString{ANALYTICS_TYPE},
                                                             /* payload= */ TProtoStructBuilder().Set("text", TString{TEXT}).Build());
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetTypeTextSilentDirectiveModel() const {
    static const auto model = TUniversalClientDirectiveModel(/* name= */ "type_silent",
                                                             /* analyticsType= */ TString{ANALYTICS_TYPE},
                                                             /* payload= */ TProtoStructBuilder().Set("text", TString{TEXT}).Build());
    return model;
}

const TUpdateDialogInfoDirectiveModel& TFixture::GetUpdateDialogInfoDirectiveModel() const {
    static const auto model = TUpdateDialogInfoDirectiveModel(
        TString{ANALYTICS_TYPE}, TString{TITLE}, TString{URI}, TString{IMAGE_URL},
        GetUpdateDialogInfoDirectiveModelStyle(""), GetUpdateDialogInfoDirectiveModelStyle("Dark"),
        {TUpdateDialogInfoDirectiveMenuItemModel(TString{INNER_TITLE}, TString{INNER_URL})}, TString{AD_BLOCK_ID});
    return model;
}

const TCallbackDirectiveModel& TFixture::GetCallbackDirectiveModel() const {
    static const auto model = TCallbackDirectiveModel(TString{ANALYTICS_TYPE}, /* ignoreAnswer= */ true,
                                                      TProtoStructBuilder().Set(TString{KEY}, TString{VALUE}).Build(),
                                                      /* isLedSilent= */ true);
    return model;
}

const TCallbackDirectiveModel& TFixture::GetCallbackDirectiveModelWithMultiroom() const {
    static const auto model = TCallbackDirectiveModel(TString{ANALYTICS_TYPE},
                                                      /* ignoreAnswer= */ false,
                                                      TProtoStructBuilder().Set(TString{KEY}, TString{VALUE}).Build(),
                                                      /* isLedSilent= */ true,
                                                      /* multiroomSessionId= */ "12345");
    return model;
}

const TGetNextCallbackDirectiveModel& TFixture::GetGetNextCallbackDirectiveModel() const {
    static const auto model = TGetNextCallbackDirectiveModel(/* ignoreAnswer= */ false,
                                                             /* isLedSilent= */ true,
                                                             /* sessionId= */ "session_id",
                                                             /* productScenarioName= */ "product_scenario_name",
                                                             /* recoveryCallback= */ Nothing(),
                                                             /* multiroomSessionId= */ "12345");
    return model;
}

const TDeferApplyDirectiveModel& TFixture::GetDeferApplyDirectiveModel() const {
    static const auto model = TDeferApplyDirectiveModel("TestSession");
    return model;
}

const TMusicPlayDirectiveModel& TFixture::GetMusicPlayDirectiveModel() const {
    static const auto model = TMusicPlayDirectiveModel(TString{ANALYTICS_TYPE}, TString{UID}, TString{SESSION_ID}, 42,
                                                       "TestAlarmId", "TestFirstTrackId", "room_1", Nothing());
    return model;
}

const TMusicPlayDirectiveModel& TFixture::GetMusicPlayDirectiveModelWithEndpointId() const {
    static auto model = TMusicPlayDirectiveModel(TString{ANALYTICS_TYPE}, TString{UID}, TString{SESSION_ID}, 42,
                                                       "TestAlarmId", "TestFirstTrackId", "room_1", Nothing());
    model.SetEndpointId("kekid");
    return model;
}

const TMusicPlayDirectiveModel& TFixture::GetMusicPlayDirectiveModel2() const {
    static const auto model =
        TMusicPlayDirectiveModel(TString{ANALYTICS_TYPE}, TString{UID}, TString{SESSION_ID},
                                 42, Nothing(), Nothing(), "room_2", Nothing());
    return model;
}

const TMusicPlayDirectiveModel& TFixture::GetMusicPlayDirectiveModelEmptyRoom() const {
    static const auto model =
        TMusicPlayDirectiveModel(/* analyticsType= */ TString{ANALYTICS_TYPE},
                                 /* uid= */ TString{UID},
                                 /* sessionId= */ TString{SESSION_ID},
                                 /* offset= */ 42,
                                 /* alarmId= */ Nothing(),
                                 /* firstTrackId= */ Nothing(),
                                 /* roomId= */ Nothing(),
                                 /* locationInfo= */ Nothing());
    return model;
}

const TMusicPlayDirectiveModel& TFixture::GetMusicPlayDirectiveModelWithLocationInfo() const {
    NScenarios::TLocationInfo locationInfo;
    locationInfo.AddRoomsIds("room_1");
    locationInfo.AddGroupsIds("group_2");

    static const auto model =
        TMusicPlayDirectiveModel(TString{ANALYTICS_TYPE}, TString{UID}, TString{SESSION_ID},
                                 42, Nothing(), Nothing(), Nothing(), locationInfo);
    return model;
}

const TSetCookiesDirectiveModel& TFixture::GetSetCookiesDirectiveModel() const {
    static const auto model = TSetCookiesDirectiveModel(
        TString{ANALYTICS_TYPE}, R"({"uaas": "t123456.e1600000000.sDEADBEEF123456DEADBEEF123456DEAD"})");
    return model;
}

const TSetSearchFilterDirectiveModel& TFixture::GetSetSearchFilterDirectiveModel(ESearchFilterLevel level) const {
    static const auto models = []() {
        TVector<TSetSearchFilterDirectiveModel> models{};
        for (const auto level : GetEnumAllValues<ESearchFilterLevel>()) {
            models.push_back(TSetSearchFilterDirectiveModel(TString{ANALYTICS_TYPE}, level));
        }
        return models;
    }();
    return models.at(static_cast<int>(level));
}

const TAddContactBookAsrDirectiveModel& TFixture::GetAddContactBookAsrDirectiveModel() const {
    static const auto model = TAddContactBookAsrDirectiveModel();
    return model;
}

const TSetTimerDirectiveModel& TFixture::GetSetTimerDirectiveModel(bool setTimestamp) const {
    static const auto model =
        TSetTimerDirectiveModel(TString{ANALYTICS_TYPE}, /* duration= */ 42, /* listeningIsPossible= */ true,
                                /* timestamp= */ 0, TProtoStructBuilder().Set(TString{KEY}, TString{VALUE}).Build(),
                                TProtoStructBuilder().Set(TString{TITLE}, TString{INNER_TITLE}).Build(),
                                /* directives= */ {});
    static const auto timestampModel = TSetTimerDirectiveModel(
        TString{ANALYTICS_TYPE}, /* duration= */ 0, /* listeningIsPossible= */ true,
        /* timestamp= */ 42, TProtoStructBuilder().Set(TString{KEY}, TString{VALUE}).Build(),
        TProtoStructBuilder().Set(TString{TITLE}, TString{INNER_TITLE}).Build(),
        /* directives= */ {MakeIntrusive<TUniversalClientDirectiveModel>(GetTypeTextDirectiveModel())});
    return setTimestamp ? timestampModel : model;
}

const TUniversalClientDirectiveModel& TFixture::GetVisualGoBackwardDirectiveModel() const {
    NAlice::NScenarios::TGoBackwardDirective goBackwardDirective;
    goBackwardDirective.SetName("go_back");
    goBackwardDirective.MutableVisualMode();

    auto builder = MessageToStructBuilder(goBackwardDirective);
    builder.Drop("name");

    static const auto model = TUniversalClientDirectiveModel("go_back", "go_back", builder.Build());
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetHistoricalGoBackwardDirectiveModel() const {
    NAlice::NScenarios::TGoBackwardDirective goBackwardDirective;
    goBackwardDirective.SetName("go_back");
    auto historicalMode = goBackwardDirective.MutableHistoricalMode();
    historicalMode->SetHistoryUrl("http://example.com");

    auto builder = MessageToStructBuilder(goBackwardDirective);
    builder.Drop("name");

    static const auto model = TUniversalClientDirectiveModel("go_back", "go_back", builder.Build());
    return model;
}

const TUniversalClientDirectiveModel& TFixture::GetNativeGoBackwardDirectiveModel() const {
    NAlice::NScenarios::TGoBackwardDirective goBackwardDirective;
    goBackwardDirective.SetName("go_back");
    goBackwardDirective.MutableNativeMode();

    auto builder = MessageToStructBuilder(goBackwardDirective);
    builder.Drop("name");

    static const auto model = TUniversalClientDirectiveModel("go_back", "go_back", builder.Build());
    return model;
}

const TAudioPlayDirectiveModel& TFixture::GetAudioPlayDirectiveModel() const {

    auto normalization = TAudioPlayDirectiveStreamNormalizationModel(-0.7, -0.13);

    auto stream = TAudioPlayDirectiveStreamModel(TString{"token"}, TString{"https://s3.yandex.net/record.mp3"},
            5000, EStreamFormat::MP3, EStreamType::Track, normalization);

    auto prevTrackInfo = TPrevNextTrackInfo("12344", EStreamType::Track);
    auto nextTrackInfo = TPrevNextTrackInfo("12346", EStreamType::Track);
    auto innerGlagolMetadata = MakeIntrusive<TMusicMetadataModel>(TString{"12345"}, EContentType::Track, TString{"Описание"},
                                                                  prevTrackInfo, nextTrackInfo, true,
                                                                  ERepeatMode::All);
    auto glagolMetadata = TGlagolMetadataModel(innerGlagolMetadata);

    auto audioPlayMetadata = TAudioPlayDirectiveMetadataModel(TString{"title"}, TString{"subtitle"},
            TString{"https://img.jpg"}, glagolMetadata, /* hideProgressBar= */ false);

    auto callbacks = TAudioPlayDirectiveCallbacksModel(
            MakeIntrusive<TCallbackDirectiveModel>(TString{"on_started"}, true,
                                                   TProtoStructBuilder().Set(TString{SKILL_ID_KEY}, TString{SKILL_ID_VALUE})
                                                   .Build(), /* isLedSilent= */ true),
            MakeIntrusive<TCallbackDirectiveModel>(TString{"on_stopped"}, true,
                                                   TProtoStructBuilder().Set(TString{SKILL_ID_KEY}, TString{SKILL_ID_VALUE})
                                                   .Build(), /* isLedSilent= */ true),
            MakeIntrusive<TCallbackDirectiveModel>(TString{"on_finished"}, true,
                                                   TProtoStructBuilder().Set(TString{SKILL_ID_KEY}, TString{SKILL_ID_VALUE})
                                                   .Build(), /* isLedSilent= */ true),
            MakeIntrusive<TCallbackDirectiveModel>(TString{"on_failed"}, true,
                                                   TProtoStructBuilder().Set(TString{SKILL_ID_KEY}, TString{SKILL_ID_VALUE})
                                                   .Build(), /* isLedSilent= */ true));

    THashMap<TString, TString> scenarioMeta{};

    scenarioMeta[TString{SKILL_ID_KEY}] =  TString{SKILL_ID_VALUE};

    static const auto model = TAudioPlayDirectiveModel(TString{ANALYTICS_TYPE}, stream, audioPlayMetadata,
                                                       callbacks, scenarioMeta, EBackgroundMode::Ducking,
                                                       TString{"ЛитРес"}, EScreenType::Default, /* setPause= */ false,
                                                       /* multiroomToken= */ TString{TEST_MULTIROOM_TOKEN});
    return model;
}

const TShowButtonsDirectiveModel& TFixture::GetShowButtonsDirectiveModel() const {
    static const auto buttons = [](){
        TVector<TIntrusivePtr<IButtonModel>> buttons;
        for (int i = 0; i < 2; ++i) {
            const auto num = ToString(i);

            TVector<TIntrusivePtr<IDirectiveModel>> directives;

            // add "type" directive
            directives.push_back(MakeIntrusive<TUniversalClientDirectiveModel>(/* name= */ "type",
                                                                               /* analyticsType= */ "",
                                                                               /* payload= */ TProtoStructBuilder().Set("text", TString{TYPE_TEXT}).Build()));

            // add callback directive
            directives.push_back(MakeIntrusive<TCallbackDirectiveModel>(/* name= */ "external_source_action",
                                                                        /* ignoreAnswer= */ true,
                                                                        /* payload= */ TProtoStructBuilder().Set("utm_source", "Yandex_Alisa").Build(),
                                                                        /* isLedSilent= */ false));

            buttons.push_back(MakeIntrusive<TActionButtonModel>(
                /* title= */ TString::Join(TITLE, "_", num),
                /* directives= */ std::move(directives),
                /* theme= */ TActionButtonModel::TTheme(TString::Join(IMAGE_URL, "_", num)),
                /* text= */ TString::Join(TEXT, "_", num)
            ));
        }
        return buttons;
    }();

    static const auto model = TShowButtonsDirectiveModel(/* analyticsType= */ TString{ANALYTICS_TYPE},
                                                         /* buttons= */ buttons,
                                                         /* screenId= */ TString{SCREEN_ID});
    return model;
}

const TUpdateSpaceActionsDirectiveModel& TFixture::GetUpdateSpaceActionsDirectiveModel() const {
    static const auto model = [] {
        TUpdateSpaceActionsDirectiveModel model;
        TTypedSemanticFrame semanticFrame;
        semanticFrame.MutableSearchSemanticFrame()->MutableQuery()->SetStringValue("how are you?");
        model.AddActionSpace("action_space_id",
                             {{"frame_name", TTypedSemanticFrameRequestDirectiveModel(semanticFrame, {}, {}, {})}});
        return model;
    }();
    return model;
}

const TAddConditionalActionsDirectiveModel& TFixture::GetAddConditionalActionsDirectiveModel() const {
    static const auto model = [] {
        TAddConditionalActionsDirectiveModel model;
        TConditionalAction conditionalAction1;
        conditionalAction1.MutableConditionalSemanticFrame()->MutableSearchSemanticFrame()->MutableQuery()->SetStringValue("how are you?");
        conditionalAction1.MutableEffectFrameRequestData()->MutableTypedSemanticFrame()->MutableSearchSemanticFrame()->MutableQuery()->SetStringValue("ok");
        model.AddConditionalAction("action_1", conditionalAction1);

        TConditionalAction conditionalAction2;
        conditionalAction2.MutableConditionalSemanticFrame()->MutablePlayerPauseSemanticFrame();
        conditionalAction2.MutableEffectFrameRequestData()->MutableTypedSemanticFrame()->MutableSearchSemanticFrame()->MutableQuery()->SetStringValue("net");
        model.AddConditionalAction("action_2", conditionalAction2);

        return model;
    }();
    return model;
}

const TAddExternalEntitiesDescriptionDirectiveModel& TFixture::GetAddExternalEntitiesDescriptionDirectiveModel() const {
    static const auto model = [] {
        TAddExternalEntitiesDescriptionDirectiveModel model;
        NData::TExternalEntityDescription entity;
        entity.SetName("entity_name");
        auto& entityItem = *entity.AddItems();
        entityItem.MutableValue()->SetStringValue("item_value");
        auto& entityItemPhrase = *entityItem.AddPhrases();
        entityItemPhrase.SetPhrase("hi");
        model.AddExternalEntityDescription(entity);
        return model;
    }();
    return model;
}

} // namespace NAlice::NMegamind
