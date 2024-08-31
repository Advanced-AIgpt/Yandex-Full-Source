#include "alice/protos/data/scenario/data.pb.h"
#include "alice/protos/data/scenario/photoframe/screen_saver.pb.h"
#include "centaur_teasers.h"

#include <alice/hollywood/library/combinators/protos/used_scenarios.pb.h>
#include <alice/hollywood/library/testing/mock_global_context.h>

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/library/unittest/mock_sensors.h>
#include <alice/megamind/protos/scenarios/combinator_request.pb.h>
#include <alice/megamind/protos/scenarios/combinator_response.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/memento/proto/device_configs.pb.h>
#include <alice/protos/data/scenario/centaur/teasers/teaser_settings.pb.h>

#include <library/cpp/testing/unittest/registar.h>

#include <apphost/lib/service_testing/service_testing.h>
#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NHollywood {

const TString CAROUSEL_SERVER_UPDATE_EXP_FLAG_NAME = "centaur_carousel_server_updates";
const TString AFISHA_TEASER_EXP_FLAG_NAME = "afisha_teaser";
const TString SKILLS_TEASER_EXP_FLAG_NAME = "collect_teasers_skills";
const TString TEASER_SETTINGS_EXP_FLAG_NAME = "teaser_settings";
const TString DEVICE_ID = "device_1";

namespace {

NCombinators::NMemento::TCentaurTeasersDeviceConfig PrepareDafaultCentaurTeasersDeviceConfig() {
    NCombinators::NMemento::TCentaurTeasersDeviceConfig centaurTeasersDeviceConfig;
    auto& teaserConfig = *centaurTeasersDeviceConfig.AddTeaserConfigs();
    teaserConfig.SetTeaserType("PhotoFrame");

    return centaurTeasersDeviceConfig;
}

NCombinators::NMemento::TCentaurTeasersDeviceConfig PrepareCentaurTeasersDeviceConfigWithNews() {
    NCombinators::NMemento::TCentaurTeasersDeviceConfig centaurTeasersDeviceConfig;
    auto& photoFrameTeaserConfig = *centaurTeasersDeviceConfig.AddTeaserConfigs();
    photoFrameTeaserConfig.SetTeaserType("PhotoFrame");
    auto& afishaTeaserConfig = *centaurTeasersDeviceConfig.AddTeaserConfigs();
    afishaTeaserConfig.SetTeaserType("Afisha");

    return centaurTeasersDeviceConfig;
}

} // namespace

