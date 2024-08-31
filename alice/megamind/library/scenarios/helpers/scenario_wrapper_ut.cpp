#include "scenario_wrapper.h"

#include <alice/megamind/library/scenarios/protocol/protocol_scenario.h>

#include <alice/megamind/library/common/defs.h>
#include <alice/megamind/library/factor_storage/factor_storage.h>
#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/library/request/event/server_action_event.h>
#include <alice/megamind/library/request/event/text_input_event.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/fake_guid_generator.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/mock_data_sources.h>
#include <alice/megamind/library/testing/mock_guid_generator.h>
#include <alice/megamind/library/testing/mock_responses.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/test.pb.h>

#include <alice/megamind/protos/common/response_error_message.pb.h>
#include <alice/megamind/protos/property/property.pb.h>
#include <alice/megamind/protos/scenarios/push.pb.h>
#include <alice/megamind/protos/scenarios/stack_engine.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>

#include <alice/library/frame/builder.h>
#include <alice/library/metrics/util.h>
#include <alice/library/proto/protobuf.h>
#include <alice/library/proto/proto.h>
#include <alice/library/response/defs.h>
#include <alice/library/response_similarity/response_similarity.h>
#include <alice/library/unittest/fake_fetcher.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/library/unittest/mock_sensors.h>

#include <alice/protos/data/language/language.pb.h>

#include <google/protobuf/wrappers.pb.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/serialized_enum.h>
#include <util/generic/variant.h>

using namespace NAlice;
using namespace NAlice::NScenarios;
using namespace testing;

namespace {

inline const TString TEST_PREVIOUS_SCENARIO_NAME = "test_previous_scenario_name";

const TString TEST_STACK_ENGINE_PRODUCT_SCENARIO_NAME = "test_stack_engine_product_scenario_name";
const TString TEST_STACK_ENGINE_SESSION_ID = "test_stack_engine_session_id";

constexpr TStringBuf SPEECHKIT_REQUEST_WITH_GET_NEXT_DIRECTIVE_AND_RECOVERY_CALLBACK = TStringBuf(R"(
Response: {
  Directives {
    Type: "server_action"
    Name: "@@mm_stack_engine_get_next"
    Payload {
      fields {
        key: "@recovery_callback"
        value {
          struct_value {
            fields {
              key: "ignore_answer"
              value {
                bool_value: false
              }
            }
            fields {
              key: "is_led_silent"
              value {
                bool_value: false
              }
            }
            fields {
              key: "name"
              value {
                string_value: "_some_recovery_action_"
              }
            }
            fields {
              key: "payload"
              value {
                struct_value {
                  fields {
                    key: "@request_id"
                    value {
                      string_value: "test_request_id"
                    }
                  }
                  fields {
                    key: "@scenario_name"
                    value {
                      string_value: "TestScenario"
                    }
                  }
                }
              }
            }
            fields {
              key: "type"
              value {
                string_value: "server_action"
              }
            }
          }
        }
      }
      fields {
        key: "@request_id"
        value {
          string_value: "test_request_id"
        }
      }
      fields {
        key: "@scenario_name"
        value {
          string_value: "TestScenario"
        }
      }
      fields {
        key: "stack_product_scenario_name"
        value {
          string_value: "test_stack_engine_product_scenario_name"
        }
      }
      fields {
        key: "stack_session_id"
        value {
          string_value: "test_stack_engine_session_id"
        }
      }
    }
    IsLedSilent: true
  }
})");

constexpr TStringBuf SPEECHKIT_REQUEST = TStringBuf(R"(
{
    "header":{
        "prev_req_id":"d34df00d-f92a-4fae-9c7e-a5630d2212f1",
        "request_id":"d34df00d-c135-4227-8cf8-386d7d989237",
        "sequence_number":670
    },
    "request":{
        "additional_options":{
            "permissions":[
                {
                    "name":"location",
                    "granted":true
                },
                {
                    "name":"read_contacts",
                    "granted":false
                },
                {
                    "name":"call_phone",
                    "granted":false
                }
            ],
            "bass_options":{
                "filtration_level":0,
                "client_ip":"128.0.0.1",
                "screen_scale_factor":3.5,
                "user_agent":"AliceKit/4.0"
            },
            "expboxes":"156432,0,3;156042,0,91;149760,0,38",
            "divkit_version":"2.0.1"
        },
        "location":{
            "lat":55.7364953,
            "lon":37.6404265,
            "recency":23450,
            "accuracy":24.21999931
        },
        "experiments":{
            "iot":"1",
            "find_poi_gallery":"1",
            "personal_assistant.scenarios.video_play":-4.624749712,
            "personal_assistant.scenarios.music_play":-5.947592122
        },
        "voice_session":false,
        "event":{
            "name":"",
            "type":"text_input",
            "text":"давай поиграем в города"
        },
        "reset_session":false,
        "laas_region":{
            "suspected_longitude":37.622504,
            "is_yandex_staff":false,
            "country_id_by_ip":225,
            "probable_regions":[

            ],
            "is_hosting":false,
            "region_home":0,
            "region_id":213,
            "latitude":55.753215,
            "location_accuracy":15000,
            "is_serp_trusted_net":false,
            "precision":2,
            "suspected_location_accuracy":15000,
            "suspected_location_unixtime":1566217745,
            "is_yandex_net":false,
            "is_public_proxy":false,
            "is_mobile":true,
            "suspected_region_city":213,
            "is_tor":false,
            "region_by_ip":213,
            "is_user_choice":false,
            "is_gdpr":false,
            "longitude":37.622504,
            "gsm_operator_if_ip_is_mobile":"mts pjsc",
            "probable_regions_reliability":1,
            "is_anonymous_vpn":false,
            "suspected_precision":2,
            "location_unixtime":1566217745,
            "should_update_cookie":false,
            "suspected_region_id":213,
            "city_id":213,
            "suspected_latitude":55.753215
        },
        "device_state":{
            "sound_level":0,
            "sound_muted":true
        }
    },
    "session":null,
    "application":{
        "device_manufacturer":"google",
        "lang":"ru-RU",
        "platform":"android",
        "device_model":"Pixel 2 XL",
        "device_id":"d34df00d-d696-4ff0-1337-a7a7a9a56174",
        "uuid":"d34df00d-d696-4ff0-1337-a7a7a9a56174",
        "app_version":"4.0",
        "os_version":"9",
        "client_time":"20190819T152916",
        "timestamp":"1566217756",
        "timezone":"Europe/Moscow",
        "app_id":"com.yandex.alicekit.demo"
    }
}
)");

constexpr TStringBuf SPEECHKIT_REQUEST_WITH_SERVER_ACTION = TStringBuf(R"(
{
    "header":{
        "prev_req_id":"d34df00d-f92a-4fae-9c7e-a5630d2212f1",
        "request_id":"d34df00d-c135-4227-8cf8-386d7d989237",
        "sequence_number":670
    },
    "request":{
        "event":{
            "name":"some_action",
            "type":"server_action",
            "payload": {},
            "ignore_answer":true
        }
    },
    "session":null,
    "application":{
        "device_manufacturer":"google",
        "lang":"ru-RU",
        "platform":"android",
        "device_model":"Pixel 2 XL",
        "device_id":"d34df00d-d696-4ff0-1337-a7a7a9a56174",
        "uuid":"d34df00d-d696-4ff0-1337-a7a7a9a56174",
        "app_version":"4.0",
        "os_version":"9",
        "client_time":"20190819T152916",
        "timestamp":"1566217756",
        "timezone":"Europe/Moscow",
        "app_id":"com.yandex.alicekit.demo"
    }
}
)");

constexpr TStringBuf MEGAREQUEST2 = TStringBuf(R"(
{
    "application" : {
        "uuid" : "hren-vam!",
        "client_time" : "20190527T213049",
        "device_manufacturer" : "Xiaomi",
        "timestamp" : "1558981849",
        "lang" : "ru-RU",
        "platform" : "android",
        "device_id" : "ne-skagu",
        "timezone" : "Europe/Moscow",
        "device_model" : "Redmi Note 4",
        "app_id" : "ru.yandex.searchplugin.dev",
        "os_version" : "7.1.2",
        "app_version" : "8.20"
    },
    "request": {
        "voice_session": true,
        "event": {
            "type": "text_input"
        }
    },
    "header": {
        "dialog_id": "dialog-id-is-here"
    }
})");

constexpr auto TEST_OLD_SUGGEST_ITEMS = TStringBuf(R"(
{
    "items": [
        {
            "type": "action",
            "title": "my_old_title",
            "directives": [
                {
                    "name": "on_suggest",
                    "payload": {
                        "button_id": "deadbeef",
                        "@request_id": "d34df00d-c135-4227-8cf8-386d7d989237",
                        "caption": "my_old_title",
                        "request_id": "d34df00d-c135-4227-8cf8-386d7d989237",
                        "@scenario_name": "Vins",
                        "scenario_name": "TestScenario"
                    },
                    "ignore_answer": true,
                    "is_led_silent": true,
                    "type": "server_action"
                }
            ]
        }
    ]
}
)");

constexpr auto TEST_SUGGEST_ITEMS = TStringBuf(R"(
{
    "items": [
        {
            "type": "action",
            "title": "🔍 \"SearchButton\"",
            "directives": [
                {
                    "name": "@@mm_semantic_frame",
                    "payload": {
                        "typed_semantic_frame": {
                            "search_semantic_frame": {
                                "query": {
                                    "string_value": "SearchQuery"
                                }
                            }
                        },
                        "analytics": {
                            "product_scenario": "",
                            "origin": "Scenario",
                            "purpose": "search",
                            "origin_info": ""
                        },
                        "utterance": "SearchQuery"
                    },
                    "type": "server_action"
                },
                {
                    "name": "on_suggest",
                    "payload": {
                        "@request_id": "d34df00d-c135-4227-8cf8-386d7d989237",
                        "button_id": "deadbeef",
                        "caption": "SearchButton",
                        "request_id": "d34df00d-c135-4227-8cf8-386d7d989237",
                        "@scenario_name": "Vins",
                        "scenario_name": "TestScenario"
                    },
                    "ignore_answer": true,
                    "is_led_silent": true,
                    "type": "server_action"
                }
            ]
        },
        {
            "type": "action",
            "title": "Suggest",
            "directives": [
                {
                    "name": "on_suggest",
                    "payload": {
                        "button_id": "deadbeef",
                        "@request_id": "d34df00d-c135-4227-8cf8-386d7d989237",
                        "caption": "Suggest",
                        "request_id": "d34df00d-c135-4227-8cf8-386d7d989237",
                        "@scenario_name": "Vins",
                        "scenario_name": "TestScenario"
                    },
                    "is_led_silent": true,
                    "ignore_answer": true,
                    "type": "server_action"
                }
            ]
        }
    ]
}
)");

constexpr auto SCENARIO_RESPONSE_BODY_WITH_ACTION_SPACES = TStringBuf(R"(
{
    "ActionSpaces": {
        "SpaceId_1": {
            "effects": {
                "ActionId_1": {
                    "semantic_frame": {
                        "typed_semantic_frame": {
                            "search_semantic_frame": {}
                        }
                    }
                },
                "ActionId_2": {}
            },
            "nlu_hints": [
                {
                    "action_id": "ActionId_1",
                    "semantic_frame_name": "SF_NAME"
                },
                {},
                { "action_id": "ActionId_2" },
                {
                    "action_id": "ActionId_2",
                    "semantic_frame_name": "SF_NAME_2"
                },
            ]
        }
    },
    "layout": {
        "directives": [
            {
                "go_home_directive": {}
            }
        ]
    }
}
)");

constexpr auto UPDATE_ACTION_SPACES = TStringBuf(R"(
{
    "type": "client_action",
    "name": "update_space_actions",
    "sub_name": "update_space_actions",
    "payload": {
        "SpaceId_1": {
            "SF_NAME": {
                "typed_semantic_frame": {
                    "search_semantic_frame": {}
                },
                "analytics": {
                    "origin": "Undefined",
                    "origin_info": "",
                    "product_scenario": "",
                    "purpose": ""
                }
            }
        }
    }
}
)");

TState CreateStringState(const TString& value) {
    TState state{};
    google::protobuf::StringValue stateValue{};
    stateValue.set_value(value);
    state.MutableState()->PackFrom(stateValue);
    return state;
}

class TMockProtocolScenario : public TConfigBasedAppHostPureProtocolScenario {
public:
    using TConfigBasedAppHostPureProtocolScenario::TConfigBasedAppHostPureProtocolScenario;

    MOCK_METHOD(TStatus, StartRun, (const IContext&,
                                    const TScenarioRunRequest&,
                                    NMegamind::TItemProxyAdapter&), (const, override));
    MOCK_METHOD(TErrorOr<TScenarioRunResponse>, FinishRun, (const IContext&,
                                                            NMegamind::TItemProxyAdapter&), (const, override));
    MOCK_METHOD(TStatus, StartCommit, (const IContext&,
                                       const TScenarioApplyRequest&,
                                       NMegamind::TItemProxyAdapter&), (const, override));
    MOCK_METHOD(TErrorOr<TScenarioCommitResponse>, FinishCommit, (const IContext&,
                                                                  NMegamind::TItemProxyAdapter&), (const, override));
    MOCK_METHOD(TErrorOr<NScenarios::TScenarioContinueResponse>, FinishContinue, (const IContext& ctx,
                                                                                  NMegamind::TItemProxyAdapter&), (const, override));
    MOCK_METHOD(TStatus, StartApply, (const IContext&,
                                      const TScenarioApplyRequest&,
                                      NMegamind::TItemProxyAdapter&), (const, override));
    MOCK_METHOD(TErrorOr<TScenarioApplyResponse>, FinishApply, (const IContext&,
                                                                NMegamind::TItemProxyAdapter&), (const, override));
};

class TFakeRequest : public NTestingHelpers::TFakeRequest {
public:
    explicit TFakeRequest(TString data)
        : NTestingHelpers::TFakeRequest(std::move(data)) {
    }

    NHttpFetcher::THandle::TRef Fetch() override {
        THttpHeaders headers;
        headers.AddHeader(NNetwork::HEADER_CONTENT_TYPE, NContentTypes::APPLICATION_PROTOBUF);
        return MakeIntrusive<NTestingHelpers::TFakeHandle>(HttpCodes::HTTP_OK, Data, headers);
    }
};

TRequest MakeRequestWithTextEvent() {
    return CreateRequest(std::make_unique<TTextInputEvent>(/* utterance= */ Default<TString>()),
                    TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithoutEvent}.Build());
}

} // namespace

