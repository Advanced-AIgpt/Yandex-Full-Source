#include "builder.h"
#include "utils.h"

#include <alice/megamind/library/models/buttons/action_button_model.h>
#include <alice/megamind/library/models/cards/text_card_model.h>
#include <alice/megamind/library/models/cards/text_with_button_card_model.h>
#include <alice/megamind/library/models/directives/callback_directive_model.h>
#include <alice/megamind/library/models/directives/open_dialog_directive_model.h>
#include <alice/megamind/library/models/directives/protobuf_uniproxy_directive_model.h>
#include <alice/megamind/library/models/directives/update_datasync_directive_model.h>
#include <alice/megamind/library/testing/fake_guid_generator.h>
#include <alice/megamind/library/testing/mock_guid_generator.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/utils.h>

#include <alice/megamind/protos/common/response_error_message.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/protos/api/matrix/scheduler_api.pb.h>

#include <alice/library/proto/protobuf.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace testing;

namespace {

constexpr TStringBuf MEGAREQUEST1 = TStringBuf(R"(
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
    }
})");

constexpr TStringBuf MEGAREQUEST1_WITH_REQUEST_ID = TStringBuf(R"(
{
    "header" : {
        "request_id" : "RequestId",
        "ref_message_id" : "RefMessageId",
        "session_id" : "SessionId"
    },
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
    }
})");

constexpr TStringBuf MEGAREQUEST1_WITHOUT_VOICE_SESSION = TStringBuf(R"(
{
    "header" : {
        "request_id" : "RequestId"
    },
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
        "voice_session": false,
        "event": {
            "type": "text_input"
        }
    }
})");

constexpr TStringBuf MEGAREQUEST_WITH_EXP = TStringBuf(R"(
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
        },
        "experiments": {
            "some_flag1": "123",
            "some_flag2": "abc"
        }
    }
})");

constexpr TStringBuf MEGAREQUEST_WITH_ENABLED_MOVED_EXP = TStringBuf(R"(
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
        },
        "experiments": {
            "analytics_info": true,
            "some_flag1": "123",
            "some_flag2": null
        }
    }
})");

constexpr TStringBuf MEGAREQUEST_WITH_DISABLED_MOVED_EXP = TStringBuf(R"(
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
        },
        "experiments": {
            "analytics_info": null,
            "some_flag1": "123",
            "some_flag2": null
        }
    }
})");

