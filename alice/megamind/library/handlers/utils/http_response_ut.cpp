#include <alice/megamind/library/handlers/utils/http_response.h>
#include <alice/megamind/library/handlers/utils/logs_util.h>

#include <alice/megamind/library/analytics/megamind_analytics_info.h>
#include <alice/megamind/library/config/config.h>
#include <alice/megamind/library/models/cards/text_card_model.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/mock_global_context.h>
#include <alice/megamind/library/testing/mock_guid_generator.h>
#include <alice/megamind/library/testing/mock_http_response.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/utils.h>

#include <alice/megamind/protos/common/content_properties.pb.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <alice/library/proto/proto.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/library/unittest/mock_sensors.h>
#include <alice/library/unittest/mock_sensors.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>

using namespace testing;
using namespace NAlice;
using namespace NAlice::NMegamind::NTesting;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NImpl;
using namespace NAlice::NMegamind::NLogsUtil;

namespace {

constexpr auto SPEECHKIT_REQUEST = TStringBuf(R"(
{
    "application": {
        "app_id": "com.yandex.alicekit.demo",
        "app_version": "4.0",
        "client_time": "20190819T152916",
        "device_id": "d34df00d-d696-4ff0-1337-a7a7a9a56174",
        "device_manufacturer": "google",
        "device_model": "Pixel 2 XL",
        "lang": "ru-RU",
        "os_version": "9",
        "platform": "android",
        "timestamp": "1566217756",
        "timezone": "Europe/Moscow",
        "uuid": "d34df00d-d696-4ff0-1337-a7a7a9a56174"
    },
    "header": {
        "prev_req_id": "d34df00d-f92a-4fae-9c7e-a5630d2212f1",
        "request_id": "d34df00d-c135-4227-8cf8-386d7d989237",
        "sequence_number": 670
    },
    "request": {
        "test_ids": [
            111,
            222
        ],
        "additional_options": {
            "oauth_token": "test_token",
            "bass_options": {
                "user_agent": "AliceKit/4.0",
                "client_ip": "127.0.0.1",
                "filtration_level": 0,
                "screen_scale_factor": 3.5,
                "video_gallery_limit": 42,
                "cookies": [
                    "user_id=USERID",
                    "session_id2=SECRET_SESSION_COOKIE"
                ]
            },
            "divkit_version": "2.0.1",
            "expboxes": "156432,0,3;156042,0,91;149760,0,38",
            "permissions": [
                {
                    "granted": true,
                    "name": "location"
                },
                {
                    "granted": false,
                    "name": "read_contacts"
                },
                {
                    "granted": false,
                    "name": "call_phone"
                }
            ],
            "server_time_ms": 1575317078,
            "supported_features": [
                "no_reliable_speakers",
                "no_bluetooth",
                "battery_power_state",
                "cec_available",
                "change_alarm_sound",
                "no_microphone",
                "music_player_allow_shots",
            ],
            "radiostations": [
                "Авторадио",
                "Максимум",
                "Монте-Карло"
            ],
            "icookie": "2253073970645310284"
        },
        "device_state": {
            "sound_level": 0,
            "sound_muted": true,
            "is_tv_plugged_in": true
        },
        "experiments": {
            "find_poi_gallery": "1",
            "iot": "1",
            "personal_assistant.scenarios.music_play": -5.947592122,
            "personal_assistant.scenarios.video_play": -4.624749712
        },
        "event":{
            "biometry_classification":{
                "status":"ok",
                "scores":[
                    {
                        "classname":"adult",
                        "tag":"children",
                        "confidence":0.0285240449
                    },
                    {
                        "classname":"child",
                        "tag":"children",
                        "confidence":0.9714759588
                    },
                    {
                        "classname":"female",
                        "tag":"gender",
                        "confidence":0.1669122279
                    },
                    {
                        "classname":"male",
                        "tag":"gender",
                        "confidence":0.8330878019
                    }
                ]
            },
            "hypothesis_number":1,
            "end_of_utterance":false,
            "biometry_scoring":{
                "scores_with_mode":[
                    {
                        "scores":[
                            {
                                "score":0.6212571859,
                                "user_id":"1110871794"
                            }
                        ],
                        "mode":"default"
                    },
                    {
                        "scores":[
                            {
                                "score":0.6212571859,
                                "user_id":"1110871794"
                            }
                        ],
                        "mode":"high_tnr"
                    },
                    {
                        "scores":[
                            {
                                "score":0.6212571859,
                                "user_id":"1110871794"
                            }
                        ],
                        "mode":"high_tpr"
                    },
                    {
                        "scores":[
                            {
                                "score":0.6212571859,
                                "user_id":"1110871794"
                            }
                        ],
                        "mode":"max_accuracy"
                    }
                ],
                "status":"ok",
                "request_id":"786a51f8-4e3c-4403-be95-d71984fa673b",
                "group_id":"9c073222983871a08d42acce1cc6b2a4"
            },
            "asr_result":[
                {
                    "endOfPhrase":false,
                    "normalized":"продолжи песню.",
                    "confidence":0,
                    "words":[
                        {
                            "value":"продолжи",
                            "confidence":1
                        },
                        {
                            "value":"песню",
                            "confidence":1
                        }
                    ],
                    "utterance":"продолжи песню.",
                    "parentModel":"grapheme"
                }
            ],
            "type":"voice_input"
        },
        "laas_region": {
            "city_id": 213,
            "country_id_by_ip": 225,
            "gsm_operator_if_ip_is_mobile": "mts pjsc",
            "is_anonymous_vpn": false,
            "is_gdpr": false,
            "is_hosting": false,
            "is_mobile": true,
            "is_public_proxy": false,
            "is_serp_trusted_net": false,
            "is_tor": false,
            "is_user_choice": false,
            "is_yandex_net": false,
            "is_yandex_staff": false,
            "latitude": 55.753215,
            "location_accuracy": 15000,
            "location_unixtime": 1566217745,
            "longitude": 37.622504,
            "precision": 2,
            "probable_regions": [],
            "probable_regions_reliability": 1,
            "region_by_ip": 213,
            "region_home": 0,
            "region_id": 213,
            "should_update_cookie": false,
            "suspected_latitude": 55.753215,
            "suspected_location_accuracy": 15000,
            "suspected_location_unixtime": 1566217745,
            "suspected_longitude": 37.622504,
            "suspected_precision": 2,
            "suspected_region_city": 213,
            "suspected_region_id": 213
        },
        "location": {
            "accuracy": 24.21999931,
            "lat": 55.7364953,
            "lon": 37.6404265,
            "recency": 23450
        },
        "raw_personal_data": "{}",
        "reset_session": false,
        "voice_session": true
    },
    "session": null
}
)");

constexpr auto SPEECHKIT_REQUEST_WITH_PROACTIVITY_LOG = TStringBuf(R"(
{
    "application": {
        "app_id": "com.yandex.alicekit.demo",
        "app_version": "4.0",
        "client_time": "20190819T152916",
        "device_id": "d34df00d-d696-4ff0-1337-a7a7a9a56174",
        "device_manufacturer": "google",
        "device_model": "Pixel 2 XL",
        "lang": "ru-RU",
        "os_version": "9",
        "platform": "android",
        "timestamp": "1566217756",
        "timezone": "Europe/Moscow",
        "uuid": "d34df00d-d696-4ff0-1337-a7a7a9a56174"
    },
    "header": {
        "prev_req_id": "d34df00d-f92a-4fae-9c7e-a5630d2212f1",
        "request_id": "d34df00d-c135-4227-8cf8-386d7d989237",
        "sequence_number": 670
    },
    "request": {
        "test_ids": [
            111,
            222
        ],
        "device_state": {
            "sound_level": 0,
            "sound_muted": true,
            "is_tv_plugged_in": true
        },
        "experiments": {
            "find_poi_gallery": "1",
            "iot": "1",
            "personal_assistant.scenarios.music_play": -5.947592122,
            "personal_assistant.scenarios.video_play": -4.624749712,
            "proactivity_log_storage": "1"
        },
        "event":{
            "hypothesis_number":1,
            "end_of_utterance":false,
            "type":"voice_input"
        },
        "raw_personal_data": "{}",
        "reset_session": false,
        "voice_session": true
    },
    "session": null
)");

constexpr TStringBuf IOT_USER_INFO = R"(
{
    "colors": [
        {
            "name": "Малиновый",
            "id": "raspberry"
        }
    ]
})";

constexpr auto SPEECHKIT_REQUEST_SHOULD_DROP_SESSION = TStringBuf(R"(
{
    "application": {
        "client_time": "20190819T152916",
        "timestamp": "1566217756",
        "timezone": "Europe/Moscow",
    },
    "request": {
        "experiments": {
        },
    },
    "session": "non_empty_session"
}
)");

constexpr auto SPEECHKIT_REQUEST_SHOULD_SAVE_SESSION = TStringBuf(R"(
{
    "application": {
        "client_time": "20190819T152916",
        "timestamp": "1566217756",
        "timezone": "Europe/Moscow",
    },
    "request": {
        "experiments": {
            "dump_sessions_to_logs": "1"
        },
    },
    "session": "non_empty_session"
}
)");

class THttpScenarioVisitorTest : public NUnitTest::TTestBase {
public:
    THttpScenarioVisitorTest()
        : GlobalContext{TMockGlobalContext::EInit::GenericInit}
        , SpeechKitRequest(TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build())
        , SkrJson{NJson::ReadJsonFastTree(SPEECHKIT_REQUEST)}
        , Log(MakeHolder<TNullLogBackend>())
        , Logger(CreateNullLogger())
        , RequestCtx(GlobalContext, TMockInitializer())
    {
    }