namespace NAlice {

class TProtocolScenarioWrapperTest : public NUnitTest::TTestBase {
public:
    TProtocolScenarioWrapperTest()
        : GuidGenerator{/* guid= */ "deadbeef"}
        , SpeechKitRequest(
            TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent)
                .Build())
        , Request(CreateRequest(IEvent::CreateEvent(SpeechKitRequest.Event()), SpeechKitRequest))
        , DefaultConfig(GetDefaultInfraConfig())
        , UserLocation("Europe/Moscow", "ru")
        , DefaultScenarioConfig(GetDefaultScenarioConfig())
        , ClientInfo(TClientInfoProto{})
        , GlobalContext()
        , AppHostServiceContext(GlobalContext)
    {
    }

    void TestActionSpaces();
    void TestOnErrorResponse();
    void TestBuildResponse();
    void TestDivCardCloudUiText(); // FIXME(sparkle, zhigan, g-kostin): remove it after HOLLYWOOD-586 supported in search app
    void TestScenarioVersionMismatch();
    void TestFindContacts();
    void TestBuildResponseApplySimulation();
    void TestBuildResponseDivCardsWithDeeplinks();
    void TestWithoutAnalyticsInfoButWithVersionBuildResponse();
    void TestWithoutAnalyticsInfoAndVersionBuildResponse();
    void TestAnalyticsInfoBuildResponse();
    void TestParentProductScenarioNameRewritesBuildResponse();
    void TestRun();
    void TestNonVoiceSession();
    void TestCallback();
    void TestDeferApply();
    void TestDeferApplyUsersInfo();
    void TestDeferApplyEnrichesResponse();
    void TestState();
    void TestVinsState();
    void TestResponseBuilderAnswer();
    void TestApplyType();
    void TestSensitiveDataInLogsOnRun();
    void TestSensitiveDataInLogsOnApply();
    void TestScenarioSessionPersistence();
    void TestStackEngine();
    void TestForcedShouldListen();
    void TestForcedOutputEmotion();
    void TestTtsPlayPlaceholderOnGetNext();
    void TestSuggests();
    void TestSuggestsWithDifferentSerializerScenarioName();
    void TestIrrelevant();
    void TestDirectiveInVoiceResponse();
    void TestWarmUpApply();
    void TestWarmUpApplyGetNextWithSemanticFrame();
    void TestWarmUpSemanticFrameApply();
    void TestWarmUpPureRequest();
    void TestSemanticFramesInApply();
    void TestMakeFramesForRequest();

    UNIT_TEST_SUITE(TProtocolScenarioWrapperTest);
    UNIT_TEST(TestActionSpaces);
    UNIT_TEST(TestOnErrorResponse);
    UNIT_TEST(TestStackEngine);
    UNIT_TEST(TestForcedShouldListen);
    UNIT_TEST(TestForcedOutputEmotion);
    UNIT_TEST(TestTtsPlayPlaceholderOnGetNext);
    UNIT_TEST(TestSuggests);
    UNIT_TEST(TestSuggestsWithDifferentSerializerScenarioName);
    UNIT_TEST(TestBuildResponse);
    UNIT_TEST(TestDivCardCloudUiText);
    UNIT_TEST(TestScenarioVersionMismatch);
    UNIT_TEST(TestBuildResponseDivCardsWithDeeplinks);
    UNIT_TEST(TestFindContacts);
    UNIT_TEST(TestBuildResponseApplySimulation);
    UNIT_TEST(TestWithoutAnalyticsInfoButWithVersionBuildResponse);
    UNIT_TEST(TestWithoutAnalyticsInfoAndVersionBuildResponse);
    UNIT_TEST(TestAnalyticsInfoBuildResponse);
    UNIT_TEST(TestParentProductScenarioNameRewritesBuildResponse);
    UNIT_TEST(TestRun);
    UNIT_TEST(TestNonVoiceSession);
    UNIT_TEST(TestIrrelevant);
    UNIT_TEST(TestCallback);
    UNIT_TEST(TestDeferApply);
    UNIT_TEST(TestDeferApplyUsersInfo);
    UNIT_TEST(TestDeferApplyEnrichesResponse);
    UNIT_TEST(TestState);
    UNIT_TEST(TestVinsState);
    UNIT_TEST(TestResponseBuilderAnswer);
    UNIT_TEST(TestApplyType);
    UNIT_TEST(TestSensitiveDataInLogsOnRun);
    UNIT_TEST(TestSensitiveDataInLogsOnApply);
    UNIT_TEST(TestScenarioSessionPersistence);
    UNIT_TEST(TestWarmUpApply);
    UNIT_TEST(TestWarmUpApplyGetNextWithSemanticFrame);
    UNIT_TEST(TestWarmUpSemanticFrameApply);
    UNIT_TEST(TestWarmUpPureRequest);
    UNIT_TEST(TestSemanticFramesInApply);
    UNIT_TEST(TestMakeFramesForRequest);
    UNIT_TEST(TestDirectiveInVoiceResponse);
    UNIT_TEST_SUITE_END();

    void SetUp() override {
        InitDefaultMocks();
    }

private:
    TAppHostPureProtocolScenarioWrapper CreateWrapper(TConfigBasedAppHostPureProtocolScenario& scenario, bool restoreAllFromSession = false,
                                               EDeferredApplyMode deferredApplyMode = EDeferredApplyMode::DeferApply,
                                               const IScenarioWrapper::TSemanticFrames& requestFrames = {})
    {
        return TAppHostPureProtocolScenarioWrapper(scenario, Context,
                requestFrames,
                GuidGenerator,
                deferredApplyMode,
                restoreAllFromSession,
                AppHostServiceContext.ItemProxyAdapter());
    }

    TMockProtocolScenario CreateDefaultScenario() {
        return TMockProtocolScenario{DefaultScenarioConfig};
    }

    static TScenarioConfig GetDefaultScenarioConfig() {
        TScenarioConfig config;
        config.SetName("TestScenario");
        config.MutableHandlers()->SetBaseUrl("http://scenario.url.testing/");
        config.AddLanguages(::NAlice::ELang::L_RUS);
        return config;
    }

    static TScenarioInfraConfig GetDefaultInfraConfig() {
        TScenarioInfraConfig config;
        auto& handlersConfig = *config.MutableHandlersConfig();

        TConfig::TSource source;
        source.SetTimeoutMs(1000);
        source.SetMaxAttempts(1);

        *handlersConfig.MutableRun() = source;
        *handlersConfig.MutableCallback() = source;
        *handlersConfig.MutableApply() = source;
        *handlersConfig.MutableCommit() = source;

        return config;
    }

    void InitDefaultMocks() {
        EXPECT_CALL(Context, Session()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(SpeechKitRequest));
        EXPECT_CALL(Context, Logger()).WillRepeatedly(ReturnRef(TRTLogger::StderrLogger()));
        EXPECT_CALL(Context, Sensors()).WillRepeatedly(ReturnRef(Sensors));
        EXPECT_CALL(Context, Responses()).WillRepeatedly(ReturnRef(Responses));
        static const auto request = CreateRequest(IEvent::CreateEvent(SpeechKitRequest.Event()), SpeechKitRequest);

        EXPECT_CALL(Context, ScenarioConfig(_)).WillRepeatedly(ReturnRef(DefaultConfig));

        EXPECT_CALL(Context, StackEngineCore()).WillRepeatedly(ReturnRef(NMegamind::TStackEngineCore::default_instance()));
        EXPECT_CALL(Context, HasExpFlag(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(Context, ExpFlags()).WillRepeatedly(ReturnRef(ExpFlags));

        EXPECT_CALL(Context, MementoData()).WillRepeatedly(ReturnRef(MementoData));
        EXPECT_CALL(Context, ClientInfo()).WillRepeatedly(ReturnRef(ClientInfo));

        TClientFeatures clientFeatures(TClientInfoProto{}, /* flags= */ {});
        EXPECT_CALL(Context, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));

        EXPECT_CALL(Context, Session()).WillRepeatedly(Return(nullptr));
    }

    NJson::TJsonValue RenderSuggests(TMockProtocolScenario& scenario, TScenarioResponseBody& body,
                                  const TRequest& request) {
        TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
                                   /* scenarioAcceptsAnyUtterance= */ true};
        response.SetResponseBody(body);
        auto wrapper = CreateWrapper(scenario);
        const auto error = wrapper.Finalize(request, Context, response);;
        UNIT_ASSERT_C(!error, *error);
        const auto& builder =
            response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
        auto skResponse = builder.GetSKRProto();
        const auto jsonResponse = SpeechKitResponseToJson(skResponse);
        return std::move(jsonResponse["response"]["suggest"]);
    }

private:
    NiceMock<TMockContext> Context;
    IContext::TExpFlags ExpFlags;
    NMegamind::TFakeGuidGenerator GuidGenerator;
    TMockSensors Sensors;
    TTestSpeechKitRequest SpeechKitRequest;
    TRequest Request;
    TMockResponses Responses;
    TScenarioInfraConfig DefaultConfig;
    TUserLocation UserLocation;
    NMegamind::TMockDataSources DataSources;

    TScenarioConfig DefaultScenarioConfig;
    NMegamind::TMementoData MementoData;
    TClientInfo ClientInfo;
    TMockGlobalContext GlobalContext;
    NMegamind::NTesting::TTestAppHostCtx AppHostServiceContext;
};

UNIT_TEST_SUITE_REGISTRATION(TProtocolScenarioWrapperTest);

void TProtocolScenarioWrapperTest::TestActionSpaces() {
    auto skRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skRequest));

    auto scenario = CreateDefaultScenario();

    TState state{};
    const TScenarioResponseBody body =
        JsonToProto<TScenarioResponseBody>(JsonFromString(SCENARIO_RESPONSE_BODY_WITH_ACTION_SPACES));

    TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
        /* scenarioAcceptsAnyUtterance= */ true};
    auto wrapper = CreateWrapper(scenario);
    const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest);
    const auto error = wrapper.BuildResponse(request, Context, body, state, /* version= */ {}, response);
    UNIT_ASSERT_C(!error, *error);
    const auto& responseBuilder =
        response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
    const auto& directives = responseBuilder.ToProto().GetResponse().GetResponse().GetDirectives();
    UNIT_ASSERT_VALUES_EQUAL(directives.size(), 2);
    const auto expectedDirective = JsonToProto<NSpeechKit::TDirective>(JsonFromString(UPDATE_ACTION_SPACES));
    UNIT_ASSERT_MESSAGES_EQUAL(expectedDirective, directives[0]);
}

void TProtocolScenarioWrapperTest::TestOnErrorResponse() {
    auto scenario = CreateDefaultScenario();
    auto wrapper = CreateWrapper(scenario);

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    TScenarioError error;
    error.SetMessage("There is no spoon");
    auto status = wrapper.OnError(error, response);
    UNIT_ASSERT(!status.Empty());
    UNIT_ASSERT_EQUAL_C(status.Get()->ErrorMsg, "scenario TestScenario failed with \"There is no spoon\".",
                        TStringBuilder{} << "Original was \'" << status.Get()->ErrorMsg << "\'");
}

void TProtocolScenarioWrapperTest::TestSuggests() {
    auto skRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skRequest));
    const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest);

    auto scenario = CreateDefaultScenario();

    TScenarioResponseBody body{};
    auto layout = body.MutableLayout();

    layout->MutableSuggests()->Add()->SetTitle("my_old_title");
    {
        UNIT_ASSERT_VALUES_EQUAL(RenderSuggests(scenario, body, request), NJson::ReadJsonFastTree(TEST_OLD_SUGGEST_ITEMS));
    }

    auto searchBtn = layout->MutableSuggestButtons()->Add()->MutableSearchButton();
    auto simpleBtn = layout->MutableSuggestButtons()->Add()->MutableActionButton();
    searchBtn->SetTitle("SearchButton");
    searchBtn->SetQuery("SearchQuery");
    simpleBtn->SetTitle("Suggest");
    simpleBtn->SetActionId("qq");
    // Use copy from to manually set oneof case
    (*body.MutableFrameActions())["qq"].MutableDirectives()->CopyFrom(TDirectiveList{});
    {
        UNIT_ASSERT_VALUES_EQUAL(RenderSuggests(scenario, body, request), NJson::ReadJsonFastTree(TEST_SUGGEST_ITEMS));
    }
}

void TProtocolScenarioWrapperTest::TestSuggestsWithDifferentSerializerScenarioName() {
    auto skRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skRequest));
    const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest);

    auto scenario = CreateDefaultScenario();

    auto infraConfig = DefaultConfig;
    infraConfig.SetReplaceScenarioNameWithPrevious(true);
    EXPECT_CALL(Context, ScenarioConfig(_)).WillRepeatedly(ReturnRef(infraConfig));

    auto changeScenarioName = [](NJson::TJsonValue&& suggests, const TString& scenarioName) {
        for (auto& item : suggests["items"].GetArraySafe()) {
            for (auto& directive : item["directives"].GetArraySafe()) {
                if (directive["name"].GetString() != "on_suggest")
                    continue;
                UNIT_ASSERT(directive["payload"].GetMap().contains("scenario_name"));
                directive["payload"]["scenario_name"] = scenarioName;
            }
        }
        return std::move(suggests);
    };

    auto state = CreateStringState("test state");
    TScenarioResponseBody body{};
    auto layout = body.MutableLayout();
    layout->MutableSuggests()->Add()->SetTitle("my_old_title");

    const auto session = MakeSessionBuilder()
                                    ->SetPreviousScenarioName(TEST_PREVIOUS_SCENARIO_NAME)
                                    .SetScenarioSession(TEST_PREVIOUS_SCENARIO_NAME, NewScenarioSession(state))
                                    .Build();
    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

    {
        UNIT_ASSERT_VALUES_EQUAL(
            RenderSuggests(scenario, body, request),
            changeScenarioName(NJson::ReadJsonFastTree(TEST_OLD_SUGGEST_ITEMS), TEST_PREVIOUS_SCENARIO_NAME));
    }

    auto searchBtn = layout->MutableSuggestButtons()->Add()->MutableSearchButton();
    auto simpleBtn = layout->MutableSuggestButtons()->Add()->MutableActionButton();
    searchBtn->SetTitle("SearchButton");
    searchBtn->SetQuery("SearchQuery");
    simpleBtn->SetTitle("Suggest");
    simpleBtn->SetActionId("qq");

    (*body.MutableFrameActions())["qq"].MutableDirectives()->CopyFrom(TDirectiveList{});
    {
        UNIT_ASSERT_VALUES_EQUAL(
            RenderSuggests(scenario, body, request),
            changeScenarioName(NJson::ReadJsonFastTree(TEST_SUGGEST_ITEMS), TEST_PREVIOUS_SCENARIO_NAME));
    }

    infraConfig.SetReplaceScenarioNameWithPrevious(false);

    {
        UNIT_ASSERT_VALUES_EQUAL(RenderSuggests(scenario, body, request),
                                 NJson::ReadJsonFastTree(TEST_SUGGEST_ITEMS));
    }

}

