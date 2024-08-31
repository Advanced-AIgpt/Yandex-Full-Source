#include "centaur_main_screen.h"
#include "util/generic/fwd.h"

#include <alice/hollywood/library/combinators/request/request.h>
#include <alice/hollywood/library/combinators/protos/used_scenarios.pb.h>
#include <alice/hollywood/library/testing/mock_global_context.h>

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/library/unittest/mock_sensors.h>
#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/megamind/protos/scenarios/combinator_request.pb.h>
#include <alice/megamind/protos/scenarios/combinator_response.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/memento/proto/api.pb.h>
#include <alice/memento/proto/device_configs.pb.h>
#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/data/scenario/centaur/my_screen/widgets.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/data/scenario/music/infinite_feed.pb.h>
#include <alice/protos/data/scenario/news/news.pb.h>
#include "alice/protos/data/scenario/video_call/video_call.pb.h"
#include <alice/protos/data/scenario/weather/weather.pb.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <apphost/lib/service_testing/service_testing.h>
#include <apphost/lib/proto_answers/http.pb.h>

#include <util/stream/file.h>

#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/frame_request_params.pb.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>

namespace NAlice::NHollywood {

namespace {

const TStringBuf DELIM = "\n=======================\n";

TFsPath GetDataPath(TString canonizedFileName) {
    return TFsPath(ArcadiaSourceRoot()) / "alice/hollywood/library/combinators/combinators/centaur/ut/data/" / canonizedFileName;
}

const TString MAIN_SCREEN_SERVER_UPDATE_EXP_FLAG_NAME = "centaur_main_screen_server_updates";
const TString SERVICES_TAB_SCENARIO_DATA_EXP_FLAG_NAME = "services_tab_scenario_data";
const TString VIDEO_CALL_WIDGET_EXP_FLAG_NAME = "video_call_widget";
const TString DEVICE_ID = "device_1";

const TString YOUTUBE_PACKAGE_NAME = "com.yandex.tv.ytplayer";

namespace NMemento = ru::yandex::alice::memento::proto;

void PutMusicScenarioResponses(NAppHost::NService::TTestContext& serviceCtx) {
    NScenarios::TScenarioRunResponse response;
    auto& musicScenarioData = *response.MutableResponseBody()->MutableScenarioData()->MutableMusicInfiniteFeedData();
    {
        auto& objectBlock = *musicScenarioData.AddMusicObjectsBlocks();
        objectBlock.SetTitle("block_title_1");
        objectBlock.SetType("inf_feed_play_contexts");
        {
            auto& musicObject = *objectBlock.AddMusicObjects()->MutableAutoPlaylist();
            musicObject.SetUid("AutoPlaylist_Uid");
            musicObject.SetImageUrl("AutoPlaylist_ImageUrl%%.svg");
            musicObject.SetTitle("AutoPlaylist_Title");
            musicObject.SetKind("AutoPlaylist_Kind");
            musicObject.SetModified("AutoPlaylist_Modified");
        }
        {
            auto& musicObject = *objectBlock.AddMusicObjects()->MutablePlaylist();
            musicObject.SetUid("Playlist_Uid");
            musicObject.SetImageUrl("Playlist_ImageUrl%%.svg");
            musicObject.SetTitle("Playlist_Title");
            musicObject.SetKind("Playlist_Kind");
            musicObject.SetModified("Playlist_Modified");
            musicObject.SetLikesCount(666);
        }
    }
    {
        auto& objectBlock = *musicScenarioData.AddMusicObjectsBlocks();
        objectBlock.SetTitle("block_title_2");
        objectBlock.SetType("inf_feed_liked_artists");
        {
            auto& musicObject = *objectBlock.AddMusicObjects()->MutableArtist();
            musicObject.SetId("Artist_Id");
            musicObject.SetImageUrl("Atrist_ImageUrl%%.svg");
            musicObject.SetName("Artist_Name");
            *musicObject.AddGenres() = "Artist_Genre_1";
            *musicObject.AddGenres() = "Artist_Genre_2";
        }
        {
            auto& musicObject = *objectBlock.AddMusicObjects()->MutableAlbum();
            musicObject.SetId("Album_Id");
            musicObject.SetImageUrl("Album_ImageUrl%%.svg");
            musicObject.SetTitle("Album_Title");
            musicObject.SetReleaseDate("Album_ReleaseDate");
            musicObject.SetLikesCount(333);
            musicObject.AddArtists()->SetName("Album_Artist_1");
            musicObject.AddArtists()->SetName("Album_Artist_2");
        }
    }
    {
        auto& objectBlock = *musicScenarioData.AddMusicObjectsBlocks();
        objectBlock.SetTitle("block_title_3");
        objectBlock.SetType("inf_feed_auto_playlists");
        {
            auto& musicObject = *objectBlock.AddMusicObjects()->MutableAutoPlaylist();
            musicObject.SetUid("AutoPlaylist2_Uid");
            musicObject.SetImageUrl("AutoPlaylist2_ImageUrl%%.svg");
            musicObject.SetTitle("AutoPlaylist2_Title");
            musicObject.SetKind("AutoPlaylist2_Kind");
            musicObject.SetModified("AutoPlaylist2_Modified");
        }
        {
            auto& musicObject = *objectBlock.AddMusicObjects()->MutableAutoPlaylist();
            musicObject.SetUid("AutoPlaylist3_Uid");
            musicObject.SetImageUrl("AutoPlaylist3_ImageUrl%%.svg");
            musicObject.SetTitle("AutoPlaylist3_Title");
            musicObject.SetKind("AutoPlaylist3_Kind");
            musicObject.SetModified("AutoPlaylist3_Modified");
        }
    }

    serviceCtx.AddProtobufItem(response, "scenario_HollywoodMusic_run_pure_response", NAppHost::EContextItemKind::Input);
}

NAlice::TIoTUserInfo GenerateIoTUserInfo() {
    NAlice::TIoTUserInfo iotScenarioData;
    {
        auto* device = iotScenarioData.AddDevices();
        device->SetId("device.1");
        auto* capability = device->MutableCapabilities()->Add();
        capability->SetType(::NAlice::TIoTUserInfo_TCapability_ECapabilityType_OnOffCapabilityType);
    }
    {
        auto* device = iotScenarioData.AddDevices();
        device->SetId("device.2");
        auto* capability = device->MutableCapabilities()->Add();
        capability->SetType(::NAlice::TIoTUserInfo_TCapability_ECapabilityType_ToggleCapabilityType);
    }
    return iotScenarioData;
}

void PutIoTScenarioResponses(NAppHost::NService::TTestContext& serviceCtx) {
    NScenarios::TScenarioRunResponse response;
    response.MutableResponseBody()->MutableScenarioData()->MutableIoTUserData()->CopyFrom(GenerateIoTUserInfo());
    serviceCtx.AddProtobufItem(response, "scenario_IoTScenarios_run_pure_response", NAppHost::EContextItemKind::Input);
}

void PutIoTUserInfo(NAppHost::NService::TTestContext& serviceCtx) {
    PutIoTScenarioResponses(serviceCtx);
}

void PutBlackBoxResponse(NAppHost::NService::TTestContext& serviceCtx) {
    NAppHostHttp::THttpResponse bbResponse;
    bbResponse.SetContent(R"({"uid":{"value":"11111111"}})");
    bbResponse.SetStatusCode(200);
    serviceCtx.AddProtobufItem(bbResponse, "blackbox_http_response");
}

void PutWeatherScenarioResponses(NAppHost::NService::TTestContext& serviceCtx) {
    NScenarios::TScenarioRunResponse response;
    auto& widgetData = *response.MutableResponseBody()->MutableScenarioData()->MutableCentaurScenarioWidgetData();
    widgetData.SetWidgetType("weather");
    auto& cardData = *widgetData.AddWidgetCards();
    auto& weatherCardData = *cardData.MutableWeatherCardData();

    weatherCardData.SetCity("Москва");
    weatherCardData.SetTemperature(-1);
    weatherCardData.SetImage("https://yastatic.net/weather/i/icons/portal/png/128x128/light/bkn_n.png");
    weatherCardData.SetComment("облачно с прояснениями");
    weatherCardData.SetSunrise("07:52");
    weatherCardData.SetSunset("17:34");
    weatherCardData.SetUserTime("15:16");
    weatherCardData.MutableCondition()->SetCloudness(0.5);
    weatherCardData.MutableCondition()->SetPrecStrength(15);

    NScenarios::TFrameAction onClickAction;
    auto* parsedUtterance = onClickAction.MutableParsedUtterance();
    parsedUtterance->MutableTypedSemanticFrame()->MutableWeatherSemanticFrame();
    parsedUtterance->MutableParams()->SetDisableOutputSpeech(true);
    parsedUtterance->MutableParams()->SetDisableShouldListen(true);

    auto* analytics = parsedUtterance->MutableAnalytics();
    analytics->SetProductScenario("CentaurMainScreen");
    analytics->SetPurpose("open_weather");
    analytics->SetOrigin(TAnalyticsTrackingModule_EOrigin_SmartSpeaker);

    const TString frameActionId = "OnClickMainScreenWeatherCard";
    cardData.SetAction("@@mm_deeplink#" + frameActionId);
    auto& actions = *response.MutableResponseBody()->MutableFrameActions();
    actions[frameActionId] = std::move(onClickAction);

    serviceCtx.AddProtobufItem(response, "scenario_Weather_run_pure_response", NAppHost::EContextItemKind::Input);
}

void PutNewsScenarioResponses(NAppHost::NService::TTestContext& serviceCtx) {
    NScenarios::TScenarioRunResponse response;

    auto& widgetData = *response.MutableResponseBody()->MutableScenarioData()->MutableCentaurScenarioWidgetData();
    widgetData.SetWidgetType("news");
    auto& newsCardData = *widgetData.AddWidgetCards()->MutableNewsCardData();

    newsCardData.SetTitle("N plus 1");
    newsCardData.SetContent("Твердость эластичного сплава увеличилась при нагревании");
    newsCardData.SetTopic("nplus1");

    serviceCtx.AddProtobufItem(response, "scenario_News_run_pure_response", NAppHost::EContextItemKind::Input);
}

void PutVideoCallScenarioResponses(NAppHost::NService::TTestContext& serviceCtx) {
    NScenarios::TScenarioRunResponse response;

    auto& telegramMainScreenData = *response.MutableResponseBody()->MutableScenarioData()->MutableVideoCallMainScreenData()->MutableTelegramCardData();
    telegramMainScreenData.SetLoggedIn(true);
    telegramMainScreenData.SetContactsUploaded(true);
    telegramMainScreenData.SetUserId("user_id");

    {
        auto& favoriteContact = *telegramMainScreenData.AddFavoriteContactData();
        favoriteContact.SetDisplayName("Display Name 1");
        favoriteContact.SetUserId("user_id_1");
        favoriteContact.SetLookupKey("lookup_key_1");
    }
    {
        auto& favoriteContact = *telegramMainScreenData.AddFavoriteContactData();
        favoriteContact.SetDisplayName("Display Name 2");
        favoriteContact.SetUserId("user_id_2");
        favoriteContact.SetLookupKey("lookup_key_2");
    }
    serviceCtx.AddProtobufItem(response, "scenario_VideoCall_run_pure_response", NAppHost::EContextItemKind::Input);
}

void PutWebviewScenarioResponses(NAppHost::NService::TTestContext& serviceCtx) {
    NScenarios::TScenarioRunResponse response;

    auto& widgetData = *response.MutableResponseBody()->MutableScenarioData()->MutableCentaurScenarioWidgetData();
    widgetData.SetWidgetType("webview");
    auto& cardData = *widgetData.AddWidgetCards();
    cardData.MutableYouTubeCardData();

    NScenarios::TFrameAction onClickAction;
    auto* parsedUtterance = onClickAction.MutableParsedUtterance();
    auto* frame = parsedUtterance->MutableTypedSemanticFrame()->MutableOpenSmartDeviceExternalAppFrame();
    frame->MutableApplication()->SetExternalAppValue(YOUTUBE_PACKAGE_NAME);
    parsedUtterance->MutableParams()->SetDisableOutputSpeech(true);
    parsedUtterance->MutableParams()->SetDisableShouldListen(true);

    auto* analytics = parsedUtterance->MutableAnalytics();
    analytics->SetProductScenario("CentaurMainScreen");
    analytics->SetPurpose("open_youtube");
    analytics->SetOrigin(TAnalyticsTrackingModule_EOrigin_SmartSpeaker);

    const TString frameActionId = "OnClickMainScreenYouTubeCard";
    cardData.SetAction("@@mm_deeplink#" + frameActionId);
    auto& actions = *response.MutableResponseBody()->MutableFrameActions();
    actions[frameActionId] = std::move(onClickAction);

    serviceCtx.AddProtobufItem(response, "scenario_Webview_run_pure_response", NAppHost::EContextItemKind::Input);
}

NMemento::TCentaurWidgetsDeviceConfig PrepareDefaultCentaurWidgetsDeviceConfig() {
    NMemento::TCentaurWidgetsDeviceConfig centaurWidgetsDeviceConfig;
    
    {
        auto& column = *centaurWidgetsDeviceConfig.AddColumns();
        auto& widgetConfigData = *column.AddWidgetConfigs();
        widgetConfigData.SetWidgetType("music");
        widgetConfigData.SetFixed(true);
    }
    {
        auto& column = *centaurWidgetsDeviceConfig.AddColumns();
        {
            auto& widgetConfigData = *column.AddWidgetConfigs();
            widgetConfigData.SetWidgetType("notification");
        }
        {
            auto& widgetConfigData = *column.AddWidgetConfigs();
            widgetConfigData.SetWidgetType("weather");
        }
    }
    {
        auto& column = *centaurWidgetsDeviceConfig.AddColumns();
        {
            auto& widgetConfigData = *column.AddWidgetConfigs();
            widgetConfigData.SetWidgetType("webview");
        }
        {
            auto& widgetConfigData = *column.AddWidgetConfigs();
            widgetConfigData.SetWidgetType("vacant");
        }
    }

    return centaurWidgetsDeviceConfig;
}

void TestGenerateMainScreenRun(THashSet<TString> experimentNames, TString canonizedFileName) {
    Cerr << "You can canonize this test using '--test-param canonize=true\'" << Endl;
    TMockGlobalContext globalCtx;

    NAppHost::NService::TTestContext serviceCtx;

    NScenarios::TCombinatorRequest request;
    auto& exps = *request.MutableBaseRequest()->MutableExperiments()->mutable_fields();
    for (const auto& expName : experimentNames) {
        exps[TString{expName}].set_string_value("1");
    }

    auto& clientInfo = *request.MutableBaseRequest()->MutableClientInfo();
    clientInfo.SetDeviceId("7977c01a-4885-11ec-81d3-0242ac130003");
    PutBlackBoxResponse(serviceCtx);

    auto& collectMainScreenSemanticFrame = *request.MutableInput()->AddSemanticFrames();
    collectMainScreenSemanticFrame.SetName("alice.centaur.collect_main_screen");
    collectMainScreenSemanticFrame.MutableTypedSemanticFrame()->MutableCentaurCollectMainScreenSemanticFrame();

    request.MutableBaseRequest()->MutableClientInfo()->SetDeviceId(DEVICE_ID);
    NMemento::TSurfaceConfig surfaceConfig;
    *surfaceConfig.MutableCentaurWidgetsDeviceConfig() = PrepareDefaultCentaurWidgetsDeviceConfig();
    NMemento::TRespGetAllObjects fullMementoData;
    (*fullMementoData.MutableSurfaceConfigs())[DEVICE_ID] = surfaceConfig;
    serviceCtx.AddProtobufItem(fullMementoData, "full_memento_data");

    request.MutableBaseRequest()->SetRandomSeed(1138);

    serviceCtx.AddProtobufItem(request, "combinator_request_apphost_type");

    PutMusicScenarioResponses(serviceCtx);
    PutIoTUserInfo(serviceCtx);
    PutWeatherScenarioResponses(serviceCtx);
    PutNewsScenarioResponses(serviceCtx);
    PutWebviewScenarioResponses(serviceCtx);
    PutVideoCallScenarioResponses(serviceCtx);

    THwServiceContext hwServiceCtx{globalCtx, serviceCtx, TRTLogger::NullLogger()};

    NCombinators::TCentaurMainScreenContinueHandle{}.Do(hwServiceCtx);
    const auto renderDatas = hwServiceCtx.GetProtos<NRenderer::TDivRenderData>("render_data");
    const auto scenarioResponse = hwServiceCtx.GetProtoOrThrow<NScenarios::TScenarioRunResponse>("mm_scenario_response");
    const auto usedScenarios = hwServiceCtx.GetProtoOrThrow<NCombinators::TCombinatorUsedScenarios>("combinator_used_scenarios");

    if (GetTestParam("canonize") == "true") {
        TFileOutput renderDataFile{TFile{GetDataPath(canonizedFileName), CreateAlways}};
        renderDataFile << JsonToString(JsonFromProto(scenarioResponse), /* validateUtf8 */ true, /* formatOutput */ true);
        renderDataFile << DELIM << JsonToString(JsonFromProto(usedScenarios), /* validateUtf8 */ true, /* formatOutput */ true);
        for (const auto& rd : renderDatas) {
            renderDataFile << DELIM << JsonToString(JsonFromProto(rd), /* validateUtf8 */ true, /* formatOutput */ true);
        }
    } else {
        TFileInput expectedDataFile{GetDataPath(canonizedFileName)};
        auto expectedData = StringSplitter(expectedDataFile.ReadAll()).SplitByString(DELIM).ToList<TString>();
        UNIT_ASSERT_C(expectedData.size(), "Empty canonized data");
        auto dataLinesIterator = expectedData.begin();
        auto expectedResponse = JsonToProto<NScenarios::TScenarioRunResponse>(JsonFromString(*dataLinesIterator++));
        UNIT_ASSERT_MESSAGES_EQUAL(scenarioResponse, expectedResponse);
        auto expectedUsedScenarios = JsonToProto<NCombinators::TCombinatorUsedScenarios>(JsonFromString(*dataLinesIterator++));
        UNIT_ASSERT_MESSAGES_EQUAL(usedScenarios, expectedUsedScenarios);

        TString expectedRenderData;
        for (const auto& renderData : renderDatas) {
            UNIT_ASSERT_C(dataLinesIterator != expectedData.end(), "Response contains new render_data items");
            expectedRenderData = *dataLinesIterator++;
            auto expectedRenderDataProto = JsonToProto<NRenderer::TDivRenderData>(JsonFromString(expectedRenderData));
            UNIT_ASSERT_MESSAGES_EQUAL(renderData, expectedRenderDataProto);
        }
        UNIT_ASSERT_C(dataLinesIterator == expectedData.end(), "Some render_data items not found in response");
    }
}

void TestProcessWidgetGalleryFrame(THashSet<TString> experimentNames, TString canonizedFileName) {
    TMockGlobalContext globalCtx;

    NAppHost::NService::TTestContext serviceCtx;

    NScenarios::TCombinatorRequest request;
    auto& exps = *request.MutableBaseRequest()->MutableExperiments()->mutable_fields();
    for (const auto& expName : experimentNames) {
        exps[TString{expName}].set_string_value("1");
    }

    auto& collectMainScreenSemanticFrame = *request.MutableInput()->AddSemanticFrames();
    collectMainScreenSemanticFrame.SetName("alice.centaur.collect_main_screen");
    {
        auto& slot = *collectMainScreenSemanticFrame.AddSlots();
        slot.SetName("widget_gallery_position");
        slot.SetType("WidgetPositionValue");
        slot.SetValue("{\"widget_position\":{}}");
        *slot.AddAcceptedTypes() = "WidgetPositionValue";
    }
    auto& collectMainScreenTypedSemanticFrame = *collectMainScreenSemanticFrame.MutableTypedSemanticFrame()->MutableCentaurCollectMainScreenSemanticFrame();
    auto& widgetPosition = *collectMainScreenTypedSemanticFrame.MutableWidgetGalleryPosition()->MutableWidgetPositionValue();
    widgetPosition.SetColumn(2);
    widgetPosition.SetRow(0);

    request.MutableBaseRequest()->MutableClientInfo()->SetDeviceId(DEVICE_ID);
    NMemento::TSurfaceConfig surfaceConfig;
    *surfaceConfig.MutableCentaurWidgetsDeviceConfig() = PrepareDefaultCentaurWidgetsDeviceConfig();
    NMemento::TRespGetAllObjects fullMementoData;
    (*fullMementoData.MutableSurfaceConfigs())[DEVICE_ID] = surfaceConfig;
    serviceCtx.AddProtobufItem(fullMementoData, "full_memento_data");

    serviceCtx.AddProtobufItem(request, "combinator_request_apphost_type");

    PutMusicScenarioResponses(serviceCtx);
    PutIoTUserInfo(serviceCtx);
    PutWeatherScenarioResponses(serviceCtx);
    PutNewsScenarioResponses(serviceCtx);
    PutVideoCallScenarioResponses(serviceCtx);

    THwServiceContext hwServiceCtx{globalCtx, serviceCtx, TRTLogger::NullLogger()};

    NCombinators::TCentaurMainScreenContinueHandle{}.Do(hwServiceCtx);
    const auto renderDatas = hwServiceCtx.GetProtos<NRenderer::TDivRenderData>("render_data");
    const auto scenarioResponse = hwServiceCtx.GetProtoOrThrow<NScenarios::TScenarioRunResponse>("mm_scenario_response");

    if (GetTestParam("canonize") == "true") {
        TFileOutput renderDataFile{TFile{GetDataPath(canonizedFileName), CreateAlways}};
        renderDataFile << JsonToString(JsonFromProto(scenarioResponse), /* validateUtf8 */ true, /* formatOutput */ true);
        for (const auto& rd : renderDatas) {
            renderDataFile << DELIM << JsonToString(JsonFromProto(rd), /* validateUtf8 */ true, /* formatOutput */ true);
        }
    } else {
        TFileInput expectedDataFile{GetDataPath(canonizedFileName)};
        auto expectedData = StringSplitter(expectedDataFile.ReadAll()).SplitByString(DELIM).ToList<TString>();
        UNIT_ASSERT_C(expectedData.size(), "Empty canonized data");
        auto dataLinesIterator = expectedData.begin();
        auto expectedResponse = JsonToProto<NScenarios::TScenarioRunResponse>(JsonFromString(*dataLinesIterator++));
        UNIT_ASSERT_MESSAGES_EQUAL(scenarioResponse, expectedResponse);

        TString expectedRenderData;
        for (const auto& renderData : renderDatas) {
            UNIT_ASSERT_C(dataLinesIterator != expectedData.end(), "Response contains new render_data items");
            expectedRenderData = *dataLinesIterator++;
            auto expectedRenderDataProto = JsonToProto<NRenderer::TDivRenderData>(JsonFromString(expectedRenderData));
            UNIT_ASSERT_MESSAGES_EQUAL(renderData, expectedRenderDataProto);
        }
        UNIT_ASSERT_C(dataLinesIterator == expectedData.end(), "Some render_data items not found in response");
    }
}

void TestProcessAddWidgetFromGalleryFrame(THashSet<TString> experimentNames, TString canonizedFileName) {
    TMockGlobalContext globalCtx;

    NAppHost::NService::TTestContext serviceCtx;

    NScenarios::TCombinatorRequest request;
    auto& exps = *request.MutableBaseRequest()->MutableExperiments()->mutable_fields();
    for (const auto& expName : experimentNames) {
        exps[TString{expName}].set_string_value("1");
    }

    auto& addWidgetSemanticFrame = *request.MutableInput()->AddSemanticFrames();
    addWidgetSemanticFrame.SetName("alice.centaur.add_widget_from_gallery");
    {
        auto& slot = *addWidgetSemanticFrame.AddSlots();
        slot.SetName("column");
        slot.SetType("num");
        slot.SetValue("2");
        *slot.AddAcceptedTypes() = "num";
    }
    {
        auto& slot = *addWidgetSemanticFrame.AddSlots();
        slot.SetName("row");
        slot.SetType("num");
        slot.SetValue("0");
        *slot.AddAcceptedTypes() = "num";
    }
    {
        auto& slot = *addWidgetSemanticFrame.AddSlots();
        slot.SetName("widget_config_data_slot");
        slot.SetType("WidgetConfigDataValue");
        slot.SetValue("{\"news_widget_data\":{}}");
        *slot.AddAcceptedTypes() = "WidgetConfigDataValue";
    }
    auto& addWidgetTypedSemanticFrame = *addWidgetSemanticFrame.MutableTypedSemanticFrame()->MutableCentaurAddWidgetFromGallerySemanticFrame();
    addWidgetTypedSemanticFrame.MutableColumn()->SetNumValue(2);
    addWidgetTypedSemanticFrame.MutableRow()->SetNumValue(0);
    addWidgetTypedSemanticFrame.MutableWidgetConfigDataSlot()->MutableWidgetConfigDataValue()->SetWidgetType("news");

    request.MutableBaseRequest()->MutableClientInfo()->SetDeviceId(DEVICE_ID);
    NMemento::TSurfaceConfig surfaceConfig;
    *surfaceConfig.MutableCentaurWidgetsDeviceConfig() = PrepareDefaultCentaurWidgetsDeviceConfig();
    NMemento::TRespGetAllObjects fullMementoData;
    (*fullMementoData.MutableSurfaceConfigs())[DEVICE_ID] = surfaceConfig;
    serviceCtx.AddProtobufItem(fullMementoData, "full_memento_data");

    serviceCtx.AddProtobufItem(request, "combinator_request_apphost_type");

    PutMusicScenarioResponses(serviceCtx);
    PutIoTUserInfo(serviceCtx);
    PutWeatherScenarioResponses(serviceCtx);
    PutNewsScenarioResponses(serviceCtx);

    THwServiceContext hwServiceCtx{globalCtx, serviceCtx, TRTLogger::NullLogger()};

    NCombinators::TCentaurMainScreenContinueHandle{}.Do(hwServiceCtx);
    const auto renderDatas = hwServiceCtx.GetProtos<NRenderer::TDivRenderData>("render_data");
    const auto scenarioResponse = hwServiceCtx.GetProtoOrThrow<NScenarios::TScenarioRunResponse>("mm_scenario_response");

    if (GetTestParam("canonize") == "true") {
        TFileOutput renderDataFile{TFile{GetDataPath(canonizedFileName), CreateAlways}};
        renderDataFile << JsonToString(JsonFromProto(scenarioResponse), /* validateUtf8 */ true, /* formatOutput */ true);
        for (const auto& rd : renderDatas) {
            renderDataFile << DELIM << JsonToString(JsonFromProto(rd), /* validateUtf8 */ true, /* formatOutput */ true);
        }
    } else {
        TFileInput expectedDataFile{GetDataPath(canonizedFileName)};
        auto expectedData = StringSplitter(expectedDataFile.ReadAll()).SplitByString(DELIM).ToList<TString>();
        UNIT_ASSERT_C(expectedData.size(), "Empty canonized data");
        auto dataLinesIterator = expectedData.begin();
        auto expectedResponse = JsonToProto<NScenarios::TScenarioRunResponse>(JsonFromString(*dataLinesIterator++));
        UNIT_ASSERT_MESSAGES_EQUAL(scenarioResponse, expectedResponse);

        TString expectedRenderData;
        for (const auto& renderData : renderDatas) {
            UNIT_ASSERT_C(dataLinesIterator != expectedData.end(), "Response contains new render_data items");
            expectedRenderData = *dataLinesIterator++;
            auto expectedRenderDataProto = JsonToProto<NRenderer::TDivRenderData>(JsonFromString(expectedRenderData));
            UNIT_ASSERT_MESSAGES_EQUAL(renderData, expectedRenderDataProto);
        }
        UNIT_ASSERT_C(dataLinesIterator == expectedData.end(), "Some render_data items not found in response");
    }
}
} // namespace

Y_UNIT_TEST_SUITE(CentaurMainScreenCombinatorTests) {
    Y_UNIT_TEST(TestGenerateMainScreenFinalize) {
        TMockGlobalContext globalCtx;

        NAppHost::NService::TTestContext serviceCtx;

        NScenarios::TCombinatorRequest request;
        serviceCtx.AddProtobufItem(request, "combinator_request_apphost_type");

        {
            NScenarios::TScenarioRunResponse response;
            response.MutableFeatures()->SetIsIrrelevant(false);
            serviceCtx.AddProtobufItem(response, "scenario_HollywoodMusic_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        {
            NScenarios::TScenarioRunResponse response;
            response.MutableFeatures()->SetIsIrrelevant(true);
            serviceCtx.AddProtobufItem(response, "scenario_Weather_run_pure_response", NAppHost::EContextItemKind::Input);
        }

        NScenarios::TScenarioRunResponse scenarioResponse;
        scenarioResponse.MutableResponseBody()->MutableLayout()->SetOutputSpeech("hihello");

        serviceCtx.AddProtobufItem(scenarioResponse, "mm_scenario_response");

        NCombinators::TCombinatorUsedScenarios usedScenarios;
        usedScenarios.AddScenarioNames("used_scenario_name");
        serviceCtx.AddProtobufItem(usedScenarios, "combinator_used_scenarios");

        THwServiceContext hwServiceCtx{globalCtx, serviceCtx, TRTLogger::NullLogger()};

        NCombinators::TCentaurMainScreenFinalizeHandle{}.Do(hwServiceCtx);
        const auto response = hwServiceCtx.GetProtoOrThrow<NScenarios::TCombinatorResponse>("combinator_response_apphost_type");
        const auto expectedResponse = JsonToProto<NScenarios::TCombinatorResponse>(JsonFromString(TStringBuf(R"({
            "response": {
                "response_body": {
                    "layout": {
                        "output_speech": "hihello"
                    }
                }
            },
            "used_scenarios": [
                "used_scenario_name"
            ],
            "combinators_analytics_info": {
                "incoming_scenario_infos": {
                    "HollywoodMusic": {
                        "is_irrelevant": false
                    },
                    "Weather": {
                        "is_irrelevant": true
                    }
                },
                "combinator_product_name": "CentaurMainScreen"
            }
        })")));
        UNIT_ASSERT_MESSAGES_EQUAL(response, expectedResponse);
    }

    Y_UNIT_TEST(TestGenerateMainScreenRun) {
        TestGenerateMainScreenRun(THashSet<TString>{}, TString("main_screen_canonized.txt"));
    }

    Y_UNIT_TEST(TestGenerateMainScreenRunIoT) {
        TestGenerateMainScreenRun(THashSet<TString>{}, TString("main_screen_canonized.txt"));
    }

    Y_UNIT_TEST(TestGenerateMainScreenRunServicesTabInScenarioDataExp) {
        TestGenerateMainScreenRun(THashSet<TString>{SERVICES_TAB_SCENARIO_DATA_EXP_FLAG_NAME}, TString("main_screen_with_services_tab_scenario_data_canonized.txt"));
    }

    Y_UNIT_TEST(TestGenerateMainScreenRunSheduledUpdateExp) {
        TestGenerateMainScreenRun(THashSet<TString>{MAIN_SCREEN_SERVER_UPDATE_EXP_FLAG_NAME}, TString("main_screen_sheduled_update_canonized.txt"));
    }

    Y_UNIT_TEST(TestGenerateMainScreenRunNoMusic) {
        TMockGlobalContext globalCtx;

        testing::StrictMock<TMockSensors> sensors;
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{{"name", "combinator_missed_scenarios_per_second"},
                                                          {"combinator_name", "CentaurMainScreen"},
                                                          {"scenario_name", "HollywoodMusic"}}));
        EXPECT_CALL(globalCtx, Sensors()).WillRepeatedly(ReturnRef(sensors));

        NAppHost::NService::TTestContext serviceCtx;
        NScenarios::TCombinatorRequest request;
        
        auto& collectMainScreenSemanticFrame = *request.MutableInput()->AddSemanticFrames();
        collectMainScreenSemanticFrame.SetName("alice.centaur.collect_main_screen");
        collectMainScreenSemanticFrame.MutableTypedSemanticFrame()->MutableCentaurCollectMainScreenSemanticFrame();

        request.MutableBaseRequest()->MutableClientInfo()->SetDeviceId(DEVICE_ID);
        NMemento::TSurfaceConfig surfaceConfig;
        *surfaceConfig.MutableCentaurWidgetsDeviceConfig() = PrepareDefaultCentaurWidgetsDeviceConfig();
        NMemento::TRespGetAllObjects fullMementoData;
        (*fullMementoData.MutableSurfaceConfigs())[DEVICE_ID] = surfaceConfig;
        serviceCtx.AddProtobufItem(fullMementoData, "full_memento_data");

        serviceCtx.AddProtobufItem(request, "combinator_request_apphost_type");
        THwServiceContext hwServiceCtx{globalCtx, serviceCtx, TRTLogger::NullLogger()};
        NCombinators::TCentaurMainScreenContinueHandle{}.Do(hwServiceCtx);
    }

    Y_UNIT_TEST(TestProcessWidgetGalleryFrame) {
        TestProcessWidgetGalleryFrame(THashSet<TString>{}, TString("widget_gallery_canonized.txt"));
    }

    Y_UNIT_TEST(TestProcessWidgetGalleryFrameIoT) {
        TestProcessWidgetGalleryFrame(THashSet<TString>{}, TString("widget_gallery_canonized.txt"));
    }

    Y_UNIT_TEST(TestProcessWidgetGalleryFrameWithVideoCall) {
        TestProcessWidgetGalleryFrame(THashSet<TString>{VIDEO_CALL_WIDGET_EXP_FLAG_NAME}, TString("widget_gallery_canonized_with_video_call.txt"));
    }

    Y_UNIT_TEST(TestProcessAddWidgetFromGalleryFrame) {
        TestProcessAddWidgetFromGalleryFrame(THashSet<TString>{}, TString("add_widget_canonized.txt"));
    }

    Y_UNIT_TEST(TestProcessAddWidgetFromGalleryFrameIot) {
        TestProcessAddWidgetFromGalleryFrame(THashSet<TString>{}, TString("add_widget_canonized.txt"));
    }
}

} // namespace NAlice::NHollywood
