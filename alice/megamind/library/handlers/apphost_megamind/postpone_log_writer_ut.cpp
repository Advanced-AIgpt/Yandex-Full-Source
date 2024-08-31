#include "postpone_log_writer.h"

#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NImpl;

constexpr TStringBuf TEST_MEGAMIND_RESPONSE = R"({
    "header": {
        "dialog_id": null,
        "ref_message_id": "48418836-fa1e-47bf-a5e3-445fa587024f",
        "request_id": "E9F0407C-E4EF-47C3-A2DC-2C674152F55F",
        "response_id": "84f631e9-4b63e83c-d992f5d9-84a6b111",
        "sequence_number": 356,
        "session_id": "21ab5218-a8a7-441d-998d-f70bd9b6a6bf"
    },
    "response": {
        "card": {
            "buttons": [
                {
                    "directives": [
                        {
                            "name": "open_uri",
                            "payload": {
                                "uri": "pedometer://home"
                            },
                            "sub_name": "open_uri",
                            "type": "client_action"
                        },
                        {
                            "ignore_answer": true,
                            "is_led_silent": true,
                            "name": "on_suggest",
                            "payload": {
                                "@request_id": "E9F0407C-E4EF-47C3-A2DC-2C674152F55F",
                                "@scenario_name": "Vins",
                                "button_id": "ee3213db-a110326d-54da06f-60c8f936",
                                "caption": "Открыть",
                                "request_id": "E9F0407C-E4EF-47C3-A2DC-2C674152F55F",
                                "scenario_name": "OpenAppsFixlist"
                            },
                            "type": "server_action"
                        }
                    ],
                    "title": "Открыть",
                    "type": "action"
                }
            ],
            "text": "Открываю Шагомер",
            "type": "text_with_button"
        },
        "cards": [
            {
                "buttons": [
                    {
                        "directives": [
                            {
                                "name": "open_uri",
                                "payload": {
                                    "uri": "pedometer://home"
                                },
                                "sub_name": "open_uri",
                                "type": "client_action"
                            },
                            {
                                "ignore_answer": true,
                                "is_led_silent": true,
                                "name": "on_suggest",
                                "payload": {
                                    "@request_id": "E9F0407C-E4EF-47C3-A2DC-2C674152F55F",
                                    "@scenario_name": "Vins",
                                    "button_id": "ee3213db-a110326d-54da06f-60c8f936",
                                    "caption": "Открыть",
                                    "request_id": "E9F0407C-E4EF-47C3-A2DC-2C674152F55F",
                                    "scenario_name": "OpenAppsFixlist"
                                },
                                "type": "server_action"
                            }
                        ],
                        "title": "Открыть",
                        "type": "action"
                    }
                ],
                "text": "Открываю Шагомер",
                "type": "text_with_button"
            }
        ],
        "directives": [
            {
                "name": "open_uri",
                "payload": {
                    "uri": "pedometer://home"
                },
                "sub_name": "open_uri",
                "type": "client_action"
            }
        ],
        "directives_execution_policy": "BeforeSpeech",
        "experiments": {
            "mm_kek": "yes"
        },
        "quality_storage": {
            "post_predicts": {
                "GeneralConversation": -4.90112257,
                "OpenAppsFixlist": 0
            },
            "post_win_reason": "WR_PRIORITY",
            "pre_predicts": {
                "GeneralConversation": -7.07022667,
                "HardcodedResponse": 0,
                "HollywoodMusic": -6.11070728,
                "IoT": 0,
                "MordoviaVideoSelection": 0,
                "OpenAppsFixlist": 0,
                "Search": -2.24946117,
                "SkillDiscoveryGc": 0,
                "Vins": 1.98584569
            },
            "scenarios_information": {
                "AddPointTr": {
                    "reason": "LR_NOT_ALLOWED"
                },
                "Alarm": {
                    "reason": "LR_NOT_ALLOWED"
                }
            }
        },
        "templates": {}
    },
    "sessions": {
        "": "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"
    },
    "version": "vins/stable-188-6@8901369",
    "voice_response": {
        "output_speech": {
            "text": "Открываю",
            "type": "simple"
        },
        "should_listen": false
    }
})";