void TProtocolScenarioWrapperTest::TestBuildResponse() {
    auto skRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skRequest));
    const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest);

    auto scenario = CreateDefaultScenario();
    auto wrapper = CreateWrapper(scenario);

    constexpr auto VERSION = "1337";
    TAnalyticsInfo expectedAnalyticsInfo;
    expectedAnalyticsInfo.SetVersion(VERSION);
    expectedAnalyticsInfo.MutableScenarioAnalyticsInfo()->AddObjects()->SetId("7331");

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    TState state;
    TScenarioResponseBody body;
    *body.MutableAnalyticsInfo() = expectedAnalyticsInfo.GetScenarioAnalyticsInfo();
    auto layout = body.MutableLayout();

    TResponseErrorMessage errorMessage;
    errorMessage.SetType("error_type");
    errorMessage.SetMessage("error message");
    errorMessage.SetLevel(TResponseErrorMessage::Error);
    *body.MutableResponseErrorMessage() = errorMessage;

    constexpr auto CARD_TEXT = "There is no spoon";
    layout->SetOutputSpeech(CARD_TEXT);
    layout->AddCards()->SetText(CARD_TEXT);
    layout->SetDirectivesExecutionPolicy(EDirectivesExecutionPolicy::AfterSpeech);

    auto stateValue = google::protobuf::StringValue();

    constexpr auto STATE_VALUE = "Brave New State";
    stateValue.set_value(STATE_VALUE);
    body.MutableState()->PackFrom(stateValue);

    const TString originalUtteranceExpected = skRequest.Event().GetText();

    UNIT_ASSERT(!originalUtteranceExpected.Empty());

    const auto error = wrapper.BuildResponse(request, Context, body, state, VERSION, response);

    UNIT_ASSERT_C(!error, *error);
    google::protobuf::StringValue actualStateValue;
    UNIT_ASSERT(state.GetState().UnpackTo(&actualStateValue));
    UNIT_ASSERT_EQUAL(actualStateValue.value(), STATE_VALUE);
    UNIT_ASSERT_C(response.BuilderIfExists(), "BuildResponse should answer with response builder");

    const auto& builderProto = response.BuilderIfExists()->ToProto();
    const auto& skResponse = builderProto.GetResponse().GetResponse();
    const auto& cards = skResponse.GetCards();
    UNIT_ASSERT(!cards.empty());
    UNIT_ASSERT_EQUAL_C(cards[0].GetType(), "simple_text",
                        TStringBuilder{} << "Actual card type is " << cards[0].GetType());
    UNIT_ASSERT_EQUAL_C(cards[0].GetText(), CARD_TEXT,
                        TStringBuilder{} << "Actual card text is " << cards[0].GetText());

    UNIT_ASSERT(skResponse.GetResponseErrorMessage().GetType() == "error_type");
    UNIT_ASSERT(skResponse.GetResponseErrorMessage().GetMessage() == "error message");
    UNIT_ASSERT(skResponse.GetResponseErrorMessage().GetLevel() == TResponseErrorMessage::Error);

    const auto& actualDirectivesExecutionPolicy = skResponse.GetDirectivesExecutionPolicy();
    UNIT_ASSERT_EQUAL_C(actualDirectivesExecutionPolicy, EDirectivesExecutionPolicy::AfterSpeech,
                        "Actual policy is " << EDirectivesExecutionPolicy_Name(actualDirectivesExecutionPolicy));

    const auto analyticsInfo = wrapper.GetAnalyticsInfo().Build();
    UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, analyticsInfo);
}

void TProtocolScenarioWrapperTest::TestDivCardCloudUiText() {
    auto skRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skRequest));
    const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest);

    auto scenario = CreateDefaultScenario();
    auto wrapper = CreateWrapper(scenario);

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    TState state;
    TScenarioResponseBody body;
    auto layout = body.MutableLayout();

    // Add DivCard
    layout->AddCards()->MutableDivCard();

    // Add FillCloudUiDirective
    constexpr auto CARD_TEXT = "There is no spoon";
    layout->AddDirectives()->MutableFillCloudUiDirective()->SetText(CARD_TEXT);

    const auto error = wrapper.BuildResponse(request, Context, body, state, /* version= */ "1337", response);

    UNIT_ASSERT_C(!error, *error);

    const auto& builderProto = response.BuilderIfExists()->ToProto();
    const auto& skResponse = builderProto.GetResponse().GetResponse();
    const auto& cards = skResponse.GetCards();

    // Check that text is changed from "..." to text from FillCloudUiDirective
    UNIT_ASSERT_VALUES_EQUAL(cards.size(), 1);
    const auto& card = cards[0];
    UNIT_ASSERT_VALUES_EQUAL_C(card.GetType(), "div_card",
                               TStringBuilder{} << "Actual card type is " << card.GetType());
    UNIT_ASSERT_VALUES_EQUAL_C(card.GetText(), CARD_TEXT,
                               TStringBuilder{} << "Actual card text is " << card.GetText());
}

void TProtocolScenarioWrapperTest::TestBuildResponseDivCardsWithDeeplinks() {
    const auto skRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skRequest));
    const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest);

    auto scenario = CreateDefaultScenario();
    TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
        /* scenarioAcceptsAnyUtterance= */ true};


    const TString actionId = "actionId";
    const TString deepLink = TString{NMegamind::MM_DEEPLINK_PLACEHOLDER} + actionId;
    const auto structWithLink = TProtoStructBuilder{}.Set("link", deepLink).Build();

    TFrameAction action;
    action.MutableDirectives()->AddList()->MutableStartImageRecognizerDirective()->SetName("name");

    TScenarioResponseBody responseBody{};
    *responseBody.MutableLayout()->MutableDiv2Templates() = structWithLink;
    *responseBody.MutableLayout()->AddCards()->MutableDiv2CardExtended()->MutableBody() = structWithLink;
    responseBody.MutableFrameActions()->insert({actionId, action});

    TState state{};
    EXPECT_CALL(Context, Logger()).WillRepeatedly(ReturnRef(TRTLogger::StderrLogger()));
    const auto error =
        CreateWrapper(scenario).BuildResponse(request, Context, responseBody, state, /* version= */ {}, response);
    UNIT_ASSERT_C(!error, *error);
    constexpr TStringBuf expectedJsonString = R"({
        "Response": {
            "header": {
                "response_id": "deadbeef",
                "ref_message_id": "",
                "request_id": "d34df00d-c135-4227-8cf8-386d7d989237",
                "sequence_number": 670,
                "session_id": "",
                "dialog_id": ""
            },
            "response": {
                "cards": [
                    {
                        "type": "div2_card",
                        "has_borders": true,
                        "body": {
                            "link": "dialog-action:\/\/?directives=%5B%7B%22name%22%3A%22start_image_recognizer%22%2C%22payload%22%3A%7B%7D%2C%22sub_name%22%3A%22name%22%2C%22type%22%3A%22client_action%22%7D%2C%7B%22ignore_answer%22%3Atrue%2C%22is_led_silent%22%3Atrue%2C%22name%22%3A%22on_suggest%22%2C%22payload%22%3A%7B%22%40request_id%22%3A%22d34df00d-c135-4227-8cf8-386d7d989237%22%2C%22%40scenario_name%22%3A%22Vins%22%2C%22button_id%22%3A%22deadbeef%22%2C%22caption%22%3A%22DeepLink%22%2C%22request_id%22%3A%22d34df00d-c135-4227-8cf8-386d7d989237%22%2C%22scenario_name%22%3A%22TestScenario%22%7D%2C%22type%22%3A%22server_action%22%7D%5D"
                        }
                    }
                ],
                "templates": {
                    "link": "dialog-action:\/\/?directives=%5B%7B%22name%22%3A%22start_image_recognizer%22%2C%22payload%22%3A%7B%7D%2C%22sub_name%22%3A%22name%22%2C%22type%22%3A%22client_action%22%7D%2C%7B%22ignore_answer%22%3Atrue%2C%22is_led_silent%22%3Atrue%2C%22name%22%3A%22on_suggest%22%2C%22payload%22%3A%7B%22%40request_id%22%3A%22d34df00d-c135-4227-8cf8-386d7d989237%22%2C%22%40scenario_name%22%3A%22Vins%22%2C%22button_id%22%3A%22deadbeef%22%2C%22caption%22%3A%22DeepLink%22%2C%22request_id%22%3A%22d34df00d-c135-4227-8cf8-386d7d989237%22%2C%22scenario_name%22%3A%22TestScenario%22%7D%2C%22type%22%3A%22server_action%22%7D%5D"
                },
                "directives_execution_policy": "BeforeSpeech"
            },
            "voice_response": {
                "should_listen": false
            }
        },
        "ScenarioName": "TestScenario",
        "ShouldAddOutputSpeech": false,
        "ProductScenarioName": "",
        "Actions": {
            "actionId": {
                "directives": {
                    "list": [
                        {
                            "start_image_recognizer_directive": {
                                "name": "name"
                            }
                        }
                    ]
                }
            }
        }
    })";
    UNIT_ASSERT_MESSAGES_EQUAL(response.BuilderIfExists()->ToProto(),
                               JsonToProto<TResponseBuilderProto>(JsonFromString(expectedJsonString)));
}

void TProtocolScenarioWrapperTest::TestScenarioVersionMismatch() {
    const auto skRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skRequest));
    const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest);

    auto scenario = CreateDefaultScenario();
    TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
                               /* scenarioAcceptsAnyUtterance= */ true};
    TScenarioResponseBody responseBody{};
    TState state{};

    NScenarios::TScenarioApplyResponse applyResponse;
    *applyResponse.MutableResponseBody() = responseBody;

    NScenarios::TScenarioContinueResponse continueResponse;
    *continueResponse.MutableResponseBody() = responseBody;

    NScenarios::TScenarioCommitResponse commitResponse;
    *commitResponse.MutableSuccess() = NScenarios::TScenarioCommitResponse_TSuccess::default_instance();

    const TString scenarioVersion = "my_scenario_version";
    EXPECT_CALL(Context, Logger()).WillRepeatedly(ReturnRef(TRTLogger::StderrLogger()));

    {
        const auto error =
            CreateWrapper(scenario).OnResponseBody(Context, responseBody, scenarioVersion, response);
        UNIT_ASSERT_C(!error, *error);
    }

    {
        const auto error =
            CreateWrapper(scenario).OnApplyResponse(Context, applyResponse, response);
        UNIT_ASSERT_C(!error, *error);
    }

    {
        const auto error =
            CreateWrapper(scenario).OnContinueResponse(Context, continueResponse, response);
        UNIT_ASSERT_C(!error, *error);
    }

    {
        const auto error =
            CreateWrapper(scenario).OnCommitResponse(Context, commitResponse, response);
        UNIT_ASSERT_C(!error, *error);
    }

    {
        const auto error =
            CreateWrapper(scenario).OnResponseBody(Context, responseBody, scenarioVersion, response);
        UNIT_ASSERT_C(!error, *error);
    }

    EXPECT_CALL(Context, HasExpFlag(EXP_PREFIX_FAIL_ON_SCENARIO_VERSION_MISMATCH + scenario.GetName()))
        .WillRepeatedly(Return(true));
    {
        const auto error =
            CreateWrapper(scenario).OnResponseBody(Context, responseBody, scenarioVersion, response);
        UNIT_ASSERT_C(error, "error expected");
        UNIT_ASSERT_VALUES_EQUAL(error->Type, TError::EType::VersionMismatch);
    }

    {
        const auto error =
            CreateWrapper(scenario).OnApplyResponse(Context, applyResponse, response);
        UNIT_ASSERT_C(error, "error expected");
        UNIT_ASSERT_VALUES_EQUAL(error->Type, TError::EType::VersionMismatch);
    }

    {
        const auto error = CreateWrapper(scenario).OnContinueResponse(Context, continueResponse, response);
        UNIT_ASSERT_C(error, "error expected");
        UNIT_ASSERT_VALUES_EQUAL(error->Type, TError::EType::VersionMismatch);
    }

    {
        const auto error =
            CreateWrapper(scenario).OnCommitResponse(Context, commitResponse, response);
        UNIT_ASSERT_C(error, "error expected");
        UNIT_ASSERT_VALUES_EQUAL(error->Type, TError::EType::VersionMismatch);
    }
}

void TProtocolScenarioWrapperTest::TestFindContacts() {
    const auto skRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skRequest));
    const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest);

    auto scenario = CreateDefaultScenario();

    TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
        /* scenarioAcceptsAnyUtterance= */ true};

    TScenarioResponseBody responseBody{};
    auto& findContacts = *responseBody.MutableLayout()->MutableDirectives()->Add()->MutableFindContactsDirective();
    findContacts.SetName("test_find_contacts");
    findContacts.SetAddAsrContactBook(true);

    TState state{};
    const auto error =
        CreateWrapper(scenario).BuildResponse(request, Context, responseBody, state, /* version= */ {}, response);

    UNIT_ASSERT_C(!error, *error);

    const auto& builderProto = response.BuilderIfExists()->ToProto();
    const auto& directives = builderProto.GetResponse().GetResponse().GetDirectives();

    UNIT_ASSERT_EQUAL(directives.size(), 2);
    UNIT_ASSERT_EQUAL(directives[0].GetName(), "find_contacts");
    UNIT_ASSERT_EQUAL(directives[1].GetName(), "add_contact_book_asr");
}

void TProtocolScenarioWrapperTest::TestBuildResponseApplySimulation() {
    auto skRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skRequest));
    const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest);

    auto scenario = CreateDefaultScenario();
    auto wrapper = CreateWrapper(scenario);

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};

    // Simulation run request
    auto& builder = response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
    builder.AddAnalyticsAttention("test");

    TState state;
    TScenarioResponseBody body;

    auto stateValue = google::protobuf::StringValue();

    constexpr auto STATE_VALUE = "Brave New State";
    stateValue.set_value(STATE_VALUE);
    body.MutableState()->PackFrom(stateValue);

    // init analytics info
    wrapper.GetAnalyticsInfo().CreateScenarioAnalyticsInfoBuilder()->SetIntentName("kek");

    const auto error = wrapper.BuildResponse(request, Context, body, state, "666", response);

    const auto analyticsInfo = wrapper.GetAnalyticsInfo().Build();
    UNIT_ASSERT_VALUES_EQUAL("666", analyticsInfo.GetVersion());
}