Y_UNIT_TEST_SUITE(Builder) {
    Y_UNIT_TEST(ProtocolResponseHasNo64BitNumber) {
        UNIT_ASSERT_C(!DoesProtoHave64BitNumber<TSpeechKitResponseProto_TResponse>(),
                      "64 bit numbers are not supported in TSpeechKitResponseProto.Response");
    }

    Y_UNIT_TEST(Experiments) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST_WITH_EXP}.Build();

        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).Times(1).WillRepeatedly(Return("deadbeef"));

        TResponseBuilderProto storage;
        TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "iot", storage, generator};

        {
            NJson::TJsonValue payload;
            payload["success"] = true;
            builder.AddMeta("iot-info", payload);
        }

        {
            auto skResponse = builder.GetSKRProto();
            const auto actual = SpeechKitResponseToJson(skResponse);

            const auto expected = NJson::ReadJsonFastTree(TStringBuf(R"({
                        "header": {
                            "dialog_id": null,
                            "request_id": "",
                            "response_id": "deadbeef",
                            "ref_message_id": "",
                            "session_id": ""
                        },
                        "response": {
                            "meta": [{
                                "type": "iot-info",
                                "payload": {"success": true}
                            }],
                            "card":null,
                            "cards":[],
                            "directives": [],
                            "experiments": {}
                        },
                        "voice_response": {
                            "should_listen": false
                        }
                    })"));

            UNIT_ASSERT_VALUES_EQUAL(expected, actual);
        }
    }

    Y_UNIT_TEST(MovedExperiments) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST_WITH_ENABLED_MOVED_EXP}.Build();

        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).Times(1).WillRepeatedly(Return("deadbeef"));

        TResponseBuilderProto storage;
        TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "iot", storage, generator};

        {
            NJson::TJsonValue payload;
            payload["success"] = true;
            builder.AddMeta("iot-info", payload);
        }

        {
            auto skResponse = builder.GetSKRProto();
            const auto actual = SpeechKitResponseToJson(skResponse);

            const auto expected = NJson::ReadJsonFastTree(TStringBuf(R"({
                    "header": {
                        "dialog_id": null,
                        "request_id": "",
                        "response_id": "deadbeef",
                        "ref_message_id": "",
                        "session_id": ""
                    },
                    "response": {
                        "meta": [{
                            "type": "iot-info",
                            "payload": {"success": true}
                        }],
                        "card":null,
                        "cards":[],
                        "directives": [],
                        "experiments": {
                            "analytics_info": true,
                            "some_flag1": "123",
                            "some_flag2": null
                        }
                    },
                    "voice_response": {
                        "should_listen": false
                    }
                })"));

            UNIT_ASSERT_VALUES_EQUAL(expected, actual);
        }
    }

    Y_UNIT_TEST(DisabledExperiments) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST_WITH_DISABLED_MOVED_EXP}.Build();

        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).Times(1).WillRepeatedly(Return("deadbeef"));

        TResponseBuilderProto storage;
        TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "iot", storage, generator};

        {
            NJson::TJsonValue payload;
            payload["success"] = true;
            builder.AddMeta("iot-info", payload);
        }

        {
            auto skResponse = builder.GetSKRProto();
            const auto actual = SpeechKitResponseToJson(skResponse);

            const auto expected = NJson::ReadJsonFastTree(TStringBuf(R"({
                    "header": {
                        "dialog_id": null,
                        "request_id": "",
                        "response_id": "deadbeef",
                        "ref_message_id": "",
                        "session_id": ""
                    },
                    "response": {
                        "meta": [{
                            "type": "iot-info",
                            "payload": {"success": true}
                        }],
                        "card":null,
                        "cards":[],
                        "directives": [],
                        "experiments": {}
                    },
                    "voice_response": {
                        "should_listen": false
                    }
                })"));

            UNIT_ASSERT_VALUES_EQUAL(expected, actual);
        }
    }

    Y_UNIT_TEST(AnalyticsInfo) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1}.Build();

        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).Times(1).WillRepeatedly(Return("deadbeef"));

        TResponseBuilderProto storage;
        TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "iot", storage, generator};

        {
            NJson::TJsonValue payload;
            payload["success"] = true;
            builder.AddMeta("iot-info", payload);
        }

        builder.AddAnalyticsAttention("some attention");

        {
            auto skResponse = builder.GetSKRProto();
            const auto actual = SpeechKitResponseToJson(skResponse);

            const auto expected = NJson::ReadJsonFastTree(TStringBuf(R"({
                "header": {
                    "dialog_id": null,
                    "request_id": "",
                    "response_id": "deadbeef",
                    "ref_message_id": "",
                    "session_id": ""
                },
                "response": {
                    "meta": [{
                        "type": "iot-info",
                        "payload": {"success": true}
                    }, {
                        "type": "attention",
                        "attention_type": "some attention"
                    }],
                    "card":null,
                    "cards":[],
                    "directives": [],
                    "experiments": {}
                },
                "voice_response": {
                    "should_listen": false
                }
            })"));

            UNIT_ASSERT_VALUES_EQUAL(expected, actual);
        }

        {
            auto skResponse = builder.GetSKRProto();
            const auto actual = SpeechKitResponseToJson(skResponse);

            const auto expected = NJson::ReadJsonFastTree(TStringBuf(R"({
                "header": {
                    "dialog_id": null,
                    "request_id": "",
                    "response_id": "deadbeef",
                    "ref_message_id": "",
                    "session_id": ""
                },
                "response": {
                    "meta": [{
                        "type": "iot-info",
                        "payload": {"success": true}
                    }, {
                        "type": "attention",
                        "attention_type": "some attention"
                    }],
                    "card":null,
                    "cards":[],
                    "directives": [],
                    "experiments": {}
                },
                "voice_response": {
                    "should_listen": false
                }
            })"));

            UNIT_ASSERT_VALUES_EQUAL(expected, actual);
        }
    }

    Y_UNIT_TEST(Directive) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1}.Build();

        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).Times(1).WillRepeatedly(Return("deadbeef"));

        TResponseBuilderProto storage;
        TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "scenario", storage, generator};

        builder.AddDirective(NMegamind::TDeferApplyDirectiveModel("session payload"));

        auto skResponse = builder.GetSKRProto();
        const auto actual = SpeechKitResponseToJson(skResponse);

        const auto expected = NJson::ReadJsonFastTree(TStringBuf(R"({
            "header": {
                "dialog_id": null,
                "request_id": "",
                "response_id": "deadbeef",
                "ref_message_id": "",
                "session_id": ""
            },
            "response": {
                "card":null,
                "cards":[],
                "directives": [{
                    "name": "defer_apply",
                    "payload": {
                      "session": "session payload"
                    },
                    "type": "uniproxy_action"
                }],
                "experiments": {}
            },
            "voice_response": {
                "should_listen": false
            }
        })"));

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(UpdateDatasyncDirective) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1}.Build();

        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).Times(1).WillRepeatedly(Return("deadbeef"));

        TResponseBuilderProto storage;
        TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "scenario", storage, generator};

        builder.AddDirectiveToVoiceResponse(NMegamind::TUpdateDatasyncDirectiveModel(
            "dir key", "dir value", NMegamind::EUpdateDatasyncMethod::Put
        ));

        auto skResponse = builder.GetSKRProto();
        const auto actual = SpeechKitResponseToJson(skResponse);

        const auto expected = NJson::ReadJsonFastTree(TStringBuf(R"({
            "header": {
                "dialog_id": null,
                "request_id": "",
                "response_id": "deadbeef",
                "ref_message_id": "",
                "session_id": ""
            },
            "response": {
                "card":null,
                "cards":[],
                "directives": [],
                "experiments": {}
            },
            "voice_response": {
                "should_listen": false,
                "directives": [{
                    "name": "update_datasync",
                    "payload": {
                      "key": "dir key",
                      "value": "dir value",
                      "method": "PUT",
                      "listening_is_possible": true
                    },
                    "type": "uniproxy_action"
                }]
            }
        })"));

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(SerDes) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1}.Build();

        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).Times(1).WillRepeatedly(Return("deadbeef"));

        TResponseBuilderProto storage;
        TResponseBuilder expected{skr, CreateRequestFromSkr(skr), "iot", storage, generator};

        expected.AddSimpleText("Hello, World!");

        expected.AddAnalyticsAttention("first attention");
        expected.AddAnalyticsAttention("second attention");

        expected.AddDirective(NMegamind::TDeferApplyDirectiveModel("directivePayload"));

        {
            NJson::TJsonValue payload;
            payload["success"] = true;
            expected.AddMeta("iot-info", payload);
        }

        TResponseBuilderProto proto = expected.ToProto();
        const auto actual = TResponseBuilder::FromProto(skr, CreateRequestFromSkr(skr), proto, generator);

        UNIT_ASSERT(expected == actual);
    }

    Y_UNIT_TEST(Smoke) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1}.Build();
        const TString intentName = "test-intent";

        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).WillRepeatedly(Return("deadbeef"));

        const TStringBuf veryLongString{"yura is a cute bro!!!"};
        const TString text{TStringBuf{veryLongString, 5}.Chop(3)};
        const TString tts{TStringBuf{veryLongString, 5}.Chop(7)};

        {
            // Simple text (same tts and text).
            TResponseBuilderProto storage;
            TResponseBuilder builder{skr, CreateRequestFromSkr(skr), intentName, storage, generator};
            builder.AddSimpleText(text);
            const auto* resp = &builder.GetSKRProto();
            UNIT_ASSERT_VALUES_EQUAL(resp->GetVoiceResponse().GetOutputSpeech().GetText(), text);
            UNIT_ASSERT_VALUES_EQUAL(resp->GetVoiceResponse().GetOutputSpeech().GetType(), "simple");
            UNIT_ASSERT_VALUES_EQUAL(resp->GetResponse().CardsSize(), 1);
            UNIT_ASSERT_VALUES_EQUAL(resp->GetResponse().GetCards(0).GetText(), text);
            UNIT_ASSERT_VALUES_EQUAL(resp->GetResponse().GetCards(0).GetType(), "simple_text");
        }

        {
            // Simple text (diff tts and text).
            TResponseBuilderProto storage;
            TResponseBuilder builder{skr, CreateRequestFromSkr(skr), intentName, storage, generator};
            builder.AddSimpleText(text, tts);
            const auto& resp = builder.GetSKRProto();
            UNIT_ASSERT_VALUES_EQUAL(resp.GetVoiceResponse().GetOutputSpeech().GetText(), tts);
            UNIT_ASSERT_VALUES_EQUAL(resp.GetVoiceResponse().GetOutputSpeech().GetType(), "simple");
            UNIT_ASSERT_VALUES_EQUAL(resp.GetResponse().CardsSize(), 1);
            UNIT_ASSERT_VALUES_EQUAL(resp.GetResponse().GetCards(0).GetText(), text);
            UNIT_ASSERT_VALUES_EQUAL(resp.GetResponse().GetCards(0).GetType(), "simple_text");
        }

        {
            // Error.
            TResponseBuilderProto storage;
            TResponseBuilder builder{skr, CreateRequestFromSkr(skr), intentName, storage, generator};

            builder.AddError("error_type", text);
            const auto& resp = builder.GetSKRProto();
            UNIT_ASSERT_VALUES_EQUAL(resp.GetResponse().MetaSize(), 1);
            UNIT_ASSERT_VALUES_EQUAL(resp.GetResponse().GetMeta(0).GetType(), "error");
            UNIT_ASSERT_VALUES_EQUAL(resp.GetResponse().GetMeta(0).GetErrorType(), "error_type");
            UNIT_ASSERT_VALUES_EQUAL(resp.GetResponse().GetMeta(0).GetMessage(), text);
        }

        {
            // Attention.
            TResponseBuilderProto storage;
            TResponseBuilder builder{skr, CreateRequestFromSkr(skr), intentName, storage, generator};
            builder.AddAnalyticsAttention(text);
            const auto& resp = builder.GetSKRProto();
            UNIT_ASSERT_VALUES_EQUAL(resp.GetResponse().MetaSize(), 1);
            UNIT_ASSERT_VALUES_EQUAL(resp.GetResponse().GetMeta(0).GetType(), "attention");
            UNIT_ASSERT_VALUES_EQUAL(resp.GetResponse().GetMeta(0).GetAttentionType(), text);
        }
    }

    Y_UNIT_TEST(TestDirectives) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1}.Build();

        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).Times(1).WillRepeatedly(Return("deadbeef"));

        TResponseBuilderProto storage;
        TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "TestScenarioName", storage, generator};

        builder.AddDirective(NMegamind::TDeferApplyDirectiveModel("Payload"));
        builder.AddFrontDirective(NMegamind::TUpdateSpaceActionsDirectiveModel());
        auto openDialog = NMegamind::TOpenDialogDirectiveModelBuilder()
                              .SetDialogId("dialog_id")
                              .SetAnalyticsType("OpenDialogDirective")
                              .AddDirective(NMegamind::TOpenDialogDirectiveModel("InnerOpenDialogDirective",
                                                                                          "dialog_id", {}))
                              .Build();
        builder.AddDirective(openDialog);

        auto skResponse = builder.GetSKRProto();
        const auto actual = SpeechKitResponseToJson(skResponse);

        const auto expected = NJson::ReadJsonFastTree(TStringBuf(R"(
        {
          "header": {
            "dialog_id": null,
            "request_id": "",
            "response_id": "deadbeef",
            "ref_message_id": "",
            "session_id": ""
          },
          "response": {
            "card":null,
            "cards":[],
            "directives": [
              {
                "name": "update_space_actions",
                "payload": {},
                "sub_name": "update_space_actions",
                "type": "client_action"
              },
              {
                "name": "defer_apply",
                "payload": {
                  "session": "Payload"
                },
                "type": "uniproxy_action"
              },
              {
                "name": "open_dialog",
                "payload": {
                  "directives": [
                    {
                      "name": "open_dialog",
                      "payload": {
                        "directives": [],
                        "dialog_id": "TestScenarioName:dialog_id"
                      },
                      "sub_name": "InnerOpenDialogDirective",
                      "type": "client_action"
                    }
                  ],
                  "dialog_id": "TestScenarioName:dialog_id"
                },
                "sub_name": "OpenDialogDirective",
                "type": "client_action"
              }
            ],
            "experiments": {}
          },
          "voice_response": {
            "should_listen": false
          }
        }
        )"));

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(TestCards) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1_WITH_REQUEST_ID}.Build();

        TResponseBuilderProto storage;
        TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "ScenarioName", storage, NMegamind::TFakeGuidGenerator("deadbeef")};

        builder.AddCard(NMegamind::TTextCardModel("TextCard"));

        auto card =
            NMegamind::TTextWithButtonCardModelBuilder()
                .SetText("Text")
                .AddButton(NMegamind::TActionButtonModelBuilder()
                               .SetTitle("ButtonTitle")
                               .AddDirective(NMegamind::TCallbackDirectiveModel("CallbackDirective",
                                                                                /* ignoreAnswer= */ true, {},
                                                                                /* isLedSilent= */ true))
                               .Build())
                .Build();
        builder.AddCard(card);

        auto skResponse = builder.GetSKRProto();
        const auto actual = SpeechKitResponseToJson(skResponse);

        const auto expected = NJson::ReadJsonFastTree(TStringBuf(R"(
        {
          "header": {
            "response_id": "deadbeef",
            "request_id": "RequestId",
            "dialog_id": null,
            "ref_message_id": "RefMessageId",
            "session_id": "SessionId"
          },
          "response": {
            "card": {
              "type": "simple_text",
              "text": "TextCard"
            },
            "cards": [
              {
                "type": "simple_text",
                "text": "TextCard"
              },
              {
                "type": "text_with_button",
                "text": "Text",
                "buttons": [
                  {
                    "type": "action",
                    "title": "ButtonTitle",
                    "directives": [
                      {
                        "name": "CallbackDirective",
                        "payload": {
                          "@request_id": "RequestId",
                          "@scenario_name": "ScenarioName"
                        },
                        "ignore_answer": true,
                        "is_led_silent": true,
                        "type": "server_action"
                      }
                    ]
                  }
                ]
              }
            ],
            "experiments": {},
            "directives": []
          },
          "voice_response": {
            "should_listen": false
          }
        }
        )"));

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(TestVoiceResponse) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1_WITH_REQUEST_ID}.Build();

        TResponseBuilderProto storage;
        TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "ScenarioName", storage, NMegamind::TFakeGuidGenerator("deadbeef")};

        builder.AddCard(NMegamind::TTextCardModel("Text"));
        builder.SetOutputSpeech("Voice");

        auto skResponse = builder.GetSKRProto();
        const auto actual = SpeechKitResponseToJson(skResponse);

        const auto expected = NJson::ReadJsonFastTree(TStringBuf(R"(
            {
              "header": {
                "response_id": "deadbeef",
                "request_id": "RequestId",
                "dialog_id": null,
                "ref_message_id": "RefMessageId",
                "session_id": "SessionId"
              },
              "response": {
                "card": {
                  "type": "simple_text",
                  "text": "Text"
                },
                "cards": [
                  {
                    "type": "simple_text",
                    "text": "Text"
                  },
                ],
                "experiments": {},
                "directives": []
              },
              "voice_response": {
                "output_speech": {
                  "text": "Voice",
                  "type": "simple"
                },
                "should_listen": false
              }
            }
        )"));

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(TestNoVoiceResponse) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1_WITHOUT_VOICE_SESSION}.Build();

        TResponseBuilderProto storage;
        TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "ScenarioName", storage, NMegamind::TFakeGuidGenerator("deadbeef")};

        builder.AddCard(NMegamind::TTextCardModel("Text"));
        builder.SetOutputSpeech("Voice");

        auto skResponse = builder.GetSKRProto();
        const auto actual = SpeechKitResponseToJson(skResponse);

        const auto expected = NJson::ReadJsonFastTree(TStringBuf(R"(
            {
              "header": {
                "response_id": "deadbeef",
                "request_id": "RequestId",
                "dialog_id": null,
                "ref_message_id": "",
                "session_id": ""
              },
              "response": {
                "card": {
                  "type": "simple_text",
                  "text": "Text"
                },
                "cards": [
                  {
                    "type": "simple_text",
                    "text": "Text"
                  },
                ],
                "experiments": {},
                "directives": []
              },
              "voice_response": {
                "should_listen": false
              }
            }
        )"));

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(TestDirectivesExecutionPolicy) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1_WITHOUT_VOICE_SESSION}.Build();

        TResponseBuilderProto storage;
        TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "ScenarioName", storage, NMegamind::TFakeGuidGenerator("deadbeef")};

        builder.SetDirectivesExecutionPolicy(EDirectivesExecutionPolicy::BeforeSpeech);

        {
            auto skResponse = builder.GetSKRProto();
            const auto actual = SpeechKitResponseToJson(skResponse);

            UNIT_ASSERT_VALUES_EQUAL(actual["response"]["directives_execution_policy"].GetString(), "BeforeSpeech");
        }

        builder.SetDirectivesExecutionPolicy(EDirectivesExecutionPolicy::AfterSpeech);

        {
            auto skResponse = builder.GetSKRProto();
            const auto actual = SpeechKitResponseToJson(skResponse);

            UNIT_ASSERT_VALUES_EQUAL(actual["response"]["directives_execution_policy"].GetString(), "AfterSpeech");
        }
    }

    Y_UNIT_TEST(TestResponseErrorMessage) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1}.Build();
        const TString intentName = "test-intent";

        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).WillRepeatedly(Return("deadbeef"));

        {
            TResponseBuilderProto storage;
            TResponseBuilder builder{skr, CreateRequestFromSkr(skr), intentName, storage, generator};
            TResponseErrorMessage errorMessage;
            errorMessage.SetType("error_type");
            errorMessage.SetMessage("error message");
            errorMessage.SetLevel(TResponseErrorMessage::Error);
            builder.SetResponseErrorMessage(errorMessage);
            const auto* resp = &builder.GetSKRProto();
            UNIT_ASSERT(resp->GetResponse().GetResponseErrorMessage().GetType() == "error_type");
            UNIT_ASSERT(resp->GetResponse().GetResponseErrorMessage().GetMessage() == "error message");
            UNIT_ASSERT(resp->GetResponse().GetResponseErrorMessage().GetLevel() == TResponseErrorMessage::Error);
        }
    }

    Y_UNIT_TEST(TestBuilderSkipVoiceSession) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1}.Build();

        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).Times(3).WillRepeatedly(Return("deadbeef"));

        {
            TResponseBuilderProto storage;
            TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "scenario", storage, generator};
            UNIT_ASSERT(builder.ToProto().GetShouldAddOutputSpeech());
        }
        {
            TResponseBuilderProto storage;
            TResponseBuilder builder{skr, CreateRequestFromSkr(skr, /* skipVoiceSession= */ false), "scenario", storage, generator};
            UNIT_ASSERT(builder.ToProto().GetShouldAddOutputSpeech());
        }
        {
            TResponseBuilderProto storage;
            TResponseBuilder builder{skr, CreateRequestFromSkr(skr, /* skipVoiceSession= */ true), "scenario", storage, generator};
            UNIT_ASSERT(!builder.ToProto().GetShouldAddOutputSpeech());
        }
    }

    Y_UNIT_TEST(TestBuilderDisableShouldListen) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1}.Build();

        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).Times(3).WillRepeatedly(Return("deadbeef"));

        {
            TResponseBuilderProto storage;
            storage.MutableResponse()->MutableVoiceResponse()->SetShouldListen(true);

            TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "scenario", storage, generator};
            UNIT_ASSERT(builder.ToProto().GetResponse().GetVoiceResponse().GetShouldListen());
            UNIT_ASSERT(!builder.ToProto().HasForceDisableShouldListen());
        }
        {
            TResponseBuilderProto storage;
            storage.MutableResponse()->MutableVoiceResponse()->SetShouldListen(true);

            TResponseBuilder builder{skr, CreateRequestFromSkr(skr, /* skipVoiceSession= */ false, /* disableShouldListen= */ false), "scenario", storage, generator};
            UNIT_ASSERT(builder.ToProto().GetResponse().GetVoiceResponse().GetShouldListen());
            UNIT_ASSERT(!builder.ToProto().HasForceDisableShouldListen());
        }
        {
            TResponseBuilderProto storage;
            storage.MutableResponse()->MutableVoiceResponse()->SetShouldListen(true);

            TResponseBuilder builder{skr, CreateRequestFromSkr(skr, /* skipVoiceSession= */ true, /* disableShouldListen= */ true), "scenario", storage, generator};
            UNIT_ASSERT(!builder.ToProto().GetResponse().GetVoiceResponse().GetShouldListen());
            UNIT_ASSERT(builder.ToProto().GetForceDisableShouldListen());
        }
    }

    Y_UNIT_TEST(TestProtobufUniproxyDirectiveNotSuitableModel) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1}.Build();
        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).WillRepeatedly(Return("deadbeef"));

        TResponseBuilderProto storage;
        TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "reminders", storage, generator};

        builder.AddProtobufUniproxyDirective(NMegamind::TUpdateDatasyncDirectiveModel(
                "dir key", "dir value", NMegamind::EUpdateDatasyncMethod::Put
        ));
        UNIT_ASSERT(builder.ToProto().GetResponse().GetVoiceResponse().GetUniproxyDirectives().empty());
    }

    Y_UNIT_TEST(TestProtobufUniproxyDirectiveAsTRemoveScheduledActionRequest) {
        auto skr = TSpeechKitRequestBuilder{MEGAREQUEST1}.Build();
        NMegamind::TMockGuidGenerator generator;
        EXPECT_CALL(generator, GenerateGuid()).WillRepeatedly(Return("deadbeef"));

        TResponseBuilderProto storage;
        TResponseBuilder builder{skr, CreateRequestFromSkr(skr), "reminders", storage, generator};

        NScenarios::TCancelScheduledActionDirective testDirective;
        testDirective.MutableRemoveScheduledActionRequest()->SetActionId("my-lovely-id");
        auto model = MakeIntrusive<NMegamind::TProtobufUniproxyDirectiveModel>(testDirective);

        builder.AddProtobufUniproxyDirective(*model);

        const auto& voiceResponse = builder.ToProto().GetResponse().GetVoiceResponse();
        UNIT_ASSERT_VALUES_EQUAL(voiceResponse.GetUniproxyDirectives().size(), 1);

        const auto& pud = voiceResponse.GetUniproxyDirectives()[0];
        UNIT_ASSERT(pud.HasContextSaveDirective());
        UNIT_ASSERT_VALUES_EQUAL(pud.GetContextSaveDirective().GetDirectiveId(), "matrix_scheduler_remove_scheduled_action_request");

        NMatrix::NScheduler::NApi::TRemoveScheduledActionRequest respTestDirective;
        pud.GetContextSaveDirective().GetPayload().UnpackTo(&respTestDirective);
        UNIT_ASSERT_VALUES_EQUAL(respTestDirective.GetActionId(), testDirective.GetRemoveScheduledActionRequest().GetActionId());
    }
}

} // namespace