constexpr TStringBuf TEST_MEGAMIND_RESPONSE_OBFUSCATED = R"({
    "header": {
        "dialog_id": null,
        "ref_message_id": "48418836-fa1e-47bf-a5e3-445fa587024f",
        "request_id": "E9F0407C-E4EF-47C3-A2DC-2C674152F55F",
        "response_id": "84f631e9-4b63e83c-d992f5d9-84a6b111",
        "sequence_number": 356,
        "session_id": "21ab5218-a8a7-441d-998d-f70bd9b6a6bf"
    },
    "sessions": "**OBFUSCATED**",
    "response": {
        "quality_storage": {
            "post_predicts": {
                "GeneralConversation": -4.90112257,
                "OpenAppsFixlist": 0
            },
            "post_win_reason": "WR_PRIORITY",
            "pre_predicts": {
                "GeneralConversation": -7.07022667,
                "HardcodedResponse": 0,
                "HollywoodMusic": -6.11070728,
                "IoT": 0,
                "MordoviaVideoSelection": 0,
                "OpenAppsFixlist": 0,
                "Search": -2.24946117,
                "SkillDiscoveryGc": 0,
                "Vins": 1.98584569
            },
            "scenarios_information": "**OBFUSCATED**"
        },
        "card": {
            "buttons": [
                {
                    "directives": [
                        {
                            "name": "open_uri",
                            "payload": {
                                "uri": "pedometer://home"
                            },
                            "sub_name": "open_uri",
                            "type": "client_action"
                        },
                        {
                            "ignore_answer": true,
                            "is_led_silent": true,
                            "name": "on_suggest",
                            "payload": {
                                "@request_id": "E9F0407C-E4EF-47C3-A2DC-2C674152F55F",
                                "@scenario_name": "Vins",
                                "button_id": "ee3213db-a110326d-54da06f-60c8f936",
                                "caption": "Открыть",
                                "request_id": "E9F0407C-E4EF-47C3-A2DC-2C674152F55F",
                                "scenario_name": "OpenAppsFixlist"
                            },
                            "type": "server_action"
                        }
                    ],
                    "title": "Открыть",
                    "type": "action"
                }
            ],
            "text": "Открываю Шагомер",
            "type": "text_with_button"
        },
        "cards": [
            {
                "buttons": [
                    {
                        "directives": [
                            {
                                "name": "open_uri",
                                "payload": {
                                    "uri": "pedometer://home"
                                },
                                "sub_name": "open_uri",
                                "type": "client_action"
                            },
                            {
                                "ignore_answer": true,
                                "is_led_silent": true,
                                "name": "on_suggest",
                                "payload": {
                                    "@request_id": "E9F0407C-E4EF-47C3-A2DC-2C674152F55F",
                                    "@scenario_name": "Vins",
                                    "button_id": "ee3213db-a110326d-54da06f-60c8f936",
                                    "caption": "Открыть",
                                    "request_id": "E9F0407C-E4EF-47C3-A2DC-2C674152F55F",
                                    "scenario_name": "OpenAppsFixlist"
                                },
                                "type": "server_action"
                            }
                        ],
                        "title": "Открыть",
                        "type": "action"
                    }
                ],
                "text": "Открываю Шагомер",
                "type": "text_with_button"
            }
        ],
        "directives": [
            {
                "name": "open_uri",
                "payload": {
                    "uri": "pedometer://home"
                },
                "sub_name": "open_uri",
                "type": "client_action"
            }
        ],
        "directives_execution_policy": "BeforeSpeech",
        "experiments": {
            "mm_kek": "yes"
        },
        "templates": {}
    },
    "version": "vins/stable-188-6@8901369",
    "voice_response": {
        "output_speech": {
            "text": "Открываю",
            "type": "simple"
        },
        "should_listen": false
    }
})";

NJson::TJsonValue RetrieveJsonFromLog(const TString& logString) {
    auto firstBracketPos = logString.find_first_of('{');
    auto jsonSubstr = logString.substr(firstBracketPos, logString.size() - firstBracketPos);
    return JsonFromString(jsonSubstr);
}

Y_UNIT_TEST_SUITE(PostponeLogWriter) {
    Y_UNIT_TEST(TestLogResponseFullByPriority) {
        TStringStream outputStream;
        NRTLog::TClient rtLogClient("/dev/null", "null");
        TLog outputLog(MakeHolder<TStreamLogBackend>(&outputStream));
        TFakeThreadPool loggingThread;
        TFakeThreadPool serializers;
        TRTLogger logger{loggingThread, serializers, rtLogClient.CreateRequestLogger(), ELogPriority::TLOG_DEBUG,
                         &outputLog};
        LogResponse(JsonFromString(TEST_MEGAMIND_RESPONSE), /* haveErrorsInResponse= */ false, logger, 200);
        UNIT_ASSERT_VALUES_EQUAL(JsonFromString(TEST_MEGAMIND_RESPONSE), RetrieveJsonFromLog(outputStream.Str()));
    }
    Y_UNIT_TEST(TestLogResponseFullByErrors) {
        TStringStream outputStream;
        NRTLog::TClient rtLogClient("/dev/null", "null");
        TLog outputLog(MakeHolder<TStreamLogBackend>(&outputStream));
        TFakeThreadPool loggingThread;
        TFakeThreadPool serializers;
        TRTLogger logger{loggingThread, serializers, rtLogClient.CreateRequestLogger(), ELogPriority::TLOG_INFO,
                         &outputLog};
        LogResponse(JsonFromString(TEST_MEGAMIND_RESPONSE), /* haveErrorsInResponse= */ true, logger, 200);
        UNIT_ASSERT_VALUES_EQUAL(JsonFromString(TEST_MEGAMIND_RESPONSE), RetrieveJsonFromLog(outputStream.Str()));
    }
    Y_UNIT_TEST(TestLogResponseObfuscated) {
        TStringStream outputStream;
        NRTLog::TClient rtLogClient("/dev/null", "null");
        TLog outputLog(MakeHolder<TStreamLogBackend>(&outputStream));
        TFakeThreadPool loggingThread;
        TFakeThreadPool serializers;
        TRTLogger logger{loggingThread, serializers, rtLogClient.CreateRequestLogger(), ELogPriority::TLOG_INFO,
                         &outputLog};
        LogResponse(JsonFromString(TEST_MEGAMIND_RESPONSE), /* haveErrorsInResponse= */ false, logger, 200);
        UNIT_ASSERT_VALUES_EQUAL(JsonFromString(TEST_MEGAMIND_RESPONSE_OBFUSCATED), RetrieveJsonFromLog(outputStream.Str()));
    }
}

} // namespace