void TProtocolScenarioWrapperTest::TestWithoutAnalyticsInfoButWithVersionBuildResponse() {
    auto scenario = CreateDefaultScenario();
    auto wrapper = CreateWrapper(scenario);
    const auto request = MakeRequestWithTextEvent();

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    TState state;
    TScenarioResponseBody body;
    auto layout = body.MutableLayout();

    constexpr auto CARD_TEXT = "There is no spoon";
    layout->SetOutputSpeech(CARD_TEXT);
    layout->AddCards()->SetText(CARD_TEXT);
    auto stateValue = google::protobuf::StringValue();

    constexpr auto VERSION = "73";
    TAnalyticsInfo expectedAnalyticsInfo;
    expectedAnalyticsInfo.SetVersion(VERSION);

    auto status = wrapper.BuildResponse(request, Context, body, state, VERSION, response);
    UNIT_ASSERT(status.Empty());

    const auto analyticsInfo = wrapper.GetAnalyticsInfo().Build();
    UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, analyticsInfo);
}

void TProtocolScenarioWrapperTest::TestWithoutAnalyticsInfoAndVersionBuildResponse() {
    auto scenario = CreateDefaultScenario();
    auto wrapper = CreateWrapper(scenario);
    const auto request = MakeRequestWithTextEvent();

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    TState state;
    TScenarioResponseBody body;
    auto layout = body.MutableLayout();

    constexpr auto CARD_TEXT = "There is no spoon";
    layout->SetOutputSpeech(CARD_TEXT);
    layout->AddCards()->SetText(CARD_TEXT);
    auto stateValue = google::protobuf::StringValue();

    auto status = wrapper.BuildResponse(request, Context, body, state, "", response);
    UNIT_ASSERT(status.Empty());
}

void TProtocolScenarioWrapperTest::TestAnalyticsInfoBuildResponse() {
    auto scenario = CreateDefaultScenario();
    auto wrapper = CreateWrapper(scenario);
    const auto request = MakeRequestWithTextEvent();

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    TState state;
    TScenarioResponseBody body;
    auto layout = body.MutableLayout();

    constexpr auto CARD_TEXT = "There is no spoon";
    layout->SetOutputSpeech(CARD_TEXT);
    layout->AddCards()->SetText(CARD_TEXT);

    auto& frameAction = (*body.MutableFrameActions())["action_id"];
    frameAction.MutableNluHint()->SetFrameName("nlu_frame_name");
    frameAction.MutableFrame()->SetName("frame_name");

    constexpr auto VERSION = "byb";
    TAnalyticsInfo expectedAnalyticsInfo;
    expectedAnalyticsInfo.SetVersion(VERSION);
    auto* action = expectedAnalyticsInfo.MutableScenarioAnalyticsInfo()->AddActions();
    action->SetId("iot.turn.on");
    action->SetName("turn_on");
    action->SetHumanReadable("turn on");
    auto* events = expectedAnalyticsInfo.MutableScenarioAnalyticsInfo()->AddEvents();
    events->SetTimestamp(123);
    auto* reqEvent = events->MutableRequestSourceEvent();
    reqEvent->SetSource("source");
    (*reqEvent->MutableCGI())["param1"] = "value1";
    (*reqEvent->MutableCGI())["param2"] = "value2";
    (*expectedAnalyticsInfo.MutableFrameActions())["action_id"].CopyFrom(frameAction);

    *body.MutableAnalyticsInfo() = expectedAnalyticsInfo.GetScenarioAnalyticsInfo();

    auto status = wrapper.BuildResponse(request, Context, body, state, VERSION, response);
    UNIT_ASSERT(status.Empty());

    const auto analyticsInfo = wrapper.GetAnalyticsInfo().Build();
    UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, analyticsInfo);
}

void TProtocolScenarioWrapperTest::TestParentProductScenarioNameRewritesBuildResponse() {
    auto scenario = CreateDefaultScenario();
    auto wrapper = CreateWrapper(scenario);
    constexpr auto PARENT_PRODUCT_SCENARIO_NAME = "parent_product";
    wrapper.GetMegamindAnalyticsInfo().SetParentProductScenarioName(PARENT_PRODUCT_SCENARIO_NAME);
    const auto request = MakeRequestWithTextEvent();

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    TState state;
    TScenarioResponseBody body;
    auto layout = body.MutableLayout();

    constexpr auto CARD_TEXT = "There is no spoon";
    layout->SetOutputSpeech(CARD_TEXT);
    layout->AddCards()->SetText(CARD_TEXT);
    auto stateValue = google::protobuf::StringValue();

    constexpr auto VERSION = "73";
    TAnalyticsInfo expectedAnalyticsInfo;
    expectedAnalyticsInfo.SetVersion(VERSION);
    expectedAnalyticsInfo.SetParentProductScenarioName(PARENT_PRODUCT_SCENARIO_NAME);

    auto status = wrapper.BuildResponse(request, Context, body, state, VERSION, response);
    UNIT_ASSERT(status.Empty());

    const auto analyticsInfo = wrapper.GetAnalyticsInfo().Build();
    UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, analyticsInfo);
}

void TProtocolScenarioWrapperTest::TestRun() {
    const auto skr = TSpeechKitRequestBuilder{MEGAREQUEST2}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skr));

    auto scenario = CreateDefaultScenario();
    auto wrapper = CreateWrapper(scenario);

    TScenarioRunResponse scenarioResponse;
    const TString psn = "some product scenario name";
    scenarioResponse.MutableResponseBody()->MutableAnalyticsInfo()->SetProductScenarioName(psn);
    auto layout = scenarioResponse.MutableResponseBody()->MutableLayout();
    constexpr auto CARD_TEXT = "There is no spoon";
    layout->SetOutputSpeech(CARD_TEXT);
    layout->AddCards()->SetText(CARD_TEXT);

    EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
    EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(scenarioResponse));

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    TState state;

    auto error = wrapper.Init(Request, Context, DataSources);
    UNIT_ASSERT_C(!error, *error);
    error = wrapper.Ask(Request, Context, response);
    UNIT_ASSERT_C(!error, *error);
    error = wrapper.Finalize(Request, Context, response);
    UNIT_ASSERT_C(!error, *error);

    const auto* builder = response.BuilderIfExists();
    UNIT_ASSERT_C(builder, "Builder is expected in scenario response");

    const auto& builderProto = builder->ToProto();
    const auto& cards = builderProto.GetResponse().GetResponse().GetCards();
    UNIT_ASSERT(!cards.empty());
    UNIT_ASSERT_EQUAL_C(cards[0].GetType(), "simple_text",
            TStringBuilder{} << "Actual type is " << cards[0].GetType());
    UNIT_ASSERT_EQUAL_C(cards[0].GetText(), CARD_TEXT,
                        TStringBuilder{} << "Actual card text is " << cards[0].GetText());
    UNIT_ASSERT_EQUAL(
        builderProto.GetResponse().GetVoiceResponse().GetOutputSpeech().GetText(),
        CARD_TEXT
    );
    UNIT_ASSERT_VALUES_EQUAL(builderProto.GetProductScenarioName(), psn);
}

void TProtocolScenarioWrapperTest::TestNonVoiceSession() {
    const auto skr = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skr));

    auto scenario = CreateDefaultScenario();
    auto wrapper = CreateWrapper(scenario);

    TScenarioRunResponse scenarioResponse;
    auto layout = scenarioResponse.MutableResponseBody()->MutableLayout();
    constexpr auto CARD_TEXT = "There is no spoon";
    layout->SetOutputSpeech(CARD_TEXT);
    layout->AddCards()->SetText(CARD_TEXT);

    EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
    EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(scenarioResponse));

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    TState state;

    auto error = wrapper.Init(Request, Context, DataSources);
    UNIT_ASSERT_C(!error, *error);
    error = wrapper.Ask(Request, Context, response);
    UNIT_ASSERT_C(!error, *error);
    error = wrapper.Finalize(Request, Context, response);
    UNIT_ASSERT_C(!error, *error);

    const auto* builder = response.BuilderIfExists();
    UNIT_ASSERT_C(builder, "Builder is expected in scenario response");

    const auto& builderProto = builder->ToProto();
    UNIT_ASSERT_C(
        builderProto.GetResponse().GetVoiceResponse().GetOutputSpeech().GetText().empty(),
        "Voice response should be empty"
    );
}

void TProtocolScenarioWrapperTest::TestCallback() {
    const auto skr = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST_WITH_SERVER_ACTION}.Build();
    const auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skr));
    auto scenario = CreateDefaultScenario();
    auto wrapper = CreateWrapper(scenario);

    TScenarioRunResponse scenarioResponse;
    auto layout = scenarioResponse.MutableResponseBody()->MutableLayout();
    constexpr auto CARD_TEXT = "There is no spoon";
    layout->SetOutputSpeech(CARD_TEXT);
    layout->AddCards()->SetText(CARD_TEXT);

    EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
    auto error = wrapper.Init(request, Context, DataSources);
    UNIT_ASSERT_C(!error, *error);
}

void TProtocolScenarioWrapperTest::TestDeferApply() {
    auto speechKitRequest = TSpeechKitRequestBuilder{MEGAREQUEST2}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

    TScenarioRunResponse runResponse;
    *runResponse.MutableCommitCandidate() = TScenarioRunResponse::TCommitCandidate();

    auto scenario = CreateDefaultScenario();
    EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
    EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(runResponse));

    auto wrapper = CreateWrapper(scenario);

    {
        const auto error = wrapper.Init(Request, Context, DataSources);
        UNIT_ASSERT_C(!error.Defined(), *error);
    }

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    {
        auto error = wrapper.Ask(Request, Context, response);
        UNIT_ASSERT_C(!error.Defined(), *error);
        error = wrapper.Finalize(Request, Context, response);
        UNIT_ASSERT_C(!error.Defined(), *error);
    }

    {
        NMegamind::TMegamindAnalyticsInfo analyticsInfo;
        TQualityStorage storage;
        const TErrorOr<EApplyResult> result =
            static_cast<IScenarioWrapper&>(wrapper).StartApply(Request, Context, response, analyticsInfo, storage, /* proactivity= */ {});
        UNIT_ASSERT_C(result.IsSuccess(), *result.Error());
    }

    const auto* builder = response.BuilderIfExists();
    UNIT_ASSERT(builder);

    {
        auto skResponse = builder->GetSKRProto();
        const auto response = SpeechKitResponseToJson(skResponse);
        const auto& sessions = response["sessions"];

        UNIT_ASSERT_C(sessions.IsMap(), response);

        const auto map = sessions.GetMap();
        UNIT_ASSERT_VALUES_EQUAL_C(map.size(), 1, response);
        UNIT_ASSERT_VALUES_EQUAL_C(map.begin()->first, "dialog-id-is-here", response);
    }
}

void TProtocolScenarioWrapperTest::TestSemanticFramesInApply() {
    auto speechKitRequest = TSpeechKitRequestBuilder{MEGAREQUEST2}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

    TScenarioRunResponse runResponse;
    *runResponse.MutableApplyArguments() = {};

    TSemanticFrame anyFrame;
    anyFrame.SetName("frame_name");

    auto scenario = CreateDefaultScenario();
    const auto state = CreateStringState("test state");
    const auto defaultSession = MakeSessionBuilder()
        ->SetPreviousScenarioName(scenario.GetName())
        .SetScenarioSession(scenario.GetName(), NewScenarioSession(state))
        .Build();
    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(defaultSession.Get()));

    EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
    EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(runResponse));
    EXPECT_CALL(scenario, StartApply(_, _, _))
        .WillOnce(Invoke([&anyFrame](
            const IContext& /* ctx */,
            const NScenarios::TScenarioApplyRequest& request,
            NMegamind::TItemProxyAdapter& /* itemProxyAdapter */) -> TStatus {
                const auto& frames = request.GetInput().GetSemanticFrames();
                UNIT_ASSERT_EQUAL(frames.size(), 2);
                for (const auto& frame : frames) {
                    UNIT_ASSERT_EQUAL(frame.GetName(), anyFrame.GetName());
                }
                return Success();
            }));

    auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ false, EDeferredApplyMode::DeferApply, {anyFrame, anyFrame});

    {
        const auto error = wrapper.Init(Request, Context, DataSources);
        UNIT_ASSERT_C(!error.Defined(), *error);
    }

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    {
        auto error = wrapper.Ask(Request, Context, response);
        UNIT_ASSERT_C(!error.Defined(), *error);
        error = wrapper.Finalize(Request, Context, response);
        UNIT_ASSERT_C(!error.Defined(), *error);
    }

    {
        NMegamind::TMegamindAnalyticsInfo analyticsInfo;
        TQualityStorage storage;
        const TErrorOr<EApplyResult> result =
            static_cast<IScenarioWrapper&>(wrapper).StartApply(Request, Context, response, analyticsInfo, storage, /* proactivity= */ {});
        UNIT_ASSERT_C(result.IsSuccess(), *result.Error());
    }

    auto builder = response.BuilderIfExists()->ToProto();
    const auto session = DeserializeSession(builder.MutableResponse()->MutableSessions()->at("dialog-id-is-here"));
    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

    auto applyWrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ true);
    applyWrapper.CallApply(Request, Context);
}

void TProtocolScenarioWrapperTest::TestDeferApplyUsersInfo() {
    auto speechKitRequest = TSpeechKitRequestBuilder{MEGAREQUEST2}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

    TScenarioRunResponse runResponse;
    *runResponse.MutableCommitCandidate() = TScenarioRunResponse::TCommitCandidate();

    auto scenario = CreateDefaultScenario();
    EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
    EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(runResponse));

    auto wrapper = CreateWrapper(scenario);
    wrapper.Init(Request, Context, DataSources);
    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    wrapper.Ask(Request, Context, response);
    wrapper.Finalize(Request, Context, response);

    TUserInfo expectedUserInfo;
    expectedUserInfo.MutableScenarioUserInfo()->AddProperties()->SetId("69");

    wrapper.GetUserInfo().CopyFrom(expectedUserInfo.GetScenarioUserInfo());

    NMegamind::TMegamindAnalyticsInfo analyticsInfo;
    TQualityStorage storage;
    static_cast<IScenarioWrapper&>(wrapper).StartApply(
        Request, Context, response, analyticsInfo, storage, /* proactivity= */ {});

    const auto userInfo = std::move(wrapper.GetUserInfo()).Build();
    UNIT_ASSERT(userInfo);
    UNIT_ASSERT_MESSAGES_EQUAL(expectedUserInfo, *userInfo);
}