    void SetUp() override {
        EXPECT_CALL(ApplyContext, SpeechKitRequest()).WillRepeatedly(Return(SpeechKitRequest));
        EXPECT_CALL(ApplyContext, Logger()).WillRepeatedly(ReturnRef(Logger));
        EXPECT_CALL(ApplyContext, Sensors()).WillRepeatedly(ReturnRef(GlobalContext.ServiceSensors()));


        EXPECT_CALL(GlobalContext, Config()).WillRepeatedly(ReturnRef(Config));
        EXPECT_CALL(GlobalContext, ServiceSensors()).WillRepeatedly(ReturnRef(Sensors));
    }

    void TestSensitiveData();
    void TestRemoveCookies();
    void TestMegamindAnalyticsLogs();
    void TestSensorsOnTestIds();
    void TestDropSessionFromAnalyticsLogs();
    void TestPatchSpeechKitRequest();
    void TestDeleteTokenSpeechKitRequest();
    void TestAnalyticsLogsContextBuilder();
    void TestProactivityLogStorage();

    UNIT_TEST_SUITE(THttpScenarioVisitorTest);
    UNIT_TEST(TestSensitiveData);
    UNIT_TEST(TestRemoveCookies);
    UNIT_TEST(TestMegamindAnalyticsLogs);
    UNIT_TEST(TestSensorsOnTestIds);
    UNIT_TEST(TestDropSessionFromAnalyticsLogs);
    UNIT_TEST(TestPatchSpeechKitRequest);
    UNIT_TEST(TestDeleteTokenSpeechKitRequest);
    UNIT_TEST(TestAnalyticsLogsContextBuilder);
    UNIT_TEST(TestProactivityLogStorage);
    UNIT_TEST_SUITE_END();

private:
    TMockGlobalContext GlobalContext;
    TMockContext ApplyContext;
    TTestSpeechKitRequest SpeechKitRequest;
    NJson::TJsonValue SkrJson;
    TMockGuidGenerator GuidGenerator;
    TLog Log;
    TRTLogger Logger;
    TConfig Config;
    TMockSensors Sensors;
    TMockRequestCtx RequestCtx;
};
UNIT_TEST_SUITE_REGISTRATION(THttpScenarioVisitorTest);

void THttpScenarioVisitorTest::TestAnalyticsLogsContextBuilder() {
    TMockHttpResponse httpResponse{true};

    TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;
    analyticsInfoBuilder.SetWinnerScenarioName("kek_winner_scenario");
    TQualityStorage qualityStorage;
    qualityStorage.SetFactorStorage("kek_factor_storage");

    TProactivityLogStorage proactivityLogStorage;
    proactivityLogStorage.MutableAnalytics()->SetInfo("kek_proactivity_info");

    TAnalyticsLogContextBuilder logContextBuilder;
    THttpResponseVisitor visitor(RequestCtx, ApplyContext, httpResponse, analyticsInfoBuilder,
                                 qualityStorage, proactivityLogStorage, logContextBuilder);
    TScenarioResponse response(
        /* scenarioName= */ "test-scenario", /* scenarioSemanticFrames= */ {}, /* scenarioAcceptsAnyUtterance= */ true);
    auto& builder = response.ForceBuilder(SpeechKitRequest, CreateRequestFromSkr(SpeechKitRequest), GuidGenerator);
    builder.SetOutputSpeech("kek_output_speech");
    visitor(response);
    UNIT_ASSERT_MESSAGES_EQUAL(logContextBuilder.BuildProto(), ParseProtoText<NMegamindAppHost::TAnalyticsLogsContext>(R"(
SpeechKitResponse {
  Header {
    RequestId: "d34df00d-c135-4227-8cf8-386d7d989237"
    ResponseId: ""
    SequenceNumber: 670
    DialogId: ""
    RefMessageId: ""
    SessionId: ""
  }
  VoiceResponse {
    OutputSpeech {
      Type: "simple"
      Text: "kek_output_speech"
    }
    ShouldListen: false
  }
  Response {
    QualityStorage {
      FactorStorage: "kek_factor_storage"
    }
  }
}
AnalyticsInfo {
  WinnerScenario {
    Name: "kek_winner_scenario"
  }
}
QualityStorage {
  FactorStorage: "kek_factor_storage"
}
ProactivityLogStorage {
  Analytics {
    Info: "kek_proactivity_info"
  }
}
HttpCode: 200
)"));
}

void THttpScenarioVisitorTest::TestProactivityLogStorage() {
    TMockHttpResponse httpResponse{true};

    TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;
    analyticsInfoBuilder.SetWinnerScenarioName("kek_winner_scenario");
    TQualityStorage qualityStorage;
    qualityStorage.SetFactorStorage("kek_factor_storage");

    TProactivityLogStorage proactivityLogStorage;
    proactivityLogStorage.MutableAnalytics()->SetInfo("kek_proactivity_info");

    TAnalyticsLogContextBuilder logContextBuilder;
    THttpResponseVisitor visitor(RequestCtx, ApplyContext, httpResponse, analyticsInfoBuilder,
                                 qualityStorage, proactivityLogStorage, logContextBuilder);
    TScenarioResponse response(
        /* scenarioName= */ "test-scenario", /* scenarioSemanticFrames= */ {}, /* scenarioAcceptsAnyUtterance= */ true);
    TTestSpeechKitRequest skr(TSpeechKitRequestBuilder{SPEECHKIT_REQUEST_WITH_PROACTIVITY_LOG}.Build());
    EXPECT_CALL(ApplyContext, SpeechKitRequest()).WillRepeatedly(Return(skr));
    auto& builder = response.ForceBuilder(skr, CreateRequestFromSkr(skr), GuidGenerator);
    builder.SetOutputSpeech("kek_output_speech");
    visitor(response);
    UNIT_ASSERT_MESSAGES_EQUAL(logContextBuilder.BuildProto(), ParseProtoText<NMegamindAppHost::TAnalyticsLogsContext>(R"(
SpeechKitResponse {
  Header {
    RequestId: "d34df00d-c135-4227-8cf8-386d7d989237"
    ResponseId: ""
    SequenceNumber: 670
    DialogId: ""
    RefMessageId: ""
    SessionId: ""
  }
  VoiceResponse {
    OutputSpeech {
      Type: "simple"
      Text: "kek_output_speech"
    }
    ShouldListen: false
  }
  Response {
    QualityStorage {
      FactorStorage: "kek_factor_storage"
    }
  }
  ProactivityLogStorage {
    Analytics {
      Info: "kek_proactivity_info"
    }
  }
}
AnalyticsInfo {
  WinnerScenario {
    Name: "kek_winner_scenario"
  }
}
QualityStorage {
  FactorStorage: "kek_factor_storage"
}
ProactivityLogStorage {
  Analytics {
    Info: "kek_proactivity_info"
  }
}
HttpCode: 200
)"));
}

void THttpScenarioVisitorTest::TestSensitiveData() {
    const TString& message = "___TOP SECRET___";
    const TString& sessionMessage = "___TOP SECRET SESSION___";

    NRTLog::TClient rtLogClient("/dev/null", "null");

    TStringStream megamindAnalyticsLogStream;
    TStringStream megamindProactivityStream;
    TLog megamindAnalyticsLog(MakeHolder<TStreamLogBackend>(&megamindAnalyticsLogStream));
    TLog megamindProactivityLog(MakeHolder<TStreamLogBackend>(&megamindProactivityStream));
    TStringStream outputLogStream;
    TLog outputLog(MakeHolder<TStreamLogBackend>(&outputLogStream));
    TFakeThreadPool loggingThread;
    TFakeThreadPool serializers;
    TRTLogger logger{loggingThread, serializers, rtLogClient.CreateRequestLogger(), ELogPriority::TLOG_RESOURCES,
                     &outputLog};

    EXPECT_CALL(GlobalContext, MegamindAnalyticsLog()).WillRepeatedly(ReturnRef(megamindAnalyticsLog));
    EXPECT_CALL(GlobalContext, MegamindProactivityLog()).WillRepeatedly(ReturnRef(megamindProactivityLog));
    EXPECT_CALL(ApplyContext, Logger()).WillRepeatedly(ReturnRef(logger));

    TMockHttpResponse httpResponse{true};

    TMegamindAnalyticsInfoBuilder analyticsInfoBuilder{};
    TQualityStorage qualityStorage{};
    TProactivityLogStorage proactivityLogStorage{};
    TAnalyticsLogContextBuilder logContextBuilder;
    THttpResponseVisitor visitor(RequestCtx, ApplyContext, httpResponse, analyticsInfoBuilder,
                                 qualityStorage, proactivityLogStorage, logContextBuilder);
    TScenarioResponse response(
            /* scenarioName= */ "test-scenario", /* scenarioSemanticFrames= */ {}, /* scenarioAcceptsAnyUtterance= */ true);
    auto& builder = response.ForceBuilder(SpeechKitRequest, CreateRequestFromSkr(SpeechKitRequest), GuidGenerator);

    TTextCardModel card(/* text= */ message);
    builder.AddCard(card);
    builder.SetContentProperties([] {
        TContentProperties contentProperties{};
        contentProperties.SetContainsSensitiveDataInResponse(true);
        return contentProperties;
    }());
    builder.SetSession("42", sessionMessage);
    visitor(response);
    UNIT_ASSERT_STRING_CONTAINS(httpResponse.Content(), message);
    UNIT_ASSERT_STRING_CONTAINS(httpResponse.Content(), sessionMessage);
    const auto& jsonResponse = JsonFromString(httpResponse.Content());
    UNIT_ASSERT_C(jsonResponse.Has("contains_sensitive_data"), "response doesn't have contains_sensitive_data");
    UNIT_ASSERT_EQUAL(jsonResponse["contains_sensitive_data"].GetBooleanSafe(), true);
    UNIT_ASSERT_C(logContextBuilder.BuildProto().GetHideSensitiveData(), "Analytics should have hideSensitiveData");

    UNIT_ASSERT_C(jsonResponse.Has("content_properties"), "response doesn't have content properties");
    const auto& contentProperties = jsonResponse["content_properties"];
    UNIT_ASSERT_C(contentProperties.Has("contains_sensitive_data_in_response"),
                  "content properties doesn't have response info");
    UNIT_ASSERT(contentProperties["contains_sensitive_data_in_response"].GetBooleanSafe());
    UNIT_ASSERT(!(contentProperties.Has("contains_sensitive_data_in_request") &&
                  contentProperties["contains_sensitive_data_in_request"].GetBooleanSafe()));

    const auto analyticsLogs = logContextBuilder.BuildProto();
    PrepareAndProcessAnalyticsLogs(SpeechKitResponseToJson(analyticsLogs.GetSpeechKitResponse()),
                                   analyticsLogs.GetHideSensitiveData(), SkrJson,
                                   analyticsLogs.GetAnalyticsInfo(), analyticsLogs.GetQualityStorage(), GlobalContext,
                                   analyticsLogs.GetProactivityLogStorage(),
                                    /* dumpSessionsToAnalyticsLogs= */ false);

    // Check new format logs
    const auto& megamindAnalyticsLogStr = megamindAnalyticsLogStream.Str();
    UNIT_ASSERT_C(!megamindAnalyticsLogStr.Contains(message),
                  "megamindAnalyticsLog should not contain \"" << message << "\" in response");
    UNIT_ASSERT_C(!megamindAnalyticsLogStr.Contains(sessionMessage),
                  "megamindAnalyticsLog should not contain \"" << sessionMessage << "\" in response");
    UNIT_ASSERT(megamindAnalyticsLogStr.Contains(R"("response":null)"));
    UNIT_ASSERT(megamindAnalyticsLogStr.Contains(R"("contains_sensitive_data":true)"));

    UNIT_ASSERT_C(!outputLogStream.Str().Contains(message),
            "Log should not contain \"" << message << "\" in response");
    UNIT_ASSERT_C(!outputLogStream.Str().Contains(sessionMessage),
            "Log should not contain \"" << sessionMessage << "\" in response");
}

void THttpScenarioVisitorTest::TestRemoveCookies() {
    NRTLog::TClient rtLogClient("/dev/null", "null");

    TStringStream megamindAnalyticsLogStream;
    TStringStream megamindProactivityStream;
    TLog megamindAnalyticsLog(MakeHolder<TStreamLogBackend>(&megamindAnalyticsLogStream));
    TLog megamindProactivityLog(MakeHolder<TStreamLogBackend>(&megamindProactivityStream));
    TStringStream outputLogStream;
    TLog outputLog(MakeHolder<TStreamLogBackend>(&outputLogStream));
    TFakeThreadPool loggingThread;
    TFakeThreadPool serializers;
    TRTLogger logger{loggingThread, serializers, rtLogClient.CreateRequestLogger(), ELogPriority::TLOG_RESOURCES,
                     &outputLog};

    EXPECT_CALL(GlobalContext, MegamindAnalyticsLog()).WillRepeatedly(ReturnRef(megamindAnalyticsLog));
    EXPECT_CALL(GlobalContext, MegamindProactivityLog()).WillRepeatedly(ReturnRef(megamindProactivityLog));
    EXPECT_CALL(ApplyContext, Logger()).WillRepeatedly(ReturnRef(logger));

    TMockHttpResponse httpResponse{true};

    TMegamindAnalyticsInfoBuilder analyticsInfoBuilder{};
    TQualityStorage qualityStorage{};
    TProactivityLogStorage proactivityLogStorage{};
    TAnalyticsLogContextBuilder logContextBuilder;
    THttpResponseVisitor visitor(RequestCtx, ApplyContext, httpResponse, analyticsInfoBuilder,
                                 qualityStorage, proactivityLogStorage, logContextBuilder);
    TScenarioResponse response(
        /* scenarioName= */ "test-scenario", /* scenarioSemanticFrames= */ {}, /* scenarioAcceptsAnyUtterance= */ true);
    auto& builder = response.ForceBuilder(SpeechKitRequest, CreateRequestFromSkr(SpeechKitRequest), GuidGenerator);
    TTextCardModel card(/* text= */ "Response");
    builder.AddCard(card);
    visitor(response);

    /* FIXME (ran1s)
    UNIT_ASSERT_C(
        !megamindProactivityStream.Str().Contains("SECRET_SESSION_COOKIE"),
        "MegamindProactivityLog should not contain secrets from cookies"
    );
    */
}

// TODO Only small part of analytics logs is tested. Need more!
void THttpScenarioVisitorTest::TestMegamindAnalyticsLogs() {
    const auto skr = TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent}
                         .SetProtoPatcher([](TSpeechKitInitContext& ctxInit) {
                             ctxInit.Proto->SetIoTUserInfoData("Kh8KCXJhc3BiZXJyeRIS0JzQsNC70LjQvdC+0LLRi9C5");
                         })
                         .Build();
    TIoTUserInfo ioTUserInfo;
    const auto status = google::protobuf::util::JsonStringToMessage(TString{IOT_USER_INFO}, &ioTUserInfo);
    UNIT_ASSERT(status.ok());
    const auto megamindAnalyticsInfo =
        TMegamindAnalyticsInfoBuilder{}.SetIoTUserInfo(ioTUserInfo).BuildProto();
    NJson::TJsonValue skrJson = JsonFromProto(skr.Proto());
    skrJson["request"]["event"] = JsonFromProto(skr.Event());
    const auto alogs = CreateMegamindAnalyticsLogs(GlobalContext.Config(), skrJson,
                                                   /* responseJson= */ {},
                                                   /* containsSensitiveData= */ false,
                                                   megamindAnalyticsInfo,
                                                   /* qualityStorage= */ {},
                                                   /* dumpSessionsToAnalyticsLogs= */ false);

    { // Event
        const auto& event = alogs["request"]["request"]["event"];
        UNIT_ASSERT_C(!event.IsNull(), "event must not be null");
        UNIT_ASSERT_VALUES_EQUAL_C(event["type"].GetString(), "text_input", "event type must be text_input");
        UNIT_ASSERT_C(!event["text"].GetString().Empty(), "event text must not be empty");
    }
    {
        UNIT_ASSERT(!alogs["request"].Has("iot_user_info_data"));
        UNIT_ASSERT_VALUES_EQUAL(alogs["analytics_info"]["iot_user_info"], NJson::ReadJsonFastTree(IOT_USER_INFO));
    }
}

void THttpScenarioVisitorTest::TestDropSessionFromAnalyticsLogs() {
    const auto responseJson = NJson::ReadJsonFastTree(TStringBuf(R"({
        "sessions": {
            "": "non_empty_session",
            "x": "non_empty_session"
        }
    })"));
    {
        const auto alogs = CreateMegamindAnalyticsLogs(GlobalContext.Config(), NJson::ReadJsonFastTree(SPEECHKIT_REQUEST_SHOULD_DROP_SESSION),
                                                       responseJson,
                                                       /* containsSensitiveData= */ false,
                                                       /* analyticsInfo= */ {},
                                                       /* qualityStorage= */ {},
                                                       /* dumpSessionsToAnalyticsLogs= */ false);
        UNIT_ASSERT(!alogs["request"].Has("session"));
        UNIT_ASSERT(!alogs["response"].Has("sessions"));
    }
    {
        const auto alogs = CreateMegamindAnalyticsLogs(GlobalContext.Config(), NJson::ReadJsonFastTree(SPEECHKIT_REQUEST_SHOULD_SAVE_SESSION),
                                                       responseJson,
                                                       /* containsSensitiveData= */ false,
                                                       /* analyticsInfo= */ {},
                                                       /* qualityStorage= */ {},
                                                       /* dumpSessionsToAnalyticsLogs= */ true);
        UNIT_ASSERT(alogs["request"].Has("session"));
        UNIT_ASSERT(alogs["response"].Has("sessions"));
    }
}

void THttpScenarioVisitorTest::TestSensorsOnTestIds() {
    NRTLog::TClient rtLogClient("/dev/null", "null");

    TStringStream megamindAnalyticsLogStream;
    TStringStream megamindProactivityStream;
    TLog megamindAnalyticsLog(MakeHolder<TStreamLogBackend>(&megamindAnalyticsLogStream));
    TLog megamindProactivityLog(MakeHolder<TStreamLogBackend>(&megamindProactivityStream));
    TStringStream outputLogStream;
    TLog outputLog(MakeHolder<TStreamLogBackend>(&outputLogStream));
    TFakeThreadPool loggingThread;
    TFakeThreadPool serializers;
    TRTLogger logger{loggingThread, serializers, rtLogClient.CreateRequestLogger(), ELogPriority::TLOG_RESOURCES,
                     &outputLog};


    EXPECT_CALL(GlobalContext, MegamindAnalyticsLog()).WillRepeatedly(ReturnRef(megamindAnalyticsLog));
    EXPECT_CALL(GlobalContext, MegamindProactivityLog()).WillRepeatedly(ReturnRef(megamindProactivityLog));
    EXPECT_CALL(ApplyContext, Logger()).WillRepeatedly(ReturnRef(logger));

    TFakeSensors sensors;
    EXPECT_CALL(GlobalContext, ServiceSensors()).WillRepeatedly(ReturnRef(sensors));
    EXPECT_CALL(ApplyContext, Sensors()).WillRepeatedly(ReturnRef(sensors));

    TMockHttpResponse httpResponse{true};

    TMegamindAnalyticsInfoBuilder analyticsInfoBuilder{};
    TQualityStorage qualityStorage{};
    TProactivityLogStorage proactivityLogStorage{};
    TAnalyticsLogContextBuilder logContextBuilder;
    THttpResponseVisitor visitor(RequestCtx, ApplyContext, httpResponse, analyticsInfoBuilder,
                                 qualityStorage, proactivityLogStorage, logContextBuilder);
    TScenarioResponse response(
        /* scenarioName= */ "test-scenario", /* scenarioSemanticFrames= */ {}, /* scenarioAcceptsAnyUtterance= */ true);
    response.SetHttpCode(HttpCodes::HTTP_INTERNAL_SERVER_ERROR);
    auto& builder = response.ForceBuilder(SpeechKitRequest, CreateRequestFromSkr(SpeechKitRequest), GuidGenerator);
    TTextCardModel card(/* text= */ "Response");
    builder.AddCard(card);
    visitor(response);

    const auto testId222 = sensors.RateCounter(NMonitoring::TLabels{{"http_code", "500"}, {"test_id", "222"}, {"name", "test_ids.errors_per_second"}, {"error_type", "http_error"}});
    UNIT_ASSERT(testId222);
    UNIT_ASSERT_VALUES_EQUAL(*testId222, 1);

    const auto testId111 = sensors.RateCounter(NMonitoring::TLabels{{"http_code", "500"}, {"test_id", "111"}, {"name", "test_ids.errors_per_second"}, {"error_type", "http_error"}});
    UNIT_ASSERT(testId111);
    UNIT_ASSERT_VALUES_EQUAL(*testId111, 1);


    /* FIXME (ran1s)
    UNIT_ASSERT_C(!megamindAnalyticsLogStream.Str().Contains("SECRET_SESSION_COOKIE"),
                  "MegamindAnalyticsLog should not contain secrets from cookies");
     */
}

void THttpScenarioVisitorTest::TestPatchSpeechKitRequest() {
    TSpeechKitRequestProto speechKitRequest;
    NJson::TJsonValue skrJson;
    skrJson["iot_user_info_data"] = "Kh8KCXJhc3BiZXJyeRIS0JzQsNC70LjQvdC+0LLRi9C5";
    PatchSpeechKitRequest(skrJson);
    UNIT_ASSERT(!skrJson["iot_user_info_data"].IsDefined());

    skrJson["request"]["additional_options"]["oauth_token"] = "test";
    PatchSpeechKitRequest(skrJson);
    UNIT_ASSERT(!skrJson["request"]["additional_options"]["oauth_token"].IsDefined());
}

void THttpScenarioVisitorTest::TestDeleteTokenSpeechKitRequest() {
    const auto alogs = CreateMegamindAnalyticsLogs(GlobalContext.Config(), NJson::ReadJsonFastTree(SPEECHKIT_REQUEST),
                                                          /* responseJson= */ {},
                                                          /* containsSensitiveData= */ false,
                                                          /* analyticsInfo= */ {},
                                                          /* qualityStorage= */ {},
                                                          /* dumpSessionsToAnalyticsLogs= */ false);

    UNIT_ASSERT(!alogs["additional_options"].Has("oauth_token"));
}

} // namespace