Y_UNIT_TEST_SUITE(CentaurCombinatorTests) {
    Y_UNIT_TEST(TestGenerateСombinatorFinalize) {
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

        NCombinators::TCentaurCombinatorFinalizeHandle{}.Do(hwServiceCtx);
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
                    "Weather": {
                        "is_irrelevant": true
                    },
                    "HollywoodMusic": {
                        "is_irrelevant": false
                    }
                },
                "combinator_product_name": "CentaurTeasersCombinator"
            }
        })")));
        UNIT_ASSERT_MESSAGES_EQUAL(response, expectedResponse);
    }
    Y_UNIT_TEST(TestIrrelevant) {
        TMockGlobalContext globalCtx;

        NAppHost::NService::TTestContext serviceCtx;
        NScenarios::TCombinatorRequest request;
        auto& frame = *request.MutableInput()->AddSemanticFrames();
        frame.SetName("alice.centaur.collect_cards");
        serviceCtx.AddProtobufItem(request, "combinator_request_apphost_type");

        NAppHostHttp::THttpResponse bbResponse;
        serviceCtx.AddProtobufItem(bbResponse, "blackbox_http_response");

        THwServiceContext hwServiceCtx{globalCtx, serviceCtx, TRTLogger::NullLogger()};

        NCombinators::TCentaurCombinatorContinueHandle{}.Do(hwServiceCtx);
        const auto response = hwServiceCtx.GetProtoOrThrow<NScenarios::TScenarioRunResponse>("mm_scenario_response");
        UNIT_ASSERT(response.GetFeatures().GetIsIrrelevant());
    }
    Y_UNIT_TEST(TestWithAllScenarios) {
        TMockGlobalContext globalCtx;

        NAppHost::NService::TTestContext serviceCtx;
        NScenarios::TCombinatorRequest request;
        auto& frame = *request.MutableInput()->AddSemanticFrames();
        frame.SetName("alice.centaur.collect_cards");
        auto& carouselSlot = *frame.AddSlots();
        carouselSlot.SetName("carousel_id");
        carouselSlot.SetValue("test_carousel_id");
        auto& exps = *request.MutableBaseRequest()->MutableExperiments()->mutable_fields();
        exps[SKILLS_TEASER_EXP_FLAG_NAME].set_string_value("1");
        serviceCtx.AddProtobufItem(request, "combinator_request_apphost_type");

        {
            NScenarios::TScenarioRunResponse response;
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("centaur_card_id");
            serviceCtx.AddProtobufItem(response, "scenario_Centaur_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        {
            NScenarios::TScenarioRunResponse response;
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("weather_card_id");
            serviceCtx.AddProtobufItem(response, "scenario_Weather_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        {
            NScenarios::TScenarioRunResponse response;
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("photo_frame_card_id_1");
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("photo_frame_card_id_2");
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("photo_frame_card_id_3");
            serviceCtx.AddProtobufItem(response, "scenario_PhotoFrame_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        {
            NScenarios::TScenarioRunResponse response;
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("afisha_teaser_card_id");
            serviceCtx.AddProtobufItem(response, "scenario_Afisha_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        {
            NScenarios::TScenarioRunResponse response;
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("external_skill_card_id");
            serviceCtx.AddProtobufItem(response, "scenario_Dialogovo_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        {
            NScenarios::TScenarioRunResponse response;
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("news_card_id1");
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("news_card_id2");
            // simply not AddCardDirective directive
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableShowPlusPromoDirective();
            serviceCtx.AddProtobufItem(response, "scenario_News_run_pure_response", NAppHost::EContextItemKind::Input);
        }

        NAppHostHttp::THttpResponse bbResponse;
        serviceCtx.AddProtobufItem(bbResponse, "blackbox_http_response");

        THwServiceContext hwServiceCtx{globalCtx, serviceCtx, TRTLogger::NullLogger()};

        NCombinators::TCentaurCombinatorContinueHandle{}.Do(hwServiceCtx);
        const auto response = hwServiceCtx.GetProtoOrThrow<NScenarios::TScenarioRunResponse>("mm_scenario_response");

        const auto expectedResponse = JsonToProto<NScenarios::TScenarioRunResponse>(JsonFromString(TStringBuf(R"({
            "response_body": {
                "layout": {
                    "directives": [
                        {
                            "add_card": {
                                "card_id": "photo_frame_card_id_1"
                            }
                        },
                        {
                            "add_card": {
                                "card_id": "afisha_teaser_card_id"
                            }
                        },
                        {
                            "add_card": {
                                "card_id": "photo_frame_card_id_2"
                            }
                        },
                        {
                            "add_card": {
                                "card_id": "external_skill_card_id"
                            }
                        },
                        {
                            "add_card": {
                                "card_id": "photo_frame_card_id_3"
                            }
                        },
                        {
                            "add_card": {
                                "card_id": "news_card_id1"
                            }
                        },
                        {
                            "add_card": {
                                "card_id": "news_card_id2"
                            }
                        },
                        {
                            "add_card": {
                                "card_id": "weather_card_id"
                            }
                        },
                        {
                            "rotate_cards": {
                                "carousel_show_time_sec": 300,
                                "carousel_id": "test_carousel_id"
                            }
                        }
                    ]
                }
            }
        })")));
        UNIT_ASSERT_MESSAGES_EQUAL(response, expectedResponse);
    }
    Y_UNIT_TEST(TestWithOneScenario) {
        TMockGlobalContext globalCtx;

        testing::StrictMock<TMockSensors> sensors;
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{{"name", "combinator_missed_scenarios_per_second"},
                                                          {"combinator_name", "CentaurCombinator"},
                                                          {"scenario_name", "PhotoFrame"}}));
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{{"name", "combinator_missed_scenarios_per_second"},
                                                          {"combinator_name", "CentaurCombinator"},
                                                          {"scenario_name", "News"}}));
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{{"name", "combinator_missed_scenarios_per_second"},
                                                          {"combinator_name", "CentaurCombinator"},
                                                          {"scenario_name", "Afisha"}}));                                                  
        EXPECT_CALL(globalCtx, Sensors()).WillRepeatedly(ReturnRef(sensors));

        NAppHost::NService::TTestContext serviceCtx;
        NScenarios::TCombinatorRequest request;
        auto& frame = *request.MutableInput()->AddSemanticFrames();
        frame.SetName("alice.centaur.collect_cards");
        auto& carouselSlot = *frame.AddSlots();
        carouselSlot.SetName("carousel_id");
        carouselSlot.SetValue("test_carousel_id");
        serviceCtx.AddProtobufItem(request, "combinator_request_apphost_type");

        {
            NScenarios::TScenarioRunResponse response;
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("weather_card_id");
            serviceCtx.AddProtobufItem(response, "scenario_Weather_run_pure_response", NAppHost::EContextItemKind::Input);
        }

        NAppHostHttp::THttpResponse bbResponse;
        serviceCtx.AddProtobufItem(bbResponse, "blackbox_http_response");

        THwServiceContext hwServiceCtx{globalCtx, serviceCtx, TRTLogger::NullLogger()};

        NCombinators::TCentaurCombinatorContinueHandle{}.Do(hwServiceCtx);
        const auto response = hwServiceCtx.GetProtoOrThrow<NScenarios::TScenarioRunResponse>("mm_scenario_response");
        const auto expectedResponse = JsonToProto<NScenarios::TScenarioRunResponse>(JsonFromString(TStringBuf(R"({
            "response_body": {
                "layout": {
                    "directives": [
                        {
                            "add_card": {
                                "card_id": "weather_card_id"
                            }
                        },
                        {
                            "rotate_cards": {
                                "carousel_show_time_sec": 300,
                                "carousel_id": "test_carousel_id"
                            }
                        }
                    ]
                }
            }
        })")));
        UNIT_ASSERT_MESSAGES_EQUAL(response, expectedResponse);
    }
    Y_UNIT_TEST(TestWithSheduledUpdated) {
        TMockGlobalContext globalCtx;

        NAppHost::NService::TTestContext serviceCtx;
        NScenarios::TCombinatorRequest request;
        auto& frame = *request.MutableInput()->AddSemanticFrames();
        frame.SetName("alice.centaur.collect_cards");
        auto& carouselSlot = *frame.AddSlots();
        carouselSlot.SetName("carousel_id");
        carouselSlot.SetValue("test_carousel_id");

        auto& baseRequest = *request.MutableBaseRequest();
        auto& exps = *request.MutableBaseRequest()->MutableExperiments()->mutable_fields();
        exps[CAROUSEL_SERVER_UPDATE_EXP_FLAG_NAME].set_string_value("1");

        auto& clientInfo = *baseRequest.MutableClientInfo();
        clientInfo.SetDeviceId("7977c01a-4885-11ec-81d3-0242ac130003");

        serviceCtx.AddProtobufItem(request, "combinator_request_apphost_type");

        {
            NScenarios::TScenarioRunResponse response;
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("centaur_card_id");
            serviceCtx.AddProtobufItem(response, "scenario_Centaur_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        {
            NScenarios::TScenarioRunResponse response;
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("weather_card_id");
            serviceCtx.AddProtobufItem(response, "scenario_Weather_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        {
            NScenarios::TScenarioRunResponse response;
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("photo_frame_card_id");
            serviceCtx.AddProtobufItem(response, "scenario_PhotoFrame_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        {
            NScenarios::TScenarioRunResponse response;
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("afisha_card_id");
            serviceCtx.AddProtobufItem(response, "scenario_Afisha_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        {
            NScenarios::TScenarioRunResponse response;
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("news_card_id1");
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("news_card_id2");
            // simply not AddCardDirective directive
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableShowPlusPromoDirective();
            serviceCtx.AddProtobufItem(response, "scenario_News_run_pure_response", NAppHost::EContextItemKind::Input);
        }

        NAppHostHttp::THttpResponse bbResponse;
        bbResponse.SetContent(R"({"uid":{"value":"11111111"}})");
        bbResponse.SetStatusCode(200);
        serviceCtx.AddProtobufItem(bbResponse, "blackbox_http_response");

        THwServiceContext hwServiceCtx{globalCtx, serviceCtx, TRTLogger::NullLogger()};

        NCombinators::TCentaurCombinatorContinueHandle{}.Do(hwServiceCtx);
        const auto response = hwServiceCtx.GetProtoOrThrow<NScenarios::TScenarioRunResponse>("mm_scenario_response");

        const auto expectedResponse = JsonToProto<NScenarios::TScenarioRunResponse>(JsonFromString(TStringBuf(R"({
            "response_body": {
                "layout": {
                    "directives": [
                        {
                            "add_card": {
                                "card_id": "photo_frame_card_id"
                            }
                        },
                        {
                            "add_card": {
                                "card_id": "afisha_card_id"
                            }
                        },
                        {
                            "add_card": {
                                "card_id": "news_card_id1"
                            }
                        },
                        {
                            "add_card": {
                                "card_id": "news_card_id2"
                            }
                        },
                        {
                            "add_card": {
                                "card_id": "weather_card_id"
                            }
                        },
                        {
                            "rotate_cards": {
                                "carousel_show_time_sec": 300,
                                "carousel_id": "test_carousel_id"
                            }
                        }
                    ]
                },
                "ServerDirectives": [
                    {
                        "AddScheduleActionDirective": {
                            "ScheduleAction": {
                                "Id": "update_carousel_11111111_7977c01a-4885-11ec-81d3-0242ac130003",
                                "DeviceId": "7977c01a-4885-11ec-81d3-0242ac130003",
                                "Puid": "11111111",
                                "SendPolicy": {
                                    "SendPeriodicallyPolicy": {
                                        "PeriodMs": 900000,
                                        "RetryPolicy": {
                                            "MaxRetries": 1000,
                                            "RestartPeriodScaleMs":900000,
                                            "RestartPeriodBackOff": 2,
                                            "MaxRestartPeriodMs": 604800000,
                                            "DoNotRetryIfDeviceOffline": true
                                        }
                                    }
                                },
                                "Action": {
                                    "OldNotificatorRequest": {
                                        "Delivery": {
                                            "DeviceId": "7977c01a-4885-11ec-81d3-0242ac130003",
                                            "Puid": "11111111",
                                            "SemanticFrameRequestData": {
                                                "TypedSemanticFrame": {
                                                    "CentaurCollectCardsSemanticFrame": {
                                                        "IsScheduledUpdate": {
                                                            "BoolValue": true
                                                        }
                                                    }
                                                },
                                                "Analytics": {
                                                    "ProductScenario": "CentaurTeasersCombinator",
                                                    "Origin":2,
                                                    "Purpose":"collect_carousel_cards"
                                                }
                                            }
                                        }
                                    }
                                },
                                "Override": true
                            }
                        }
                    }
                ]
            }
        })")));
        UNIT_ASSERT_MESSAGES_EQUAL(response, expectedResponse);
    }
    Y_UNIT_TEST(TestWithTeaserSettingsScreen) {
        TMockGlobalContext globalCtx;

        NAppHost::NService::TTestContext serviceCtx;
        NScenarios::TCombinatorRequest request;

        auto& frame = *request.MutableInput()->AddSemanticFrames();
        frame.SetName("alice.centaur.collect_teasers_preview");

        request.MutableBaseRequest()->MutableClientInfo()->SetDeviceId(DEVICE_ID);
        NCombinators::NMemento::TSurfaceConfig surfaceConfig;
        *surfaceConfig.MutableCentaurTeasersDeviceConfig() = PrepareDafaultCentaurTeasersDeviceConfig();
        NCombinators::NMemento::TRespGetAllObjects fullMementoData;
        (*fullMementoData.MutableSurfaceConfigs())[DEVICE_ID] = surfaceConfig;
        serviceCtx.AddProtobufItem(fullMementoData, "full_memento_data");

        serviceCtx.AddProtobufItem(request, "combinator_request_apphost_type");

        {
            NScenarios::TScenarioRunResponse response;
            const auto& teaserPreviewData = response.MutableResponseBody()->MutableScenarioData()->MutableTeasersPreviewData()->AddTeaserPreviews();
            teaserPreviewData->SetTeaserName("Name");
            const auto& teaserConfigData = teaserPreviewData->MutableTeaserConfigData();
            teaserConfigData->SetTeaserType("type");
            teaserConfigData->SetTeaserId("id");
            const auto& data = teaserPreviewData->MutableTeaserPreviewScenarioData()->MutableScreenSaverData();
            data->SetImageUrl("url");
            serviceCtx.AddProtobufItem(response, "scenario_PhotoFrame_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        NAppHostHttp::THttpResponse bbResponse;
        serviceCtx.AddProtobufItem(bbResponse, "blackbox_http_response");

        THwServiceContext hwServiceCtx{globalCtx, serviceCtx, TRTLogger::NullLogger()};

        NCombinators::TCentaurCombinatorContinueHandle{}.Do(hwServiceCtx);
        const auto response = hwServiceCtx.GetProtoOrThrow<NScenarios::TScenarioRunResponse>("mm_scenario_response");

        const auto expectedResponse = JsonToProto<NScenarios::TScenarioRunResponse>(JsonFromString(TStringBuf(R"({
            "response_body": {
                "layout": {
                    "directives": [
                        {
                            "show_view": {
                                "name": "show_view",
                                "layer": {
                                    "dialog": {}
                                },
                                "inactivity_timeout": "Infinity",
                                "do_not_show_close_button": true,
                                "card_id": "teaser.settings.id"
                            }
                        }
                    ]
                }
            }
        })")));
        UNIT_ASSERT_MESSAGES_EQUAL(response, expectedResponse);
    }
    Y_UNIT_TEST(TestWithCollectCardsNewApproach) {
        TMockGlobalContext globalCtx;

        NAppHost::NService::TTestContext serviceCtx;
        NScenarios::TCombinatorRequest request;

        auto& frame = *request.MutableInput()->AddSemanticFrames();
        frame.SetName("alice.centaur.collect_cards");

        auto& carouselSlot = *frame.AddSlots();
        carouselSlot.SetName("carousel_id");
        carouselSlot.SetValue("test_carousel_id");

        auto& exps = *request.MutableBaseRequest()->MutableExperiments()->mutable_fields();
        exps[TEASER_SETTINGS_EXP_FLAG_NAME].set_string_value("1");

        request.MutableBaseRequest()->MutableClientInfo()->SetDeviceId(DEVICE_ID);
        NCombinators::NMemento::TSurfaceConfig surfaceConfig;
        *surfaceConfig.MutableCentaurTeasersDeviceConfig() = PrepareCentaurTeasersDeviceConfigWithNews();
        NCombinators::NMemento::TRespGetAllObjects fullMementoData;
        (*fullMementoData.MutableSurfaceConfigs())[DEVICE_ID] = surfaceConfig;
        serviceCtx.AddProtobufItem(fullMementoData, "full_memento_data");

        serviceCtx.AddProtobufItem(request, "combinator_request_apphost_type");

        {
            NScenarios::TScenarioRunResponse response;
            auto& addCardDirective = *response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective();
            addCardDirective.SetCardId("weather_card_id");
            auto& teaserConfig = *addCardDirective.MutableTeaserConfig();
            teaserConfig.SetTeaserType("Weather");
            serviceCtx.AddProtobufItem(response, "scenario_Weather_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        {
            NScenarios::TScenarioRunResponse response;
            
            auto& addCardDirective = *response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective();
            addCardDirective.SetCardId("photo_frame_card_id_1");
            auto& teaserConfig = *addCardDirective.MutableTeaserConfig();
            teaserConfig.SetTeaserType("PhotoFrame");

            auto& addCardDirective2 = *response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective();
            addCardDirective2.SetCardId("photo_frame_card_id_2");
            auto& teaserConfig2 = *addCardDirective2.MutableTeaserConfig();
            teaserConfig2.SetTeaserType("PhotoFrame");
            
            serviceCtx.AddProtobufItem(response, "scenario_PhotoFrame_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        {
            NScenarios::TScenarioRunResponse response;
            auto& addCardDirective = *response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective();
            addCardDirective.SetCardId("afisha_teaser_card_id");
            auto& teaserConfig = *addCardDirective.MutableTeaserConfig();
            teaserConfig.SetTeaserType("Afisha");
            serviceCtx.AddProtobufItem(response, "scenario_Afisha_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        {
            NScenarios::TScenarioRunResponse response;
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective()->SetCardId("external_skill_card_id");
            serviceCtx.AddProtobufItem(response, "scenario_Dialogovo_run_pure_response", NAppHost::EContextItemKind::Input);
        }
        {
            NScenarios::TScenarioRunResponse response;
            auto& addCardDirective = *response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective();
            addCardDirective.SetCardId("news_card_id1");
            auto& teaserConfig = *addCardDirective.MutableTeaserConfig();
            teaserConfig.SetTeaserType("News");
            // simply not AddCardDirective directive
            response.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableShowPlusPromoDirective();
            serviceCtx.AddProtobufItem(response, "scenario_News_run_pure_response", NAppHost::EContextItemKind::Input);
        }

        NAppHostHttp::THttpResponse bbResponse;
        serviceCtx.AddProtobufItem(bbResponse, "blackbox_http_response");

        THwServiceContext hwServiceCtx{globalCtx, serviceCtx, TRTLogger::NullLogger()};

        NCombinators::TCentaurCombinatorContinueHandle{}.Do(hwServiceCtx);
        const auto response = hwServiceCtx.GetProtoOrThrow<NScenarios::TScenarioRunResponse>("mm_scenario_response");

        const auto expectedResponse = JsonToProto<NScenarios::TScenarioRunResponse>(JsonFromString(TStringBuf(R"({
            "response_body": {
                "layout": {
                    "directives": [
                        {
                            "add_card": {
                                "card_id": "photo_frame_card_id_1",
                                "teaser_config": {
                                    "teaser_type": "PhotoFrame"
                                }
                            }
                        },
                        {
                            "add_card": {
                                "card_id": "afisha_teaser_card_id",
                                "teaser_config": {
                                    "teaser_type": "Afisha"
                                }
                            }
                        },
                        {
                            "add_card": {
                                "card_id": "photo_frame_card_id_2",
                                "teaser_config": {
                                    "teaser_type": "PhotoFrame"
                                }
                            }
                        },
                        {
                            "rotate_cards": {
                                "carousel_show_time_sec": 300,
                                "carousel_id": "test_carousel_id"
                            }
                        }
                    ]
                }
            }
        })")));
        UNIT_ASSERT_MESSAGES_EQUAL(response, expectedResponse);
    }
    Y_UNIT_TEST(TestWithAddTeaserSettings) {
        TMockGlobalContext globalCtx;

        NAppHost::NService::TTestContext serviceCtx;
        NScenarios::TCombinatorRequest request;

        auto& frame = *request.MutableInput()->AddSemanticFrames();
        frame.SetName("alice.centaur.set_teaser_configuration");

        {
            auto& slot = *frame.AddSlots();
            slot.SetName("scenarios_for_teasers_slot");
            slot.SetType("TeaserSettingsDataValue");
            *slot.AddAcceptedTypes() = "TeaserSettingsDataValue";
        }

        auto& setTeaserConfSemanticFrame = *frame.MutableTypedSemanticFrame()->MutableCentaurSetTeaserConfigurationSemanticFrame();
        auto& teaserSetting = *setTeaserConfSemanticFrame.MutableScenariosForTeasersSlot()->MutableTeaserSettingsData()->AddTeaserSettings();
        auto& teaserConfigData = *teaserSetting.MutableTeaserConfigData();
        teaserConfigData.SetTeaserType("PhotoFrame");
        teaserSetting.SetIsChosen(true);
       
        auto& teaserSettingAfisha = *setTeaserConfSemanticFrame.MutableScenariosForTeasersSlot()->MutableTeaserSettingsData()->AddTeaserSettings();
        auto& teaserConfigDataAfisha = *teaserSettingAfisha.MutableTeaserConfigData();
        teaserConfigDataAfisha.SetTeaserType("Afisha");
        teaserSettingAfisha.SetIsChosen(false);

        request.MutableBaseRequest()->MutableClientInfo()->SetDeviceId(DEVICE_ID);
        NCombinators::NMemento::TSurfaceConfig surfaceConfig;
        *surfaceConfig.MutableCentaurTeasersDeviceConfig() = PrepareCentaurTeasersDeviceConfigWithNews();
        NCombinators::NMemento::TRespGetAllObjects fullMementoData;
        (*fullMementoData.MutableSurfaceConfigs())[DEVICE_ID] = surfaceConfig;
        serviceCtx.AddProtobufItem(fullMementoData, "full_memento_data");

        serviceCtx.AddProtobufItem(request, "combinator_request_apphost_type");

        NAppHostHttp::THttpResponse bbResponse;
        serviceCtx.AddProtobufItem(bbResponse, "blackbox_http_response");

        THwServiceContext hwServiceCtx{globalCtx, serviceCtx, TRTLogger::NullLogger()};

        NCombinators::TCentaurCombinatorContinueHandle{}.Do(hwServiceCtx);
        const auto response = hwServiceCtx.GetProtoOrThrow<NScenarios::TScenarioRunResponse>("mm_scenario_response");

        const auto expectedResponse = JsonToProto<NScenarios::TScenarioRunResponse>(JsonFromString(TStringBuf(R"({
            "response_body": {
                "layout": {
                    "cards": [ 
                        {
                            "text": "Настройки для тизеров сохранены."
                        }
                    ],
                    "output_speech": " ",
                    "directives": [
                        {
                            "callback_directive":
                            {
                                "name":"@@mm_semantic_frame",
                                "payload":
                                {
                                    "analytics":
                                    {
                                        "origin":"SmartSpeaker",
                                        "product_scenario":"CentaurTeasersCombinator",
                                        "purpose":"refresh_teasers"
                                    },
                                    "typed_semantic_frame":
                                    {
                                        "centaur_collect_cards": {}
                                    }
                                }
                            }
                        }
                    ]
                },
                "ServerDirectives": [
                    {
                        "MementoChangeUserObjectsDirective": {
                            "user_objects": {
                                "DevicesConfigs": [
                                    {
                                        "DeviceConfigs":
                                            [
                                            {
                                                "Key":"DCK_CENTAUR_TEASERS",
                                                "Value":
                                                {
                                                    "@type":"type.googleapis.com/ru.yandex.alice.memento.proto.TCentaurTeasersDeviceConfig",
                                                    "teaser_configs": [
                                                        {
                                                            "teaser_type": "PhotoFrame"
                                                        }
                                                        
                                                    ]
                                                }
                                            }
                                            ],
                                        "DeviceId":"device_1"
                                    }
                                ]
                            }
                        }
                    }
                ]
            }
        })")));
        UNIT_ASSERT_MESSAGES_EQUAL(response, expectedResponse);
    }

}

} // namespace NAlice::NHollywood