void TProtocolScenarioWrapperTest::TestDeferApplyEnrichesResponse() {
    auto scenario = CreateDefaultScenario();
    auto speechKitRequest = TSpeechKitRequestBuilder{MEGAREQUEST2}.Build();
    auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
    TResponseBuilderProto storage;
    TResponseBuilder responseBuilder(speechKitRequest, request, scenario.GetName(), storage, NMegamind::TMockGuidGenerator{});

    NMegamind::TMegamindAnalyticsInfo megamindAnalyticsInfo;
    (*megamindAnalyticsInfo.MutableUsersInfo())["test"].MutableScenarioUserInfo()->AddProperties()->SetId("69");

    TQualityStorage qualityStorage;
    auto& predicts = *qualityStorage.MutablePreclassifierPredicts();
    predicts["a"] = 0.0f;
    predicts["b"] = 1.0f;
    qualityStorage.SetFactorStorage("0.0 1.0 2.0");

    TSessionProto::TProtocolInfo protocolInfo;
    protocolInfo.MutableApplyArguments()->PackFrom(google::protobuf::StringValue());

    const auto session = MakeSessionBuilder()
            ->SetPreviousScenarioName(scenario.GetName())
            .SetScenarioResponseBuilder(TMaybe<TResponseBuilderProto>(responseBuilder.ToProto()))
            .SetScenarioSession(scenario.GetName(), NewScenarioSession(TState{}))
            .SetProtocolInfo(protocolInfo)
            .SetMegamindAnalyticsInfo(megamindAnalyticsInfo)
            .SetQualityStorage(qualityStorage)
            .Build();

    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

    TScenarioApplyResponse applyResponse;
    applyResponse.MutableResponseBody()->MutableLayout()->AddCards()->SetText("qq");

    EXPECT_CALL(scenario, StartApply(_, _, _)).WillOnce(Return(Success()));
    EXPECT_CALL(scenario, FinishApply(_, _)).WillOnce(Return(applyResponse));

    auto wrapper = CreateWrapper(scenario, true, EDeferredApplyMode::DeferredCall);
    wrapper.Init(Request, Context, DataSources);
    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};

    // init from walker
    if (auto proto = session->GetScenarioResponseBuilder()) {
        response.ForceBuilder(Context.SpeechKitRequest(), request, GuidGenerator, std::move(*proto));
    }

    auto result = static_cast<IScenarioWrapper&>(wrapper).StartApply(
        Request, Context, response, megamindAnalyticsInfo, qualityStorage, /* proactivity= */ {});
    UNIT_ASSERT_C(result.IsSuccess(), *result.Error());
    result = static_cast<IScenarioWrapper&>(wrapper).FinishApply(Request, Context, response);
    UNIT_ASSERT_C(result.IsSuccess(), *result.Error());

    UNIT_ASSERT_MESSAGES_EQUAL(megamindAnalyticsInfo, wrapper.GetMegamindAnalyticsInfo());
    UNIT_ASSERT_MESSAGES_EQUAL(qualityStorage, wrapper.GetQualityStorage());
}

void TProtocolScenarioWrapperTest::TestState() {
    const auto state = CreateStringState("test state");

    // Assert wrong state is not sent to scenario
    {
        auto scenario = CreateDefaultScenario();
        const auto& scenarioName = "NOT" + scenario.GetName();
        const auto session = MakeSessionBuilder()
            ->SetPreviousScenarioName(scenarioName)
            .SetScenarioSession(scenarioName, NewScenarioSession(state))
            .Build();

        EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

        EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(
                Invoke([](const IContext& /*ctx*/,
                          const TScenarioRunRequest& request,
                          NMegamind::TItemProxyAdapter& /* itemProxyAdapter */) -> TStatus {
                    UNIT_ASSERT_MESSAGES_EQUAL(::google::protobuf::Any{}, request.GetBaseRequest().GetState());
                    UNIT_ASSERT(request.GetBaseRequest().GetIsNewSession());
                    return Success();
                }));

        auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ true);
        auto error = wrapper.Init(Request, Context, DataSources);
        UNIT_ASSERT_C(!error, *error);
    }

    // Now assert own state is correctly sent to scenario

    {
        auto scenario = CreateDefaultScenario();
        const auto session = MakeSessionBuilder()
            ->SetPreviousScenarioName(scenario.GetName())
            .SetScenarioSession(scenario.GetName(), NewScenarioSession(state))
            .Build();

        EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

        EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(
                Invoke([&state](const IContext& /*ctx*/,
                        const TScenarioRunRequest& request,
                        NMegamind::TItemProxyAdapter& /* itemProxyAdapter */) -> TStatus {
                    UNIT_ASSERT_MESSAGES_EQUAL(state.GetState(), request.GetBaseRequest().GetState());
                    UNIT_ASSERT(!request.GetBaseRequest().GetIsNewSession());
                    return Success();
                }));
        auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ true);

        auto error = wrapper.Init(Request, Context, DataSources);
        UNIT_ASSERT_C(!error, *error);
    }

    // Assert vins fallback disabled
    {
        TScenarioConfig config;
        config.SetName(TString{MM_PROTO_VINS_SCENARIO});
        config.MutableHandlers()->SetBaseUrl("http://scenario.url.testing/");
        config.AddLanguages(::NAlice::ELang::L_RUS);
        const auto vins = TString{MM_VINS_SCENARIO};
        auto scenario = TMockProtocolScenario{config};
        const auto session = MakeSessionBuilder()
            ->SetPreviousScenarioName(vins)
            .SetScenarioSession(vins, NewScenarioSession(state))
            .Build();

        EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

        EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(
                Invoke([](const IContext& /*ctx*/,
                        const TScenarioRunRequest& request,
                        NMegamind::TItemProxyAdapter& /* itemProxyAdapter */) -> TStatus {
                    UNIT_ASSERT(request.GetBaseRequest().GetIsNewSession());
                    return Success();
                }));
        auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ true);

        auto error = wrapper.Init(Request, Context, DataSources);
        UNIT_ASSERT_C(!error, *error);
    }
}

void TProtocolScenarioWrapperTest::TestVinsState() {
    const auto vins = TString{MM_VINS_SCENARIO};
    const auto protoVins = TString{MM_PROTO_VINS_SCENARIO};
    const auto vinsState = CreateStringState("VinsState");
    const auto protoVinsState = CreateStringState("ProtoVinsState");
    auto scenario = [&protoVins] {
        auto config = GetDefaultScenarioConfig();
        config.SetName(protoVins);
        return TMockProtocolScenario{std::move(config)};
    }();

    const auto session = MakeSessionBuilder()
        ->SetPreviousScenarioName(vins)
        .SetScenarioSession(vins, NewScenarioSession(vinsState))
        .SetScenarioSession(protoVins, NewScenarioSession(protoVinsState))
        .Build();

    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

    EXPECT_CALL(scenario, StartRun(_, _, _))
        .WillOnce(Invoke([&protoVinsState](const IContext& /* ctx */,
                                           const TScenarioRunRequest& request,
                                           NMegamind::TItemProxyAdapter& /* itemProxyAdapter */) -> TStatus {
            UNIT_ASSERT_MESSAGES_EQUAL(protoVinsState.GetState(), request.GetBaseRequest().GetState());
            return Success();
        }));

    auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ true);
    auto error = wrapper.Init(Request, Context, DataSources);
    UNIT_ASSERT_C(!error, *error);
}

void TProtocolScenarioWrapperTest::TestApplyType() {
    auto scenario = CreateDefaultScenario();
    {
        TSessionProto::TProtocolInfo protocolInfo;
        *protocolInfo.MutableCommitCandidate() = TScenarioRunResponse::TCommitCandidate{};

        NMegamind::TMegamindAnalyticsInfo megamindAnalyticsInfo;
        (*megamindAnalyticsInfo.MutableUsersInfo())["test"].MutableScenarioUserInfo()->AddProperties()->SetId("69");

        const auto session = MakeSessionBuilder()
            ->SetPreviousScenarioName(scenario.GetName())
            .SetScenarioSession(scenario.GetName(), NewScenarioSession(TState{}))
            .SetProtocolInfo(protocolInfo)
            .SetMegamindAnalyticsInfo(megamindAnalyticsInfo)
            .Build();

        EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

        auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ true);

        bool wasCalled = false;

        EXPECT_CALL(scenario, StartCommit(_, _, _)).WillOnce(InvokeWithoutArgs(
                [&wasCalled]() -> TStatus {
                    wasCalled = true;
                    return Success();
                }
        ));

        wrapper.CallApply(Request, Context);

        UNIT_ASSERT_C(wasCalled, "Commit has not been called");
    }

    {
        TSessionProto::TProtocolInfo protocolInfo;
        protocolInfo.MutableApplyArguments()->PackFrom(google::protobuf::StringValue());

        NMegamind::TMegamindAnalyticsInfo megamindAnalyticsInfo;
        (*megamindAnalyticsInfo.MutableUsersInfo())["test"].MutableScenarioUserInfo()->AddProperties()->SetId("69");

        const auto session = MakeSessionBuilder()
            ->SetPreviousScenarioName(scenario.GetName())
            .SetScenarioSession(scenario.GetName(), NewScenarioSession(TState{}))
            .SetProtocolInfo(protocolInfo)
            .SetMegamindAnalyticsInfo(megamindAnalyticsInfo)
            .Build();

        EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

        auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ true);

        bool wasCalled = false;

        EXPECT_CALL(scenario, StartApply(_, _, _)).WillOnce(InvokeWithoutArgs(
                [&wasCalled]() -> TStatus {
                    wasCalled = true;
                    return Success();
                }
        ));

        wrapper.CallApply(Request, Context);

        UNIT_ASSERT_C(wasCalled, "Apply has not been called");
    }
    {
        TSessionProto::TProtocolInfo protocolInfo;
        protocolInfo.MutableContinueArguments()->PackFrom(google::protobuf::StringValue());

        NMegamind::TMegamindAnalyticsInfo megamindAnalyticsInfo;
        (*megamindAnalyticsInfo.MutableUsersInfo())["test"].MutableScenarioUserInfo()->AddProperties()->SetId("42");

        const auto session = MakeSessionBuilder()
            ->SetPreviousScenarioName(scenario.GetName())
            .SetScenarioSession(scenario.GetName(), NewScenarioSession(TState{}))
            .SetProtocolInfo(protocolInfo)
            .SetMegamindAnalyticsInfo(megamindAnalyticsInfo)
            .Build();

        EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

        auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ true);

        bool wasFinishCalled = false;

        EXPECT_CALL(scenario, FinishContinue(_, _))
            .WillOnce(InvokeWithoutArgs([&wasFinishCalled]() -> TScenarioContinueResponse {
                wasFinishCalled = true;
                return TScenarioContinueResponse{};
            }));
        {
            TScenarioResponse response(scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true);
            wrapper.FinishContinue(Request, Context, response);
            UNIT_ASSERT_C(!wasFinishCalled, "FinishContinue was called without Mode flag set!");
        }

        {
            wasFinishCalled = false;

            auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ true);
            wrapper.Mode = TScenario::EApplyMode::Continue;

            TScenarioResponse response(scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true);
            wrapper.FinishContinue(Request, Context, response);
            UNIT_ASSERT_C(wasFinishCalled, "FinishContinue has not been called");
        }
    }
}

void TProtocolScenarioWrapperTest::TestResponseBuilderAnswer() {
    auto scenario = [] {
        auto config = GetDefaultScenarioConfig();
        config.SetName("fake_protocol");
        return TMockProtocolScenario{std::move(config)};
    }();

    const auto skr = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skr));
    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(nullptr));

    TScenarioRunResponse runResponse;
    runResponse.MutableContinueArguments()->PackFrom(google::protobuf::StringValue());
    EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(runResponse));

    const auto request = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);
    TScenarioResponse response(scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true);
    auto wrapper = CreateWrapper(scenario);
    {
        auto error = wrapper.Ask(request, Context, response);
        UNIT_ASSERT_C(!error, *error);
    }
}

void TProtocolScenarioWrapperTest::TestSensitiveDataInLogsOnRun() {
    NRTLog::TClient rtLogClient("/dev/null", "null");

    const TString CARD_TEXT = "___TOP SECRET___";

    TLayout layout;
    layout.SetOutputSpeech(CARD_TEXT);
    layout.AddCards()->SetText(CARD_TEXT);
    layout.SetContainsSensitiveData(true);

    TStringStream outputLogStream;
    TLog outputLog(MakeHolder<TStreamLogBackend>(&outputLogStream));
    TFakeThreadPool loggingThread;
    TFakeThreadPool serializers;
    TRTLogger logger{loggingThread, serializers, rtLogClient.CreateRequestLogger(), ELogPriority::TLOG_RESOURCES,
                     &outputLog};
    EXPECT_CALL(Context, Logger()).WillRepeatedly(ReturnRef(logger));
    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(nullptr));

    auto scenario = CreateDefaultScenario();
    auto wrapper = CreateWrapper(scenario);

    TScenarioRunResponse scenarioResponse;
    scenarioResponse.MutableResponseBody()->MutableLayout()->CopyFrom(layout);
    EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
    EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(scenarioResponse));

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};

    auto error = wrapper.Init(Request, Context, DataSources);
    UNIT_ASSERT_C(!error, *error);
    error = wrapper.Ask(Request, Context, response);
    UNIT_ASSERT_C(!error, *error);
    error = wrapper.Finalize(Request, Context, response);
    UNIT_ASSERT_C(!error, *error);

    const auto* builder = response.BuilderIfExists();
    UNIT_ASSERT_C(builder->ToProto().GetResponse().GetContainsSensitiveData(),
        "There should be ContainsSensitiveData flag");

    UNIT_ASSERT_C(!outputLogStream.Str().Contains(CARD_TEXT),
        "Log should not contain \"" << CARD_TEXT << "\" in RunResponse");
}

void TProtocolScenarioWrapperTest::TestSensitiveDataInLogsOnApply() {
    NRTLog::TClient rtLogClient("/dev/null", "null");

    const TString CARD_TEXT = "___TOP SECRET___";

    TLayout layout;
    layout.SetOutputSpeech(CARD_TEXT);
    layout.AddCards()->SetText(CARD_TEXT);
    layout.SetContainsSensitiveData(true);

    TStringStream outputLogStream;
    TLog outputLog(MakeHolder<TStreamLogBackend>(&outputLogStream));
    TFakeThreadPool loggingThread;
    TFakeThreadPool serializers;
    TRTLogger logger{loggingThread, serializers, rtLogClient.CreateRequestLogger(), ELogPriority::TLOG_RESOURCES,
                     &outputLog};
    EXPECT_CALL(Context, Logger()).WillRepeatedly(ReturnRef(logger));

    auto scenario = CreateDefaultScenario();

    TSessionProto::TProtocolInfo protocolInfo;
    protocolInfo.MutableApplyArguments()->PackFrom(google::protobuf::StringValue());
    const auto session = MakeSessionBuilder()
        ->SetPreviousScenarioName(scenario.GetName())
        .SetScenarioSession(scenario.GetName(), NewScenarioSession(TState{}))
        .SetProtocolInfo(protocolInfo)
        .Build();
    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

    auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ true);

    TScenarioApplyResponse scenarioResponse;
    scenarioResponse.MutableResponseBody()->MutableLayout()->CopyFrom(layout);
    EXPECT_CALL(scenario, StartApply(_, _, _)).WillOnce(Return(Success()));
    EXPECT_CALL(scenario, FinishApply(_, _)).WillOnce(Return(scenarioResponse));

    auto error = wrapper.CallApply(Request, Context);
    UNIT_ASSERT_C(!error, *error);

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    error = wrapper.GetApplyResponse(Request, Context, response);
    UNIT_ASSERT_C(!error, *error);

    UNIT_ASSERT_C(!outputLogStream.Str().Contains(CARD_TEXT),
                  "Log should not contain \"" << CARD_TEXT << "\" in ApplyResponse");
}

void TProtocolScenarioWrapperTest::TestScenarioSessionPersistence() {
    auto scenario = CreateDefaultScenario();


    const auto& sessionA = NewScenarioSession(CreateStringState("A"));
    const auto& sessionB = NewScenarioSession(CreateStringState("B"));

    const auto& anotherScenarioName = scenario.GetName() + "2";

    const auto session = MakeSessionBuilder()
        ->SetPreviousScenarioName(anotherScenarioName)
        .SetScenarioSession(scenario.GetName(), sessionA)
        .SetScenarioSession(anotherScenarioName, sessionB)
        .Build();
    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

    auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ true);

    EXPECT_CALL(scenario, StartRun(_, _, _))
        .WillOnce(Invoke([&sessionA](
            const IContext& /* ctx */,
            const TScenarioRunRequest& request,
            NMegamind::TItemProxyAdapter& /* itemProxyAdapter */) -> TStatus {
                UNIT_ASSERT_MESSAGES_EQUAL(sessionA.GetState().GetState(), request.GetBaseRequest().GetState());
                return Success();
            }));
    auto error = wrapper.Init(Request, Context, DataSources);
    UNIT_ASSERT_C(!error, *error);

    auto skRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest);

    auto env = wrapper.GetApplyEnv(Request, Context);

    {
        const auto& sessionUpdate = wrapper.GetUpdatedSession(
            env,
            request,
            /* builderProto= */ Nothing(),
            NMegamind::TMegamindAnalyticsInfo{},
            TQualityStorage{},
            /* intent= */ Default<TString>(),
            /* proactivity= */ {}
        );

        UNIT_ASSERT(sessionUpdate);
        UNIT_ASSERT_MESSAGES_EQUAL(sessionUpdate->GetScenarioSession(scenario.GetName() + "2"), sessionB);
        UNIT_ASSERT_MESSAGES_EQUAL(sessionUpdate->GetScenarioSession(scenario.GetName()), sessionA);
    }
}

void TProtocolScenarioWrapperTest::TestStackEngine() {
    auto skRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skRequest));

    auto scenario = CreateDefaultScenario();

    TState state{};
    TScenarioResponseBody body{};

    auto& resetAdd = *body.MutableStackEngine()->MutableActions()->Add()->MutableResetAdd();
    resetAdd.MutableEffects()->Add()->MutableCallback()->SetName("sf.A");
    resetAdd.MutableEffects()->Add()->MutableCallback()->SetName("sf.B");
    const auto assertScenarioResetAddItem = [&scenario](const auto& items, const auto idx) {
        {
            const auto& item = items[idx];
            UNIT_ASSERT_VALUES_EQUAL(item.GetScenarioName(), scenario.GetName());
            UNIT_ASSERT_VALUES_EQUAL(item.GetEffect().GetCallback().GetName(), "sf.A");
        }
        {
            const auto& item = items[idx + 1];
            UNIT_ASSERT_VALUES_EQUAL(item.GetScenarioName(), scenario.GetName());
            UNIT_ASSERT_VALUES_EQUAL(item.GetEffect().GetCallback().GetName(), "sf.B");
        }
    };
    constexpr auto assertGetNextDirective = [](const auto& responseBuilder, const TString& stackSessionId,
                                               const TString& productScenarioName) {
        const auto& directives = responseBuilder.ToProto().GetResponse().GetResponse().GetDirectives();
        UNIT_ASSERT_VALUES_EQUAL(directives.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(directives[0].GetName(), MM_STACK_ENGINE_GET_NEXT_CALLBACK_NAME);
        const auto& payloadFields = directives[0].GetPayload().fields();
        UNIT_ASSERT_VALUES_EQUAL(payloadFields.at(MM_STACK_ENGINE_SESSION_ID).string_value(), stackSessionId);
        UNIT_ASSERT_VALUES_EQUAL(payloadFields.at(MM_STACK_ENGINE_PRODUCT_SCENARIO_NAME).string_value(),
                                 productScenarioName);
    };

    /* simpleAdd */ {
        TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
                                   /* scenarioAcceptsAnyUtterance= */ true};
        auto wrapper = CreateWrapper(scenario);
        const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest);
        const auto error = wrapper.BuildResponse(request, Context, body, state, /* version= */ {}, response);
        UNIT_ASSERT_C(!error, *error);
        const auto& responseBuilder =
            response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
        const auto& core = responseBuilder.GetStackEngine()->GetCore();
        UNIT_ASSERT_VALUES_EQUAL(core.ItemsSize(), 2);
        assertScenarioResetAddItem(core.GetItems(), 0);
        assertGetNextDirective(responseBuilder, /* stackSessionId= */ {}, /* productScenarioName= */ {});
    }
    const TString parentReqId = "old_req_id";
    const auto getParentRequestId = [](const TResponseBuilder& responseBuilder) {
        return responseBuilder.ToProto()
                              .GetResponse()
                              .GetHeader()
                              .GetParentRequestId();
    };

    const auto originalCore = [&scenario, &parentReqId]() {
        NMegamind::TStackEngineCore core{};
        {
            auto& item = *core.AddItems();
            item.SetScenarioName("PreviousScenario");
            item.MutableEffect()->MutableCallback()->SetName("sf.C");
        }
        {
            auto& item = *core.AddItems();
            item.SetScenarioName(scenario.GetName());
            item.MutableEffect()->MutableCallback()->SetName("sf.old1");
        }
        {
            auto& item = *core.AddItems();
            item.SetScenarioName(scenario.GetName());
            item.MutableEffect()->MutableCallback()->SetName("sf.old2");
        }
        core.SetSessionId(parentReqId);
        core.SetProductScenarioName("parent_scenario_name");
        return core;
    }();
    EXPECT_CALL(Context, StackEngineCore()).WillRepeatedly(ReturnRef(originalCore));

    /* addWithPopScenario */ {
        TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
                                   /* scenarioAcceptsAnyUtterance= */ true};
        auto wrapper = CreateWrapper(scenario);
        const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest,
                                          /* iotUserInfo= */ Nothing(),
                                          /* requestSource= */ {},
                                          /* semanticFrames= */ {},
                                          /* recognizedActionEffectFrames= */ {},
                                          originalCore);
        const auto error = wrapper.BuildResponse(request, Context, body, state, /* version= */ {}, response);
        UNIT_ASSERT_C(!error, *error);
        const auto& responseBuilder =
            response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
        const auto& core = responseBuilder.GetStackEngine()->GetCore();
        UNIT_ASSERT_VALUES_EQUAL(core.ItemsSize(), 3);
        auto& previousItem = core.GetItems()[0];
        UNIT_ASSERT_VALUES_EQUAL(previousItem.GetScenarioName(), "PreviousScenario");
        UNIT_ASSERT_VALUES_EQUAL(previousItem.GetEffect().GetCallback().GetName(), "sf.C");
        assertScenarioResetAddItem(core.GetItems(), 1);
        assertGetNextDirective(responseBuilder, originalCore.GetSessionId(), originalCore.GetProductScenarioName());
        UNIT_ASSERT_VALUES_EQUAL(getParentRequestId(responseBuilder), parentReqId);
    }

    /* noResetAdd */ {
        TScenarioResponseBody body{};
        TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
            /* scenarioAcceptsAnyUtterance= */ true};
        auto wrapper = CreateWrapper(scenario);
        const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest,
            /* iotUserInfo= */ Nothing(),
            /* requestSource= */ {},
            /* semanticFrames= */ {},
            /* recognizedActionEffectFrames= */ {}, originalCore);
        const auto error = wrapper.BuildResponse(request, Context, body, state, /* version= */ {}, response);

        UNIT_ASSERT_C(!error, *error);
        const auto& responseBuilder =
            response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
        const auto& core = responseBuilder.GetStackEngine()->GetCore();
        UNIT_ASSERT_VALUES_EQUAL(core.ItemsSize(), 3);
        const auto& directives = responseBuilder.ToProto().GetResponse().GetResponse().GetDirectives();
        UNIT_ASSERT_VALUES_EQUAL(directives.size(), 0);
        UNIT_ASSERT_VALUES_EQUAL(getParentRequestId(responseBuilder), TString{});
    }

    /* itemPoppedNoResetAdd */ {
        TScenarioResponseBody body{};
        TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
            /* scenarioAcceptsAnyUtterance= */ true};

        NMegamind::TStackEngine modifiedCore{originalCore};
        modifiedCore.Pop();

        auto wrapper = CreateWrapper(scenario);
        const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest,
            /* iotUserInfo= */ Nothing(),
            /* requestSource= */ {},
            /* semanticFrames= */ {},
            /* recognizedActionEffectFrames= */ {}, modifiedCore.GetCore());
        const auto error = wrapper.BuildResponse(request, Context, body, state, /* version= */ {}, response);

        UNIT_ASSERT_C(!error, *error);
        const auto& responseBuilder =
            response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
        const auto& core = responseBuilder.GetStackEngine()->GetCore();
        UNIT_ASSERT_VALUES_EQUAL(core.ItemsSize(), 2);
        assertGetNextDirective(responseBuilder, modifiedCore.GetSessionId(), modifiedCore.GetProductScenarioName());
        UNIT_ASSERT_VALUES_EQUAL(getParentRequestId(responseBuilder), parentReqId);
    }

    /* simpleNewSession */ {
        TScenarioResponseBody body{};

        *body.MutableStackEngine()->MutableActions()->Add()->MutableNewSession() = {};

        TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
                                   /* scenarioAcceptsAnyUtterance= */ true};
        auto wrapper = CreateWrapper(scenario);
        const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest,
            /* iotUserInfo= */ Nothing(),
            /* requestSource= */ {}, /* semanticFrames= */ {},
            /* recognizedActionEffectFrames= */ {}, originalCore);
        const auto error = wrapper.BuildResponse(request, Context, body, state, /* version= */ {}, response);
        UNIT_ASSERT_C(!error, *error);
        const auto& responseBuilder =
            response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
        const auto& core = responseBuilder.GetStackEngine()->GetCore();
        UNIT_ASSERT_VALUES_EQUAL(core.ItemsSize(), 0);
        UNIT_ASSERT_VALUES_EQUAL(core.GetSessionId(), skRequest.RequestId());
        UNIT_ASSERT_VALUES_EQUAL(getParentRequestId(responseBuilder), TString{});
        UNIT_ASSERT_VALUES_EQUAL(core.GetStackOwner(), scenario.GetName());
    }

    /* newSessionWithResetAdd */ {
        TScenarioResponseBody body{};

        *body.MutableStackEngine()->MutableActions()->Add()->MutableNewSession() = {};
        auto& resetAdd = *body.MutableStackEngine()->MutableActions()->Add()->MutableResetAdd();
        resetAdd.MutableEffects()->Add()->MutableCallback()->SetName("sf.A");
        resetAdd.MutableEffects()->Add()->MutableCallback()->SetName("sf.B");

        TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
                                   /* scenarioAcceptsAnyUtterance= */ true};
        auto wrapper = CreateWrapper(scenario);
        const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest,
            /* iotUserInfo= */ Nothing(),
            /* requestSource= */ {}, /* semanticFrames= */ {},
            /* recognizedActionEffectFrames= */ {}, originalCore);
        const auto error = wrapper.BuildResponse(request, Context, body, state, /* version= */ {}, response);
        UNIT_ASSERT_C(!error, *error);
        const auto& responseBuilder =
            response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
        const auto& core = responseBuilder.GetStackEngine()->GetCore();
        UNIT_ASSERT_VALUES_EQUAL(core.ItemsSize(), 2);
        assertScenarioResetAddItem(core.GetItems(), 0);
        assertGetNextDirective(responseBuilder, skRequest.RequestId(), /* productScenarioName= */ {});
        UNIT_ASSERT_VALUES_EQUAL(getParentRequestId(responseBuilder), skRequest.RequestId());
    }

    /* parentRequestId */ {
        TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
                                   /* scenarioAcceptsAnyUtterance= */ true};
        auto wrapper = CreateWrapper(scenario);

        EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return([]{
            TScenarioRunResponse scenarioResponse;
            auto layout = scenarioResponse.MutableResponseBody()->MutableLayout();
            layout->SetOutputSpeech("some text");
            layout->AddCards()->SetText("some text");
            return scenarioResponse;
        }()));

        const auto request =
            CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest,
                          /* iotUserInfo= */ Nothing(),
                          NScenarios::TScenarioBaseRequest_ERequestSourceType_GetNext, /* semanticFrames= */ {},
                          /* recognizedActionEffectFrames= */ {}, originalCore);
        auto error = wrapper.Ask(request, Context, response);
        UNIT_ASSERT_C(!error, *error);
        error = wrapper.Finalize(request, Context, response);
        UNIT_ASSERT_C(!error, *error);

        const auto& responseBuilder = response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
        UNIT_ASSERT_VALUES_EQUAL(getParentRequestId(responseBuilder), parentReqId);
    }

}

void TProtocolScenarioWrapperTest::TestForcedShouldListen() {
    auto skRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skRequest));
    auto scenario = CreateDefaultScenario();

    TSessionProto::TProtocolInfo protocolInfo;
    protocolInfo.MutableForcedShouldListen()->set_value(false);

    const auto session = MakeSessionBuilder()
        ->SetPreviousScenarioName(scenario.GetName())
        .SetScenarioSession(scenario.GetName(), NewScenarioSession({}))
        .SetProtocolInfo(protocolInfo)
        .Build();
    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

    TState state{};
    TScenarioResponseBody body{};
    body.MutableLayout()->SetShouldListen(true);
    TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
                               /* scenarioAcceptsAnyUtterance= */ true};
    const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest);
    /* defaultShouldListen */ {
        auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ false);
        const auto error = wrapper.BuildResponse(request, Context, body, state, /* version= */ {}, response);
        UNIT_ASSERT_C(!error, *error);
        const auto& responseBuilder =
            response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
        UNIT_ASSERT(responseBuilder.GetShouldListen(/* def= */ false));
    }
    /* forced */ {
        auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ true);
        const auto error = wrapper.BuildResponse(request, Context, body, state, /* version= */ {}, response);
        UNIT_ASSERT_C(!error, *error);
        const auto& responseBuilder =
            response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
        UNIT_ASSERT(!responseBuilder.GetShouldListen(/* def= */ false));
    }
}

void TProtocolScenarioWrapperTest::TestForcedOutputEmotion() {
    auto skRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skRequest));
    auto scenario = CreateDefaultScenario();

    TSessionProto::TProtocolInfo protocolInfo;
    protocolInfo.SetForcedEmotion("evil");

    const auto session = MakeSessionBuilder()
                             ->SetPreviousScenarioName(scenario.GetName())
                             .SetScenarioSession(scenario.GetName(), NewScenarioSession({}))
                             .SetProtocolInfo(protocolInfo)
                             .Build();
    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

    TState state{};
    TScenarioResponseBody body{};
    body.MutableLayout()->SetShouldListen(true);
    TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
                               /* scenarioAcceptsAnyUtterance= */ true};
    const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest);
    auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ true);
    const auto error = wrapper.BuildResponse(request, Context, body, state, /* version= */ {}, response);
    UNIT_ASSERT_C(!error, *error);
    const auto& responseBuilder =
        response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
    const auto jsonResponse = SpeechKitResponseToJson(responseBuilder.GetSKRProto());
    UNIT_ASSERT_VALUES_EQUAL(jsonResponse["voice_response"]["output_emotion"], "evil");
}

void TProtocolScenarioWrapperTest::TestTtsPlayPlaceholderOnGetNext() {
    auto skRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skRequest));
    auto scenario = CreateDefaultScenario();

    TSessionProto::TProtocolInfo protocolInfo;
    protocolInfo.SetRequestSourceType(NScenarios::TScenarioBaseRequest_ERequestSourceType_GetNext);
    protocolInfo.SetChannel(TDirectiveChannel_EDirectiveChannel_Content);

    const auto session = MakeSessionBuilder()
        ->SetPreviousScenarioName(scenario.GetName())
        .SetScenarioSession(scenario.GetName(), NewScenarioSession({}))
        .SetProtocolInfo(protocolInfo)
        .Build();
    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

    TState state{};
    const auto request = CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest);
    auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ true);
    /* addNew */ {
        TScenarioResponseBody body{};
        TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
                                   /* scenarioAcceptsAnyUtterance= */ true};

        const auto error = wrapper.BuildResponse(request, Context, body, state, /* version= */ {}, response);
        UNIT_ASSERT_C(!error, *error);
        const auto& responseBuilder =
            response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
        UNIT_ASSERT(responseBuilder.GetSKRProto().GetResponse().GetDirectives().size() > 0);
        UNIT_ASSERT(responseBuilder.GetSKRProto().GetResponse().GetDirectives()[0].GetName() == "tts_play_placeholder");
        UNIT_ASSERT(responseBuilder.GetSKRProto().GetResponse().GetDirectives()[0].GetPayload().fields().at("channel").string_value() == "Content");
    }
    /* fixExisting */ {
        TScenarioResponseBody body{};
        auto& directive = *body.MutableLayout()->AddDirectives();
        directive.MutableTtsPlayPlaceholderDirective()->SetDirectiveChannel(TDirectiveChannel_EDirectiveChannel_Dialog);
        TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
                                   /* scenarioAcceptsAnyUtterance= */ true};

        const auto error = wrapper.BuildResponse(request, Context, body, state, /* version= */ {}, response);
        UNIT_ASSERT_C(!error, *error);
        const auto& responseBuilder =
            response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
        UNIT_ASSERT(responseBuilder.GetSKRProto().GetResponse().GetDirectives().size() > 0);
        UNIT_ASSERT(responseBuilder.GetSKRProto().GetResponse().GetDirectives()[0].GetName() == "tts_play_placeholder");
        UNIT_ASSERT(responseBuilder.GetSKRProto().GetResponse().GetDirectives()[0].GetPayload().fields().at("channel").string_value() == "Content");
    }
    /* doNotAdd */ {
        TSessionProto::TProtocolInfo protocolInfoNoAdd;
        protocolInfoNoAdd.SetRequestSourceType(NScenarios::TScenarioBaseRequest_ERequestSourceType_GetNext);

        const auto sessionNoAdd = MakeSessionBuilder()
            ->SetPreviousScenarioName(scenario.GetName())
            .SetScenarioSession(scenario.GetName(), NewScenarioSession({}))
            .SetProtocolInfo(protocolInfoNoAdd)
            .Build();
        EXPECT_CALL(Context, Session()).WillRepeatedly(Return(sessionNoAdd.Get()));
        auto wrapperNoAdd = CreateWrapper(scenario, /* restoreAllFromSession= */ true);

        TScenarioResponseBody body{};
        TScenarioResponse response{scenario.GetName(), /* scenarioSemanticFrames= */ {},
                                   /* scenarioAcceptsAnyUtterance= */ true};

        const auto error = wrapperNoAdd.BuildResponse(request, Context, body, state, /* version= */ {}, response);
        UNIT_ASSERT_C(!error, *error);
        const auto& responseBuilder =
            response.ForceBuilder(Context.SpeechKitRequest(), request, NMegamind::TMockGuidGenerator{});
        UNIT_ASSERT(responseBuilder.GetSKRProto().GetResponse().GetDirectives().size() == 0);
    }
}

void TProtocolScenarioWrapperTest::TestIrrelevant() {
    const auto skr = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skr));

    auto scenario = CreateDefaultScenario();
    auto wrapper = CreateWrapper(scenario);

    TScenarioRunResponse scenarioResponse;
    scenarioResponse.MutableResponseBody()->MutableLayout()->SetOutputSpeech("There is no spoon");
    scenarioResponse.MutableFeatures()->SetIsIrrelevant(true);

    EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
    EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(scenarioResponse));

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyInput= */ true};
    TState state;

    auto error = wrapper.Init(Request, Context, DataSources);
    UNIT_ASSERT_C(!error, *error);
    error = wrapper.Ask(Request, Context, response);
    UNIT_ASSERT_C(!error, *error);
    error = wrapper.Finalize(Request, Context, response);
    UNIT_ASSERT_C(!error, *error);

    const auto* builder = response.BuilderIfExists();
    UNIT_ASSERT_C(builder, "Builder is expected in scenario response");

    const auto& builderProto = builder->ToProto();
    UNIT_ASSERT_C(
        builderProto.GetResponse().GetVoiceResponse().GetOutputSpeech().GetText().empty(),
        "Voice response should be empty"
    );
}

void TProtocolScenarioWrapperTest::TestDirectiveInVoiceResponse() {
    const auto skr = TSpeechKitRequestBuilder{MEGAREQUEST2}.Build();
    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(skr));

    auto scenario = CreateDefaultScenario();
    auto wrapper = CreateWrapper(scenario);

    TScenarioRunResponse scenarioResponse;
    auto layout = scenarioResponse.MutableResponseBody()->MutableLayout();
    constexpr auto CARD_TEXT = "There is no spoon";
    layout->SetOutputSpeech(CARD_TEXT);
    layout->AddCards()->SetText(CARD_TEXT);

    NScenarios::TDirective directive;
    NScenarios::TUpdateNotificationSubscriptionDirective* subscribeDirective = directive.MutableUpdateNotificationSubscriptionDirective();
    subscribeDirective->SetSubscriptionId(1);
    *layout->AddDirectives() = std::move(directive);

    auto& serverDirective = *scenarioResponse.MutableResponseBody()->AddServerDirectives();
    serverDirective.MutableSendPushDirective()->SetPushId("test_push");

    EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
    EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(scenarioResponse));

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    TState state;

    auto error = wrapper.Init(Request, Context, DataSources);
    UNIT_ASSERT_C(!error, *error);
    error = wrapper.Ask(Request, Context, response);
    UNIT_ASSERT_C(!error, *error);
    error = wrapper.Finalize(Request, Context, response);
    UNIT_ASSERT_C(!error, *error);

    const auto* builder = response.BuilderIfExists();
    UNIT_ASSERT_C(builder, "Builder is expected in scenario response");

    const auto& builderProto = builder->ToProto();
    const auto& directives = builderProto.GetResponse().GetVoiceResponse().GetDirectives();
    UNIT_ASSERT(directives.size() == 2);
    UNIT_ASSERT_EQUAL_C(directives[0].GetName(), "update_notification_subscription",
            TStringBuilder{} << "Actual name is " << directives[0].GetName());
    UNIT_ASSERT_EQUAL_C(directives[0].GetType(), "uniproxy_action",
            TStringBuilder{} << "Actual type is " << directives[0].GetType());
    UNIT_ASSERT_EQUAL_C(directives[1].GetName(), "send_push_directive",
            TStringBuilder{} << "Actual name is " << directives[1].GetName());
    UNIT_ASSERT_EQUAL_C(directives[1].GetType(), "uniproxy_action",
            TStringBuilder{} << "Actual type is " << directives[1].GetType());
}

NAlice::TExperimentsProto::TValue GetExpValue() {
    NAlice::TExperimentsProto::TValue experiment;
    experiment.SetString("1");
    return experiment;
}

void TProtocolScenarioWrapperTest::TestWarmUpApply() {
    auto scenario = CreateDefaultScenario();

    auto payload = TProtoStructBuilder{}
        .Set("key", "value")
        .Set(TString{NMegamind::SCENARIO_NAME_JSON_KEY}, scenario.GetName())
        .Build();

    const TString directiveName = "directive_name";

    NMegamind::TStackEngineCore stackEngine{};
    auto* stackEngineItem = stackEngine.AddItems();
    stackEngineItem->SetScenarioName("another_scenario");
    stackEngineItem->MutableEffect()->MutableCallback()->SetName("stack_engine_callback");

    EXPECT_CALL(Context, StackEngineCore()).WillRepeatedly(ReturnRef(stackEngine));

    auto speechKitRequest = TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithoutEvent}
        .SetProtoPatcher([&](NMegamind::TSpeechKitInitContext& initCtx) {
            auto& event = *initCtx.EventProtoPtr;
            event.SetType(NAlice::EEventType::server_action);
            event.SetName(directiveName);
            event.MutablePayload()->CopyFrom(payload);
            event.SetIsWarmUp(true);
        })
        .Build();

    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
    EXPECT_CALL(Context, Logger()).WillRepeatedly(ReturnRef(TRTLogger::StderrLogger()));

    TScenarioRunResponse runResponse;
    runResponse.MutableCommitCandidate()->MutableResponseBody()->MutableLayout()->AddCards()->SetText("response");

    EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
    EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(runResponse));

    const auto session = MakeSessionBuilder()
        ->SetPreviousScenarioName(scenario.GetName())
        .SetScenarioSession(scenario.GetName(), NewScenarioSession(TState{}))
        .Build();

    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

    auto wrapper = CreateWrapper(
        scenario, /* restoreAllFromSession= */ false, /* deferredApplyMode= */ EDeferredApplyMode::WarmUp);

    const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);

    EXPECT_CALL(Context, Event()).WillRepeatedly(Return(&request.GetEvent()));

    {
        const auto error = wrapper.Init(request, Context, DataSources);
        UNIT_ASSERT_C(!error.Defined(), *error);
    }

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    {
        auto error = wrapper.Ask(request, Context, response);
        UNIT_ASSERT_C(!error.Defined(), *error);
        error = wrapper.Finalize(request, Context, response);
        UNIT_ASSERT_C(!error.Defined(), *error);
    }

    EXPECT_CALL(Sensors, IncRate(NMonitoring::TLabels{{"name", "scenario.warmup_per_second"},
                                                      {"scenario_name", scenario.GetName()}}));
    {
        NMegamind::TMegamindAnalyticsInfo analyticsInfo;
        TQualityStorage storage;
        const TErrorOr<EApplyResult> result =
            static_cast<IScenarioWrapper&>(wrapper).StartApply(request, Context, response, analyticsInfo, storage, /* proactivity= */ {});
        UNIT_ASSERT_C(result.IsSuccess(), *result.Error());
        const auto* responseBuilder = response.BuilderIfExists();
        UNIT_ASSERT_C(responseBuilder, "There should be builder in response");
        const auto& directives = responseBuilder->ToProto().GetResponse().GetResponse().GetDirectives();
        UNIT_ASSERT_VALUES_EQUAL(directives.size(), 1);
        const auto& directive = directives[0];
        UNIT_ASSERT_VALUES_EQUAL(directive.GetName(), directiveName);
        // we should ignore this system field
        const TString reqIdKey{NMegamind::REQUEST_ID_JSON_KEY};
        (*payload.mutable_fields())[reqIdKey] = directive.GetPayload().fields().at(reqIdKey);
        UNIT_ASSERT_MESSAGES_EQUAL(directive.GetPayload(), payload);
    }
}

void TProtocolScenarioWrapperTest::TestWarmUpApplyGetNextWithSemanticFrame() {
    auto scenario = CreateDefaultScenario();

    auto speechKitRequest = TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithoutEvent}
                                .SetProtoPatcher([&](NMegamind::TSpeechKitInitContext& initCtx) {
                                    auto& event = *initCtx.EventProtoPtr;
                                    event.SetType(NAlice::EEventType::text_input);
                                    event.SetText("что такое ананас");
                                    event.SetIsWarmUp(true);
                                })
                                .Build();

    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
    EXPECT_CALL(Context, Logger()).WillRepeatedly(ReturnRef(TRTLogger::StderrLogger()));

    TScenarioRunResponse runResponse;
    runResponse.MutableCommitCandidate()->MutableResponseBody()->MutableLayout()->AddCards()->SetText("response");

    EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
    EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(runResponse));

    NMegamind::TStackEngineCore stackEngine{};
    stackEngine.SetSessionId(TEST_STACK_ENGINE_SESSION_ID);
    stackEngine.SetProductScenarioName(TEST_STACK_ENGINE_PRODUCT_SCENARIO_NAME);
    auto* stackEngineItem = stackEngine.AddItems();
    auto* recoveryAction = stackEngineItem->MutableRecoveryAction();
    recoveryAction->MutableCallback()->SetName("_some_recovery_action_");
    stackEngineItem->SetScenarioName("another_scenario");
    auto& effect = *stackEngineItem->MutableEffect();
    auto& parsedUtteranceEffect = *effect.MutableParsedUtterance();
    auto& frame = *parsedUtteranceEffect.MutableTypedSemanticFrame()->MutableSearchSemanticFrame();
    frame.MutableQuery()->SetStringValue("что такое ананас");
    parsedUtteranceEffect.SetUtterance("что такое ананас");
    auto& effectAnalytics = *parsedUtteranceEffect.MutableAnalytics();
    effectAnalytics.SetOrigin(TAnalyticsTrackingModule_EOrigin_Scenario);
    effectAnalytics.SetProductScenario("search");
    effectAnalytics.SetPurpose("get_factoid");
    effectAnalytics.SetOriginInfo("");

    const auto session = MakeSessionBuilder()
                             ->SetPreviousScenarioName(scenario.GetName())
                             .SetScenarioSession(scenario.GetName(), NewScenarioSession(TState{}))
                             .SetStackEngineCore(stackEngine)
                             .Build();

    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

    auto wrapper = CreateWrapper(scenario, /* restoreAllFromSession= */ false,
                                 /* deferredApplyMode= */ EDeferredApplyMode::WarmUp);

    auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                                 /* iotUserInfo = */ Nothing(),
                                 /* requestSource = */ NScenarios::TScenarioBaseRequest_ERequestSourceType_GetNext);

    EXPECT_CALL(Context, Event()).WillRepeatedly(Return(&request.GetEvent()));

    {
        const auto error = wrapper.Init(request, Context, DataSources);
        UNIT_ASSERT_C(!error.Defined(), *error);
    }

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    {
        auto error = wrapper.Ask(request, Context, response);
        UNIT_ASSERT_C(!error.Defined(), *error);
        error = wrapper.Finalize(request, Context, response);
        UNIT_ASSERT_C(!error.Defined(), *error);
    }

    EXPECT_CALL(Sensors, IncRate(NMonitoring::TLabels{{"name", "scenario.warmup_per_second"},
                                                      {"scenario_name", scenario.GetName()}}));
    {
        NMegamind::TMegamindAnalyticsInfo analyticsInfo;
        TQualityStorage storage;
        const TErrorOr<EApplyResult> result = static_cast<IScenarioWrapper&>(wrapper).StartApply(
            request, Context, response, analyticsInfo, storage, /* proactivity= */ {});
        UNIT_ASSERT_C(result.IsSuccess(), *result.Error());
        const auto* responseBuilder = response.BuilderIfExists();
        UNIT_ASSERT_C(responseBuilder, "There should be builder in response");
        const auto& directives = responseBuilder->ToProto().GetResponse().GetResponse().GetDirectives();
        UNIT_ASSERT_VALUES_EQUAL(directives.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(directives[0].GetName(), MM_STACK_ENGINE_GET_NEXT_CALLBACK_NAME);
        const auto expectedPayload =
            ParseProtoText<TSpeechKitResponseProto>(SPEECHKIT_REQUEST_WITH_GET_NEXT_DIRECTIVE_AND_RECOVERY_CALLBACK)
                .GetResponse()
                .GetDirectives()[0]
                .GetPayload();
        UNIT_ASSERT_MESSAGES_EQUAL(directives[0].GetPayload(), expectedPayload);
        UNIT_ASSERT(static_cast<IScenarioWrapper&>(wrapper).IsApplyNeededOnWarmUpRequestWithSemanticFrame());
    }
}

void TProtocolScenarioWrapperTest::TestWarmUpSemanticFrameApply() {
    auto scenario = CreateDefaultScenario();

    auto payload = TProtoStructBuilder{}
        .Set("typed_semantic_frame",
             TProtoStructBuilder{}
                .Set("search_semantic_frame",
                     TProtoStructBuilder{}
                        .Set("query",
                             TProtoStructBuilder{}
                                .Set("string_value", "что такое ананас")
                                .Build())
                        .Build())
                .Build())
        .Set("utterance", "что такое ананас")
        .Set("analytics",
             TProtoStructBuilder{}
                .Set("product_scenario", "search")
                .Set("origin", "Scenario")
                .Set("purpose", "get_factoid")
                .Build())
        .Build();

    auto speechKitRequest = TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithoutEvent}
        .SetProtoPatcher([&](NMegamind::TSpeechKitInitContext& initCtx) {
            auto& event = *initCtx.EventProtoPtr;
            event.SetType(NAlice::EEventType::server_action);
            event.SetName(TString{NMegamind::SEMANTIC_FRAME_REQUEST_NAME});
            event.MutablePayload()->CopyFrom(payload);
            event.SetIsWarmUp(true);
        })
        .Build();

    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
    EXPECT_CALL(Context, Logger()).WillRepeatedly(ReturnRef(TRTLogger::StderrLogger()));

    TScenarioRunResponse runResponse;
    runResponse.MutableCommitCandidate()->MutableResponseBody()->MutableLayout()->AddCards()->SetText("response");

    EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
    EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(runResponse));

    const auto session = MakeSessionBuilder()
        ->SetPreviousScenarioName(scenario.GetName())
        .SetScenarioSession(scenario.GetName(), NewScenarioSession(TState{}))
        .Build();

    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

    auto wrapper = CreateWrapper(
        scenario, /* restoreAllFromSession= */ false, /* deferredApplyMode= */ EDeferredApplyMode::WarmUp);

    const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);

    EXPECT_CALL(Context, Event()).WillRepeatedly(Return(&request.GetEvent()));

    {
        const auto error = wrapper.Init(request, Context, DataSources);
        UNIT_ASSERT_C(!error.Defined(), *error);
    }

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    {
        auto error = wrapper.Ask(request, Context, response);
        UNIT_ASSERT_C(!error.Defined(), *error);
        error = wrapper.Finalize(request, Context, response);
        UNIT_ASSERT_C(!error.Defined(), *error);
    }

    EXPECT_CALL(Sensors, IncRate(NMonitoring::TLabels{{"name", "scenario.warmup_per_second"},
                                                      {"scenario_name", scenario.GetName()}}));

    {
        NMegamind::TMegamindAnalyticsInfo analyticsInfo;
        TQualityStorage storage;
        const TErrorOr<EApplyResult> result =
            static_cast<IScenarioWrapper&>(wrapper).StartApply(request, Context, response, analyticsInfo, storage, /* proactivity= */ {});
        UNIT_ASSERT_C(result.IsSuccess(), *result.Error());
        const auto* responseBuilder = response.BuilderIfExists();
        UNIT_ASSERT_C(responseBuilder, "There should be builder in response");
        const auto& directives = responseBuilder->ToProto().GetResponse().GetResponse().GetDirectives();
        UNIT_ASSERT_VALUES_EQUAL(directives.size(), 1);
        auto directive = directives[0];
        UNIT_ASSERT_VALUES_EQUAL(directive.GetName(), TString{NMegamind::SEMANTIC_FRAME_REQUEST_NAME});
        // we should ignore this system field
        const TString reqIdKey{NMegamind::REQUEST_ID_JSON_KEY};
        (*payload.mutable_fields())[reqIdKey] = directive.GetPayload().fields().at(reqIdKey);
        // FIXME(g-kostin): After MEGAMIND-1893 there should not be such key
        directive.MutablePayload()->mutable_fields()->erase(TString{NMegamind::SCENARIO_NAME_JSON_KEY});
        UNIT_ASSERT_MESSAGES_EQUAL(directive.GetPayload(), payload);
    }
}


void TProtocolScenarioWrapperTest::TestWarmUpPureRequest() {
    auto scenario = CreateDefaultScenario();

    auto speechKitRequest = TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent}
        .SetProtoPatcher([&](NMegamind::TSpeechKitInitContext& initCtx) {
            auto& event = *initCtx.EventProtoPtr;
            event.SetIsWarmUp(true);
        })
        .Build();

    EXPECT_CALL(Context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
    EXPECT_CALL(Context, Logger()).WillRepeatedly(ReturnRef(TRTLogger::StderrLogger()));

    TScenarioRunResponse runResponse;
    runResponse.MutableResponseBody()->MutableLayout()->AddCards()->SetText("response");

    EXPECT_CALL(scenario, StartRun(_, _, _)).WillOnce(Return(Success()));
    EXPECT_CALL(scenario, FinishRun(_, _)).WillOnce(Return(runResponse));

    const auto session = MakeSessionBuilder()
        ->SetPreviousScenarioName(scenario.GetName())
        .SetScenarioSession(scenario.GetName(), NewScenarioSession(TState{}))
        .Build();

    EXPECT_CALL(Context, Session()).WillRepeatedly(Return(session.Get()));

    auto wrapper = CreateWrapper(
        scenario, /* restoreAllFromSession= */ false, /* deferredApplyMode= */ EDeferredApplyMode::WarmUp);

    const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);

    EXPECT_CALL(Context, Event()).WillRepeatedly(Return(&request.GetEvent()));

    {
        const auto error = wrapper.Init(request, Context, DataSources);
        UNIT_ASSERT_C(!error.Defined(), *error);
    }

    TScenarioResponse response{scenario.GetName(), {}, /* scenarioAcceptsAnyUtterance= */ true};
    {
        auto error = wrapper.Ask(request, Context, response);
        UNIT_ASSERT_C(!error.Defined(), *error);
        error = wrapper.Finalize(request, Context, response);
        UNIT_ASSERT_C(!error.Defined(), *error);
    }

    EXPECT_CALL(Sensors, IncRate(NMonitoring::TLabels{{"name", "scenario.warmup_per_second"},
                                                      {"scenario_name", scenario.GetName()}}));

    {
        NMegamind::TMegamindAnalyticsInfo analyticsInfo;
        TQualityStorage storage;
        const TErrorOr<EApplyResult> result =
            static_cast<IScenarioWrapper&>(wrapper).StartApply(request, Context, response, analyticsInfo, storage, /* proactivity= */ {});
        UNIT_ASSERT_C(result.IsSuccess(), *result.Error());
        const auto* responseBuilder = response.BuilderIfExists();
        UNIT_ASSERT_C(responseBuilder, "There should be builder in response");
        const auto& cards = responseBuilder->ToProto().GetResponse().GetResponse().GetCards();
        UNIT_ASSERT(!cards.empty());
        UNIT_ASSERT_EQUAL_C(cards[0].GetType(), "simple_text",
                            TStringBuilder{} << "Actual type is " << cards[0].GetType());
        UNIT_ASSERT_EQUAL_C(cards[0].GetText(), "response",
                            TStringBuilder{} << "Actual card text is " << cards[0].GetText());
    }
}


void TProtocolScenarioWrapperTest::TestMakeFramesForRequest() {
    auto makeFrame = [] (const TString& name) {
        TSemanticFrame frame;
        frame.SetName(name);
        return frame;
    };

    /* testDefault */ {
        TConfigBasedAppHostPureProtocolScenario scenario{TScenarioConfig{}};
        TVector<TSemanticFrame> requestFrames;
        requestFrames.push_back(makeFrame("request_frame_1"));
        TVector<TSemanticFrame> allParsedFrames;
        allParsedFrames.push_back(makeFrame("all_frame_1"));
        auto result = MakeFramesForRequest(requestFrames, allParsedFrames, scenario);
        UNIT_ASSERT(result.size() == 1);
        UNIT_ASSERT(result[0].GetName() == "request_frame_1");
    }

    /* testNoAllParsedFrames */ {
        TScenarioConfig config;
        config.SetAlwaysRecieveAllParsedSemanticFrames(true);
        TConfigBasedAppHostPureProtocolScenario scenario{config};
        TVector<TSemanticFrame> requestFrames;
        requestFrames.push_back(makeFrame("request_frame_1"));
        TVector<TSemanticFrame> allParsedFrames;
        auto result = MakeFramesForRequest(requestFrames, allParsedFrames, scenario);
        UNIT_ASSERT(result.size() == 1);
        UNIT_ASSERT(result[0].GetName() == "request_frame_1");
    }

    /* testWithAllParsedFrames */ {
        TScenarioConfig config;
        config.SetAlwaysRecieveAllParsedSemanticFrames(true);
        config.AddAcceptedFrames("all_frame_2");
        TConfigBasedAppHostPureProtocolScenario scenario{config};
        TVector<TSemanticFrame> requestFrames;
        requestFrames.push_back(makeFrame("request_frame_1"));
        TVector<TSemanticFrame> allParsedFrames;
        allParsedFrames.push_back(makeFrame("all_frame_1"));
        allParsedFrames.push_back(makeFrame("all_frame_2"));
        auto result = MakeFramesForRequest(requestFrames, allParsedFrames, scenario);
        UNIT_ASSERT(result.size() == 2);
        UNIT_ASSERT(result[0].GetName() == "request_frame_1");
        UNIT_ASSERT(result[1].GetName() == "all_frame_2");
    }

    /* testNotAddingIfHas */ {
        TScenarioConfig config;
        config.SetAlwaysRecieveAllParsedSemanticFrames(true);
        config.AddAcceptedFrames("all_frame_2");
        config.AddAcceptedFrames("request_frame_1");
        TConfigBasedAppHostPureProtocolScenario scenario{config};
        TVector<TSemanticFrame> requestFrames;
        requestFrames.push_back(makeFrame("request_frame_1"));
        TVector<TSemanticFrame> allParsedFrames;
        allParsedFrames.push_back(makeFrame("all_frame_1"));
        allParsedFrames.push_back(makeFrame("all_frame_2"));
        allParsedFrames.push_back(makeFrame("request_frame_1"));
        auto result = MakeFramesForRequest(requestFrames, allParsedFrames, scenario);
        UNIT_ASSERT(result.size() == 2);
        UNIT_ASSERT(result[0].GetName() == "request_frame_1");
        UNIT_ASSERT(result[1].GetName() == "all_frame_2");
    }
}

} // namespace NAlice
