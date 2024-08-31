#include "postroll.h"

#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/testing/modifier_fixture.h>

namespace {

using namespace NAlice::NMegamind;

// Common data -----------------------------------------------------------------
constexpr auto DEFAULT_STORAGE = TStringBuf(R"(
{
    "Proactivity": {
        "LastShowTime": "1336",
        "ShownPhrasesMap": {"2": "1"},
        "LastShownActionFrameName": "alice.external_skill_activate"
    }
}
)");

constexpr TStringBuf INTENT_FAST_CANCEL = "personal_assistant.handcrafted.fast_cancel";
constexpr TStringBuf INTENT_GET_TIME = "personal_assistant.scenarios.get_time";
constexpr TStringBuf INTENT_HELLO = "personal_assistant.handcrafted.hello";

// Postroll modifier data ------------------------------------------------------

constexpr TStringBuf EXPECTED_TEXT_RESPONSE_WITHOUT_POSTROLL = "Таков путь.";

constexpr TStringBuf EXPECTED_ACTION_NAME_POSTROLL_ADD_SKILL_ACTION = "postroll_action";
constexpr auto EXPECTED_ACTION_BODY_POSTROLL_ADD_SKILL_ACTION = TStringBuf(R"(
{
    "nlu_hint": {
        "frame_name": "alice.proactivity.confirm"
    },
    "frame": {
        "name": "alice.external_skill_activate",
        "slots": [{
            "name": "activation_phrase",
            "type": "string",
            "value": "skill name"
        }]
    }
}
)");

// Service Postroll modifier data ----------------------------------------------
constexpr auto SKR_SERVICE_POSTROLL = TStringBuf(R"(
{
    "application": {
        "timestamp": "1337",
        "uuid": "test-uuid"
    },
    "header": {
        "request_id": "test-reqid"
    },
    "request": {
        "voice_session": true,
        "event": {
            "type": "text_input"
        }
    }
}
)");

constexpr auto SKR_SERVICE_POSTROLL_ENABLE_EMOTIONAL = TStringBuf(R"(
{
    "application": {
        "timestamp": "1337",
        "uuid": "test-uuid"
    },
    "header": {
        "request_id": "test-reqid"
    },
    "request": {
        "experiments": {
            "mm_proactivity_disable_memento": "1",
            "mm_proactivity_enable_emotional_tts": "1"
        },
        "voice_session": true,
        "event": {
            "type": "text_input"
        }
    }
}
)");

constexpr auto SKR_SERVICE_POSTROLL_ENABLE_NOTIFICATION_SOUND = TStringBuf(R"(
{
    "application": {
        "timestamp": "1337",
        "uuid": "test-uuid"
    },
    "header": {
        "request_id": "test-reqid"
    },
    "request": {
        "experiments": {
            "mm_proactivity_disable_memento": "1",
            "mm_proactivity_enable_notification_sound": "1"
        },
        "voice_session": true,
        "event": {
            "type": "text_input"
        }
    }
}
)");

constexpr TStringBuf PERSONAL_DATA = R"(
{
    "LastShowTime": "0",
    "ShownPhrasesMap": {"2": "0"},
    "LastShownActionFrameName": "alice.external_skill_activate"
}
)";

constexpr auto SKR_SERVICE_POSTROLL_SAVE_MEMENTO = TStringBuf(R"(
{
    "application": {
        "timestamp": "1337",
        "uuid": "test-uuid"
    },
    "header": {
        "request_id": "test-reqid"
    },
    "request": {
        "voice_session": true,
        "event": {
            "type": "text_input"
        }
    }
}
)");

constexpr auto MEMENTO_USER_CONFIGS = TStringBuf(R"(
{
    "ProactivityConfig": {
        "Storage": {
            "LastShowTime": "0",
            "ShownPhrasesMap": {"2": "0"},
            "LastShownActionFrameName": "alice.external_skill_activate"
        }
    }
}
)");

constexpr auto SKR_SERVICE_POSTROLL_TIME_PASSED = TStringBuf(R"(
{
    "application": {
        "timestamp": "1337",
        "uuid": "test-uuid",
        "app_id": "aliced"
    },
    "header": {
        "request_id": "test-reqid"
    },
    "request": {
        "experiments": {
            "mm_proactivity_storage_update_time_delta=1": "1"
        },
        "voice_session": true,
        "event": {
            "type": "text_input"
        }
    }
}
)");

constexpr auto SKR_SERVICE_POSTROLL_NOT_ENOUGH_TIME_PASSED = TStringBuf(R"(
{
    "application": {
        "timestamp": "1337",
        "uuid": "test-uuid",
        "app_id": "aliced"
    },
    "header": {
        "request_id": "test-reqid"
    },
    "request": {
        "experiments": {
            "mm_proactivity_storage_update_time_delta=10000": "1"
        },
        "voice_session": true,
        "event": {
            "type": "text_input"
        }
    }
}
)");

constexpr auto SKR_SERVICE_POSTROLL_SERVER_ACTION = TStringBuf(R"(
{
    "application": {
        "timestamp": "1337",
        "uuid": "test-uuid",
        "app_id": "aliced"
    },
    "header": {
        "request_id": "test-reqid"
    },
    "request": {
        "experiments": {
            "mm_proactivity_storage_update_time_delta=1": "1"
        },
        "voice_session": true,
        "event": {
            "type": "server_action"
        }
    }
}
)");

constexpr auto SKR_SERVICE_POSTROLL_SEARCHAPP = TStringBuf(R"(
{
    "application": {
        "timestamp": "1337",
        "uuid": "test-uuid",
        "app_id": "ru.yandex.searchplugin"
    },
    "header": {
        "request_id": "test-reqid"
    },
    "request": {
        "experiments": {
            "mm_proactivity_storage_update_time_delta=1": "1"
        },
        "voice_session": true,
        "event": {
            "type": "text_input"
        }
    }
}
)");

constexpr auto SKR_SERVICE_POSTROLL_SEARCHAPP_ENABLE_ALL_APPS = TStringBuf(R"(
{
    "application": {
        "timestamp": "1337",
        "uuid": "test-uuid",
        "app_id": "ru.yandex.searchplugin"
    },
    "header": {
        "request_id": "test-reqid"
    },
    "request": {
        "experiments": {
            "mm_proactivity_storage_update_time_delta=1": "1",
            "mm_proactivity_service_all_apps": "1"
        },
        "voice_session": true,
        "event": {
            "type": "text_input"
        }
    }
}
)");

constexpr auto SKR_SERVICE_POSTROLL_NO_USER_VOICE = TStringBuf(R"(
{
    "application": { "timestamp": "1337" },
    "request": {
        "experiments": {
            "mm_proactivity_disable_memento": "1"
        },
        "voice_session": false,
        "event": {
            "type": "text_input"
        }
    }
}
)");

constexpr auto SKILL_REC_PROACTIVITY = TStringBuf(R"(
{
    "Item": {
        "Id": "42",
        "Tags": ["music", "alarm"],
        "Analytics": {
            "Info": "awesome postroll",
            "SuccessConditions": [{
                "Frame": {
                    "name": "frame_name",
                    "slots": [
                        {"name": "name_1", "value": "value_1"},
                        {"name": "name_2", "value": "value_2"},
                    ]
                },
                "IsTrigger": true
            }]
        },
        "Result": {
            "ShouldListen": true,
            "Postroll": {
                "Voice": "service tts"
            }
        }
    }
}
)");

constexpr auto SKILL_REC_PROACTIVITY_WITH_TEXT = TStringBuf(R"(
{
    "Item": {
        "Id": "42",
        "Tags": ["music", "alarm"],
        "Analytics": {
            "Info": "awesome postroll",
            "SuccessConditions": [{
                "Frame": {
                    "name": "frame_name",
                    "slots": [
                        {"name": "name_1", "value": "value_1"},
                        {"name": "name_2", "value": "value_2"},
                    ]
                },
                "IsTrigger": true
            }]
        },
        "Result": {
            "ShouldListen": true,
            "Postroll": {
                "Voice": "service tts",
                "Text": "service tts"
            }
        }
    }
}
)");

constexpr auto SKILL_REC_PROACTIVITY_NOT_LISTENING_CONDITION = TStringBuf(R"(
{
    "Item": {
        "Tags": ["music", "alarm"],
        "Analytics": {
            "Info": "awesome postroll"
        },
        "Result": {
            "ShouldListen": true,
            "Postroll": {
                "Voice": "service tts not listening"
            }
        }
    },
    "Conditions": [{
        "Listening": {
            "value": false
        }
    }],
    "Score": 0.05
}
)");

constexpr auto SKILL_REC_PROACTIVITY_NOT_LISTENING_CONDITION_HIGH_SCORE = TStringBuf(R"(
{
    "Item": {
        "Tags": ["music", "alarm"],
        "Analytics": {
            "Info": "awesome postroll"
        },
        "Result": {
            "ShouldListen": true,
            "Postroll": {
                "Voice": "service tts not listening high score"
            }
        }
    },
    "Conditions": [{
        "Listening": {
            "value": false
        }
    }],
    "Score": 0.5
}
)");

constexpr auto SKILL_REC_PROACTIVITY_LISTENING_CONDITION = TStringBuf(R"(
{
    "Item": {
        "Tags": ["music", "alarm"],
        "Analytics": {
            "Info": "awesome postroll"
        },
        "Result": {
            "ShouldListen": true,
            "Postroll": {
                "Voice": "service tts listening"
            }
        }
    },
    "Conditions": [{
        "Listening": {
            "value": true
        }
    }]
}
)");

constexpr auto SKILL_REC_PROACTIVITY_HAS_NOT_STACK_ENGINE_GET_NEXT_CONDITION = TStringBuf(R"(
{
    "Item": {
        "Tags": ["music", "alarm"],
        "Analytics": {
            "Info": "awesome postroll"
        },
        "Result": {
            "ShouldListen": true,
            "Postroll": {
                "Voice": "service tts"
            }
        }
    },
    "Conditions": [{
        "Directive": {
            "Name": "@@mm_stack_engine_get_next",
            "Status": IsAbsent
        }
    }]
}
)");

constexpr auto SKILL_REC_PROACTIVITY_HAS_STACK_ENGINE_GET_NEXT_CONDITION = TStringBuf(R"(
{
    "Item": {
        "Tags": ["music", "alarm"],
        "Analytics": {
            "Info": "awesome postroll"
        },
        "Result": {
            "ShouldListen": true,
            "Postroll": {
                "Voice": "service tts"
            }
        }
    },
    "Conditions": [{
        "Directive": {
            "Name": "@@mm_stack_engine_get_next"
        }
    }]
}
)");

constexpr auto SKILL_REC_PROACTIVITY_NO_MATTER_THE_TEXT_CONDITION = TStringBuf(R"(
{
    "Item": {
        "Tags": ["music", "alarm"],
        "Analytics": {
            "Info": "awesome postroll"
        },
        "Result": {
            "ShouldListen": true,
            "Postroll": {
                "Voice": "service tts",
                "Text": "service tts"
            }
        }
    },
    "Conditions": [{
        "VoiceInResponse": IsPresent,
        "TextInResponse": Ignore
    }]
}
)");

constexpr auto SKILL_REC_PROACTIVITY_NO_MATTER_THE_VOICE_CONDITION = TStringBuf(R"(
{
    "Item": {
        "Tags": ["music", "alarm"],
        "Analytics": {
            "Info": "awesome postroll"
        },
        "Result": {
            "ShouldListen": true,
            "Postroll": {
                "Voice": "service tts"
            }
        }
    },
    "Conditions": [{
        "VoiceInResponse": Ignore,
        "TextInResponse": IsPresent
    }
}
)");

constexpr auto SKILL_REC_PROACTIVITY_MULTIPLE_CONDITIONS = TStringBuf(R"(
{
    "Item": {
        "Tags": ["music", "alarm"],
        "Analytics": {
            "Info": "awesome postroll"
        },
        "Result": {
            "ShouldListen": true,
            "Postroll": {
                "Voice": "service tts"
            }
        }
    },
    "Conditions": [{
        "VoiceInResponse": IsPresent
    }, {
        "TextInResponse": IsPresent
    }]
}
)");

constexpr auto SKILL_REC_PROACTIVITY_FRAME_ACTION = TStringBuf(R"(
{
    "Item": {
        "Tags": ["music", "alarm"],
        "Analytics": {
            "Info": "awesome postroll"
        },
        "Result": {
            "ShouldListen": true,
            "FrameAction": {
                "Frame": {
                    "Name": "alice.external_skill_activate",
                    "Slots": [{
                        "Name": "activation_phrase",
                        "Type": "string",
                        "Value": "skill name"
                    }]
                }
            },
            "Postroll": {
                "Voice": "service tts"
            }
        }
    }
}
)");

constexpr auto SKILL_REC_PROACTIVITY_MARKETING_POSTROLL = TStringBuf(R"(
{
    "Item": {
        "Id": "42",
        "Tags": ["music", "alarm"],
        "Analytics": {
            "Info": "awesome postroll",
            "SuccessConditions": [{
                "Frame": {
                    "name": "frame_name",
                    "slots": [
                        {"name": "name_1", "value": "value_1"},
                        {"name": "name_2", "value": "value_2"},
                    ]
                },
                "IsTrigger": true
            }]
        },
        "Result": {
            "ShouldListen": true,
            "Postroll": {
                "Voice": "service tts"
            }
        },
        "MarketingScore": 1.0
    }
}
)");

constexpr TStringBuf EXPECTED_TEXT_SERVICE_POSTROLL_WITHOUT_RESPONSE = "service tts";
constexpr auto EXPECTED_TTS_SERVICE_POSTROLL = TStringBuf("Таков путь. <speaker voice=\"shitova\" emotion=\"neutral\">service tts");
constexpr auto EXPECTED_TTS_SERVICE_POSTROLL_ENABLE_EMOTIONAL = TStringBuf("Таков путь. <speaker voice=\"shitova\">service tts");
constexpr auto EXPECTED_TTS_SERVICE_POSTROLL_ENABLE_NOTIFICATION_SOUND = TStringBuf("Таков путь. <speaker audio=\"postroll_notification_sound.opus\"> <speaker voice=\"shitova\" emotion=\"neutral\">service tts");
constexpr auto EXPECTED_TTS_SERVICE_POSTROLL_LISTENING = TStringBuf("Таков путь. <speaker voice=\"shitova\" emotion=\"neutral\">service tts listening");
constexpr auto EXPECTED_TTS_SERVICE_POSTROLL_NOT_LISTENING = TStringBuf("Таков путь. <speaker voice=\"shitova\" emotion=\"neutral\">service tts not listening");
constexpr auto EXPECTED_TTS_SERVICE_POSTROLL_NOT_LISTENING_HIGH_SCORE = TStringBuf("Таков путь. <speaker voice=\"shitova\" emotion=\"neutral\">service tts not listening high score");
constexpr auto EXPECTED_TTS_SERVICE_POSTROLL_NO_VOICE_RESPONSE = TStringBuf("<speaker voice=\"shitova\" emotion=\"neutral\">service tts");

constexpr auto EXPECTED_LOG_STORAGE_SERVICE_POSTROLL = TStringBuf(R"(
Actions: {
    Type: 2
    Id: "test-uuid"
    ToType: 1
    ToId: "42"
    ActionType: 0
    Value: 1
    Timestamp: 1337
    RequestId: "test-reqid"
    Payload {
        [NDJ.NAS.TActionPayloadData.AlicePayloadExtension] {
            Tags: "music"
            Tags: "alarm"
        }
    }
}
Analytics: {
    Info: "awesome postroll"
    SuccessConditions: [{
        Frame: {
            Name: "frame_name",
            Slots: {Name: "name_1" Value: "value_1"}
            Slots: {Name: "name_2" Value: "value_2"}
        }
        IsTrigger: true
    }]
}
)");

constexpr auto SKILL_REC_PROACTIVITY_SAVE_CONDITION = TStringBuf(R"(
{
    "Item": {
        "Id": "42",
        "Tags": ["music"],
        "Analytics": {
            "Info": "awesome postroll",
            "SuccessConditions": [{
                "Frame": {
                    "name": "frame_name",
                    "slots": [
                        {"name": "name_1", "value": "value_1"}
                    ]
                },
                "IsTrigger": true
            }]
        },
        "Result": {
            "ShouldListen": true,
            "Postroll": {
                "Voice": "service tts"
            }
        }
    }
}
)");
constexpr auto EXPECTED_SERVICE_SAVE_CONDITION_DATA_MM = TStringBuf(R"(
{
    "response": {
        "cards": [{
            "type": "simple_text",
            "text": "Таков путь."
        }]
    },
    "voice_response": {
        "output_speech": {
            "type": "simple",
            "text": "Таков путь. <speaker voice=\"shitova\" emotion=\"neutral\">service tts"
        },
        "should_listen": true,
        "directives": [{
            "name": "update_memento",
            "type": "uniproxy_action",
            "payload": {
                "user_objects": "CpkCCAgSlAIKRHR5cGUuZ29vZ2xlYXBpcy5jb20vcnUueWFuZGV4LmFsaWNlLm1lbWVudG8ucHJvdG8uVFByb2FjdGl2aXR5Q29uZmlnEssBCsgBCLkKMgQIAhAAOAFCEAoFbXVzaWMSBwi5ChABGAFSHWFsaWNlLmV4dGVybmFsX3NraWxsX2FjdGl2YXRlWncKAjQyEjcKEGF3ZXNvbWUgcG9zdHJvbGwaIwofCgpmcmFtZV9uYW1lEhEKBm5hbWVfMRoHdmFsdWVfMTABGgVtdXNpYyIlcGVyc29uYWxfYXNzaXN0YW50LnNjZW5hcmlvcy5nZXRfdGltZTIKdGVzdC1yZXFpZGIKdGVzdC1yZXFpZGgBcAF4uQo="
            }
        }, {
            "name": "update_datasync",
            "type": "uniproxy_action",
            "payload": {
                "key": "/v1/personality/profile/alisa/kv/proactivity_history",
                "listening_is_possible": true,
                "method": "PUT",
                "value": "{}"
            }
        }]
    }
}
)");
constexpr auto EXPECTED_SERVICE_SAVE_CONDITION_PLUS_MEMENTO_DATA_MM = TStringBuf(R"(
{
    "response": {
        "cards": [{
            "type": "simple_text",
            "text": "Таков путь."
        }]
    },
    "voice_response": {
        "output_speech": {
            "type": "simple",
            "text": "Таков путь. <speaker voice=\"shitova\" emotion=\"neutral\">service tts"
        },
        "should_listen": true,
        "directives": [{
            "name": "update_memento",
            "type": "uniproxy_action",
            "payload": {
                "user_objects": "CvQBCAgS7wEKRHR5cGUuZ29vZ2xlYXBpcy5jb20vcnUueWFuZGV4LmFsaWNlLm1lbWVudG8ucHJvdG8uVFByb2FjdGl2aXR5Q29uZmlnEqYBCqMBCLkKOAFCEAoFbXVzaWMSBwi5ChABGAFadwoCNDISNwoQYXdlc29tZSBwb3N0cm9sbBojCh8KCmZyYW1lX25hbWUSEQoGbmFtZV8xGgd2YWx1ZV8xMAEaBW11c2ljIiVwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLmdldF90aW1lMgp0ZXN0LXJlcWlkYgp0ZXN0LXJlcWlkaAFwAXi5Cg=="
            }
        }, {
            "name": "update_datasync",
            "type": "uniproxy_action",
            "payload": {
                "key": "/v1/personality/profile/alisa/kv/proactivity_history",
                "listening_is_possible": true,
                "method": "PUT",
                "value": "{}"
            }
        }]
    }
}
)");
constexpr auto EXPECTED_SERVICE_SAVE_CONDITION_PLUS_MEMENTO_READ_DATA_MM = TStringBuf(R"(
{
    "response": {
        "cards": [{
            "type": "simple_text",
            "text": "Таков путь."
        }]
    },
    "voice_response": {
        "output_speech": {
            "type": "simple",
            "text": "Таков путь. <speaker voice=\"shitova\" emotion=\"neutral\">service tts"
        },
        "should_listen": true,
        "directives": [{
            "name": "update_memento",
            "type": "uniproxy_action",
            "payload": {
                "user_objects": "CpkCCAgSlAIKRHR5cGUuZ29vZ2xlYXBpcy5jb20vcnUueWFuZGV4LmFsaWNlLm1lbWVudG8ucHJvdG8uVFByb2FjdGl2aXR5Q29uZmlnEssBCsgBCLkKMgQIAhAAOAFCEAoFbXVzaWMSBwi5ChABGAFSHWFsaWNlLmV4dGVybmFsX3NraWxsX2FjdGl2YXRlWncKAjQyEjcKEGF3ZXNvbWUgcG9zdHJvbGwaIwofCgpmcmFtZV9uYW1lEhEKBm5hbWVfMRoHdmFsdWVfMTABGgVtdXNpYyIlcGVyc29uYWxfYXNzaXN0YW50LnNjZW5hcmlvcy5nZXRfdGltZTIKdGVzdC1yZXFpZGIKdGVzdC1yZXFpZGgBcAF4uQo="
            }
        }, {
            "name": "update_datasync",
            "type": "uniproxy_action",
            "payload": {
                "key": "/v1/personality/profile/alisa/kv/proactivity_history",
                "listening_is_possible": true,
                "method": "PUT",
                "value": "{}"
            }
        }]
    }
}
)");
constexpr auto EXPECTED_SERVICE_SAVE_CAUSE_TIME_PASSED_MM = TStringBuf(R"(
{
    "response": {
        "cards": [{
            "type": "simple_text",
            "text": "Таков путь."
        }]
    },
    "voice_response": {
        "output_speech": {
            "type": "simple",
            "text": "Таков путь."
        },
        "should_listen": false,
        "directives": [{
            "name": "update_memento",
            "type": "uniproxy_action",
            "payload": {
                "user_objects": "ClMICBJPCkR0eXBlLmdvb2dsZWFwaXMuY29tL3J1LnlhbmRleC5hbGljZS5tZW1lbnRvLnByb3RvLlRQcm9hY3Rpdml0eUNvbmZpZxIHCgVoAXi5Cg=="
            }
        }, {
            "name": "update_datasync",
            "type": "uniproxy_action",
            "payload": {
                "key": "/v1/personality/profile/alisa/kv/proactivity_history",
                "listening_is_possible": true,
                "method": "PUT",
                "value": "{}"
            }
        }]
    }
}
)");
constexpr auto EXPECTED_SERVICE_SAVE_CAUSE_ENABLE_ALL_APPS = TStringBuf(R"(
{
    "response": {
        "cards": [{
            "type": "simple_text",
            "text": "Таков путь."
        }]
    },
    "voice_response": {
        "output_speech": {
            "type": "simple",
            "text": "Таков путь."
        },
        "should_listen": false,
        "directives": [{
            "name": "update_memento",
            "type": "uniproxy_action",
            "payload": {
                "user_objects": "ClMICBJPCkR0eXBlLmdvb2dsZWFwaXMuY29tL3J1LnlhbmRleC5hbGljZS5tZW1lbnRvLnByb3RvLlRQcm9hY3Rpdml0eUNvbmZpZxIHCgVoAXi5Cg=="
            }
        }, {
            "name": "update_datasync",
            "type": "uniproxy_action",
            "payload": {
                "key": "/v1/personality/profile/alisa/kv/proactivity_history",
                "listening_is_possible": true,
                "method": "PUT",
                "value": "{}"
            }
        }]
    }
}
)");
constexpr auto EXPECTED_SERVICE_DO_NOT_SAVE_MM = TStringBuf(R"(
{
    "response": {
        "cards": [{
            "type": "simple_text",
            "text": "Таков путь."
        }]
    },
    "voice_response": {
        "output_speech": {
            "type": "simple",
            "text": "Таков путь."
        },
        "should_listen": false
    }
}
)");



constexpr auto SKR_STOP_ACTION = TStringBuf(R"(
{
    "application": {
        "timestamp": "144000",
        "uuid": "test-uuid"
    },
    "header": {
        "request_id": "test-reqid",
        "prev_req_id": "req-id-with-postroll"
    },
    "request": {
        "voice_session": true,
        "event": {
            "type": "text_input"
        }
    }
}
)");
constexpr auto PERSONAL_DATA_STOP_ACTION = TStringBuf(R"(
{
    "LastPostrollViews": [
        {
            "ItemId": "item-id",
            "Analytics": {
                "Info": "postroll_info",
                "SuccessConditions": [{
                    "Frame": {
                        "name": "postroll_intent"
                    }
                }]
            },
            "Tags": [ "tag_1" ],
            "Source": "postroll_source",
            "Context": { "App" : "aliced", "Features" : ["change_alarm_sound"] }
        }
    ],
    "LastPostrollRequestId": "req-id-with-postroll"
}
)");
constexpr auto PERSONAL_DATA_STOP_ACTION_DIFFERENT_REQID = TStringBuf(R"(
{
    "LastPostrollViews": [
        {
            "ItemId": "item-id",
            "Analytics": {
                "Info": "postroll_info",
                "SuccessConditions": [{
                    "Frame": {
                        "name": "postroll_intent"
                    }
                }]
            },
            "Tags": [ "tag_1" ],
            "Source": "postroll_source"
        }
    ],
    "LastPostrollRequestId": "different-from-prev-req-id"
}
)");
constexpr auto EXPECTED_LOG_STORAGE_STOP_ACTION = TStringBuf(R"(
Actions: {
    Type: 2
    Id: "test-uuid"
    ToType: 1
    ToId: "item-id"
    ActionType: 4
    Value: 1
    Timestamp: 144000
    RequestId: "test-reqid"
    Payload {
        [NDJ.NAS.TActionPayloadData.AlicePayloadExtension] {
            Tags: "tag_1"
            Source: "postroll_source"
            Context: {
                App: "aliced"
                Features: "change_alarm_sound"
            }
            PostrollViewRequestId: "req-id-with-postroll"
        }
    }
}
)");
constexpr auto EXPECTED_LOG_STORAGE_DECLINE_BUTTON_ACTION = TStringBuf(R"(
Actions: {
    Type: 2
    Id: "test-uuid"
    ToType: 1
    ToId: "item-id"
    ActionType: 5
    Value: 1
    Timestamp: 144000
    RequestId: "test-reqid"
    Payload {
        [NDJ.NAS.TActionPayloadData.AlicePayloadExtension] {
            Tags: "tag_1"
            Source: "postroll_source"
            Context: {
                App: "aliced"
                Features: "change_alarm_sound"
            }
            PostrollViewRequestId: "req-id-with-postroll"
        }
    }
}
)");
constexpr TStringBuf EXPECTED_LOG_STORAGE_NO_ACTIONS = "";

constexpr TStringBuf EXPECTED_POSTROLL_ANALYTICS_JSON = R"({
    "modifiers_analytics_info": {
        "postroll": {
            "frame_actions": [
                {
                    "name": "postroll_action"
                },
                {
                    "name": "postroll_decline"
                }
            ]
        }
    }
})";

constexpr auto EXPECTED_PROACTIVITY_INFO = TStringBuf(R"(
    ItemId: "42"
    Appended: true
    FromSkillRec: true
    ItemInfo: "awesome postroll"
    Tags: "music"
    Tags: "alarm"
    IsMarketingPostroll: false
)");
constexpr auto EXPECTED_PROACTIVITY_INFO_MARKETING_POSTROLL = TStringBuf(R"(
    ItemId: "42"
    Appended: true
    FromSkillRec: true
    ItemInfo: "awesome postroll"
    Tags: "music"
    Tags: "alarm"
    IsMarketingPostroll: true
)");

using TInputBuilder = TModifierTestFixture::TInputBuilder;
TInputBuilder InputBuilder() {
    auto builder = TInputBuilder()
        .SetStorage(DEFAULT_STORAGE);
    return builder;
}

TString MakeMementoUserConfigs(TStringBuf storage) {
    return TString::Join(R"(
{
    "ProactivityConfig": {
        "Storage": )", storage, R"(
    }
}
)");
}

Y_UNIT_TEST_SUITE_F(PostrollModifier, TModifierTestFixture) {
    // SkillRec postroll tests -------------------------------------------------
    Y_UNIT_TEST(ServicePostrollNoSkillRec) {
        TestExpectedNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .Build(),
            TNonApply::EType::ModSpecific,
            TStringBuf("no valid SkillRec response for source")
        );
    }

    Y_UNIT_TEST(ServicePostrollNoUserVoice) {
        TestExpectedNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL_NO_USER_VOICE)
                .SetSkillRec(SKILL_REC_PROACTIVITY)
                .Build(),
            TNonApply::EType::ModSpecific,
            TStringBuf("default condition is not satisfied")
        );
    }

    Y_UNIT_TEST(ServicePostrollNoUserVoiceIgnoreVoiceResponse) {
        TestExpectedTextAndVoice(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL_NO_USER_VOICE)
                .SetSkillRec(SKILL_REC_PROACTIVITY_NO_MATTER_THE_VOICE_CONDITION)
                .Build(),
            EXPECTED_TEXT_RESPONSE_WITHOUT_POSTROLL,
            "" // This should not actually happen, but it's good to know that in this case we'd get an EMPTY postroll
        );
    }

    Y_UNIT_TEST(ServicePostrollNoTextResponse) {
        TestExpectedNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY_WITH_TEXT)
                .SetAddTextReponse(false)
                .Build(),
            TNonApply::EType::ModSpecific,
            TStringBuf("default condition is not satisfied")
        );
    }

    Y_UNIT_TEST(ServicePostrollNoTextResponseIgnore) {
        TestExpectedTextAndVoice(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY_NO_MATTER_THE_TEXT_CONDITION)
                .SetAddTextReponse(false)
                .Build(),
            EXPECTED_TEXT_SERVICE_POSTROLL_WITHOUT_RESPONSE,
            EXPECTED_TTS_SERVICE_POSTROLL
        );
    }

    Y_UNIT_TEST(ServicePostrollNoVoiceResponse) {
        TestExpectedNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY)
                .SetAddVoiceReponse(false)
                .Build(),
            TNonApply::EType::ModSpecific,
            TStringBuf("default condition is not satisfied")
        );
    }

    Y_UNIT_TEST(ServicePostrollNoVoiceResponseIgnore) {
        TestExpectedTextAndVoice(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY_NO_MATTER_THE_VOICE_CONDITION)
                .SetAddVoiceReponse(false)
                .Build(),
            EXPECTED_TEXT_RESPONSE_WITHOUT_POSTROLL,
            EXPECTED_TTS_SERVICE_POSTROLL_NO_VOICE_RESPONSE
        );
    }

    Y_UNIT_TEST(ServicePostrollMultipleConditions) {
        TestExpectedTextAndVoice(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY_MULTIPLE_CONDITIONS)
                .SetAddTextReponse(false)
                .Build(),
            "",
            EXPECTED_TTS_SERVICE_POSTROLL
        );
    }

    Y_UNIT_TEST(ServicePostroll) {
        TestExpectedTts(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY)
                .Build(),
            EXPECTED_TTS_SERVICE_POSTROLL
        );
    }

    Y_UNIT_TEST(ServicePostrollEnableEmotional) {
        TestExpectedTts(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL_ENABLE_EMOTIONAL)
                .SetSkillRec(SKILL_REC_PROACTIVITY)
                .Build(),
            EXPECTED_TTS_SERVICE_POSTROLL_ENABLE_EMOTIONAL
        );
    }

    Y_UNIT_TEST(ServicePostrollEnableNotificationSound) {
        TestExpectedTts(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL_ENABLE_NOTIFICATION_SOUND)
                .SetSkillRec(SKILL_REC_PROACTIVITY)
                .Build(),
            EXPECTED_TTS_SERVICE_POSTROLL_ENABLE_NOTIFICATION_SOUND
        );
    }

    Y_UNIT_TEST(ServicePostrollUnsatisfiedCondition) {
        TestExpectedNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY_NOT_LISTENING_CONDITION)
                .SetShouldListen(true)
                .Build(),
            TNonApply::EType::ModSpecific,
            TStringBuf("apply condition is not satisfied")
        );
    }

    Y_UNIT_TEST(ServicePostrollUnsatisfiedStackEngineGetNext) {
        TestExpectedNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY_HAS_NOT_STACK_ENGINE_GET_NEXT_CONDITION)
                .SetAddStackEngineGetNext(true)
                .Build(),
            TNonApply::EType::ModSpecific,
            TStringBuf("apply condition is not satisfied")
        );

        TestExpectedNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY_HAS_STACK_ENGINE_GET_NEXT_CONDITION)
                .Build(),
            TNonApply::EType::ModSpecific,
            TStringBuf("apply condition is not satisfied")
        );
    }

    Y_UNIT_TEST(ServicePostrollSatisfiedStackEngineGetNext) {
        TestExpectedTts(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY_HAS_NOT_STACK_ENGINE_GET_NEXT_CONDITION)
                .Build(),
            EXPECTED_TTS_SERVICE_POSTROLL
        );

        TestExpectedTts(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY_HAS_STACK_ENGINE_GET_NEXT_CONDITION)
                .SetAddStackEngineGetNext(true)
                .Build(),
            EXPECTED_TTS_SERVICE_POSTROLL
        );
    }

    Y_UNIT_TEST(ServicePostrollSatisfiedCondition) {
        TestExpectedTts(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY_NOT_LISTENING_CONDITION)
                .Build(),
            EXPECTED_TTS_SERVICE_POSTROLL_NOT_LISTENING
        );
    }

    Y_UNIT_TEST(ServicePostrollSatisfiedConditionForkedListening) {
        TestExpectedTts(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRecs({SKILL_REC_PROACTIVITY_NOT_LISTENING_CONDITION, SKILL_REC_PROACTIVITY_LISTENING_CONDITION})
                .SetShouldListen(true)
                .Build(),
            EXPECTED_TTS_SERVICE_POSTROLL_LISTENING
        );
    }

    Y_UNIT_TEST(ServicePostrollSatisfiedConditionForkedNotListening) {
        TestExpectedTts(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRecs({SKILL_REC_PROACTIVITY_NOT_LISTENING_CONDITION, SKILL_REC_PROACTIVITY_LISTENING_CONDITION})
                .Build(),
            EXPECTED_TTS_SERVICE_POSTROLL_NOT_LISTENING
        );
    }

    Y_UNIT_TEST(ServicePostrollSatisfiedConditionForkedByScore) {
        TestExpectedTts(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRecs({SKILL_REC_PROACTIVITY_NOT_LISTENING_CONDITION, SKILL_REC_PROACTIVITY_NOT_LISTENING_CONDITION_HIGH_SCORE})
                .Build(),
            EXPECTED_TTS_SERVICE_POSTROLL_NOT_LISTENING_HIGH_SCORE
        );
        TestExpectedTts(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRecs({SKILL_REC_PROACTIVITY_NOT_LISTENING_CONDITION_HIGH_SCORE, SKILL_REC_PROACTIVITY_NOT_LISTENING_CONDITION_HIGH_SCORE})
                .Build(),
            EXPECTED_TTS_SERVICE_POSTROLL_NOT_LISTENING_HIGH_SCORE
        );
    }

    Y_UNIT_TEST(ServicePostrollAddSkillAction) {
        TestExpectedAction(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY_FRAME_ACTION)
                .Build(),
            EXPECTED_ACTION_NAME_POSTROLL_ADD_SKILL_ACTION,
            EXPECTED_ACTION_BODY_POSTROLL_ADD_SKILL_ACTION
        );
    }

    Y_UNIT_TEST(TestIncRateCall) {
        TestExpectedIncRateCall(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY)
                .Build(),
            "awesome postroll",
            ""
        );
    }

    Y_UNIT_TEST(TestAnalytics) {
        TestPostrollAnalytics(
            CreatePostrollModifier,
            InputBuilder().SetSkrJson(SKR_SERVICE_POSTROLL).SetSkillRec(SKILL_REC_PROACTIVITY_FRAME_ACTION).Build(),
            EXPECTED_POSTROLL_ANALYTICS_JSON
        );
    }

    // Proactivity log tests ---------------------------------------------------
    Y_UNIT_TEST(ServiceProactivityLogStorage) {
        TestExpectedLogStorage(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY)
                .Build(),
            EXPECTED_LOG_STORAGE_SERVICE_POSTROLL
        );
    }

    Y_UNIT_TEST(ServicePostrollSaveConditions) {
        TestExpectedResponse(
            CreatePostrollModifier,
            InputBuilder()
                .SetIntent(INTENT_GET_TIME)
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY_SAVE_CONDITION)
                .SetUserConfigs(MakeMementoUserConfigs(PERSONAL_DATA))
                .Build(),
            EXPECTED_SERVICE_SAVE_CONDITION_DATA_MM
        );
    }

    Y_UNIT_TEST(ServicePostrollSaveConditionsPlusMemento) {
        TestExpectedResponse(
            CreatePostrollModifier,
            InputBuilder()
                .SetIntent(INTENT_GET_TIME)
                .SetSkrJson(SKR_SERVICE_POSTROLL_SAVE_MEMENTO)
                .SetSkillRec(SKILL_REC_PROACTIVITY_SAVE_CONDITION)
                .Build(),
            EXPECTED_SERVICE_SAVE_CONDITION_PLUS_MEMENTO_DATA_MM
        );
    }

    Y_UNIT_TEST(ServicePostrollSaveConditionsPlusMementoRead) {
        TestExpectedResponse(
            CreatePostrollModifier,
            InputBuilder()
                .SetIntent(INTENT_GET_TIME)
                .SetSkrJson(SKR_SERVICE_POSTROLL_SAVE_MEMENTO)
                .SetSkillRec(SKILL_REC_PROACTIVITY_SAVE_CONDITION)
                .SetUserConfigs(MEMENTO_USER_CONFIGS)
                .Build(),
            EXPECTED_SERVICE_SAVE_CONDITION_PLUS_MEMENTO_READ_DATA_MM
        );
    }

    Y_UNIT_TEST(ServicePostrollSaveCauseTimePassed) {
        TestExpectedResponseNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetIntent(INTENT_GET_TIME)
                .SetSkrJson(SKR_SERVICE_POSTROLL_TIME_PASSED)
                .Build(),
            EXPECTED_SERVICE_SAVE_CAUSE_TIME_PASSED_MM,
            TNonApply::EType::ModSpecific,
            TStringBuf("no valid SkillRec response for source")
        );
    }

    Y_UNIT_TEST(ServicePostrollDoNotSaveCauseNotEnoughTimePassed) {
        TestExpectedResponseNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetIntent(INTENT_GET_TIME)
                .SetSkrJson(SKR_SERVICE_POSTROLL_NOT_ENOUGH_TIME_PASSED)
                .Build(),
            EXPECTED_SERVICE_DO_NOT_SAVE_MM,
            TNonApply::EType::ModSpecific,
            TStringBuf("no valid SkillRec response for source")
        );
    }

    Y_UNIT_TEST(ServicePostrollDoNotSaveCauseServerAction) {
        TestExpectedResponseNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetIntent(INTENT_GET_TIME)
                .SetSkrJson(SKR_SERVICE_POSTROLL_SERVER_ACTION)
                .Build(),
            EXPECTED_SERVICE_DO_NOT_SAVE_MM,
            TNonApply::EType::ModSpecific,
            TStringBuf("no valid SkillRec response for source")
        );
    }

    Y_UNIT_TEST(ServicePostrollDoNotSaveCauseNotSpeaker) {
        TestExpectedResponseNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetIntent(INTENT_GET_TIME)
                .SetSkrJson(SKR_SERVICE_POSTROLL_SEARCHAPP)
                .Build(),
            EXPECTED_SERVICE_DO_NOT_SAVE_MM,
            TNonApply::EType::ModSpecific,
            TStringBuf("no valid SkillRec response for source")
        );
    }
    Y_UNIT_TEST(ServicePostrollSaveCauseEnableAllApps) {
        TestExpectedResponseNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetIntent(INTENT_GET_TIME)
                .SetSkrJson(SKR_SERVICE_POSTROLL_SEARCHAPP_ENABLE_ALL_APPS)
                .Build(),
            EXPECTED_SERVICE_SAVE_CAUSE_ENABLE_ALL_APPS,
            TNonApply::EType::ModSpecific,
            TStringBuf("no valid SkillRec response for source")
        );
    }

    Y_UNIT_TEST(CheckStopAction) {
        TestExpectedLogStorageNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetIntent(INTENT_FAST_CANCEL)
                .SetSkrJson(SKR_STOP_ACTION)
                .SetUserConfigs(MakeMementoUserConfigs(PERSONAL_DATA_STOP_ACTION))
                .Build(),
            EXPECTED_LOG_STORAGE_STOP_ACTION,
            TNonApply::EType::ModSpecific,
            TStringBuf("no valid SkillRec response for source")
        );
    }

    Y_UNIT_TEST(CheckStopActionNotStopIntent) {
        TestExpectedLogStorageNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetIntent(INTENT_HELLO)
                .SetSkrJson(SKR_STOP_ACTION)
                .SetUserConfigs(MakeMementoUserConfigs(PERSONAL_DATA_STOP_ACTION))
                .Build(),
            EXPECTED_LOG_STORAGE_NO_ACTIONS,
            TNonApply::EType::ModSpecific,
            TStringBuf("no valid SkillRec response for source")
        );
    }

    Y_UNIT_TEST(CheckStopActionDifferentReqId) {
        TestExpectedLogStorageNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetIntent(INTENT_FAST_CANCEL)
                .SetSkrJson(SKR_STOP_ACTION)
                .SetUserConfigs(MakeMementoUserConfigs(PERSONAL_DATA_STOP_ACTION_DIFFERENT_REQID))
                .Build(),
            EXPECTED_LOG_STORAGE_NO_ACTIONS,
            TNonApply::EType::ModSpecific,
            TStringBuf("no valid SkillRec response for source")
        );
    }

    Y_UNIT_TEST(CheckDeclineButtonAction) {
        TestExpectedLogStorageNonApply(
            CreatePostrollModifier,
            InputBuilder()
                .SetIntent(NAlice::HOLLYWOOD_DO_NOTHING_SCENARIO)
                .SetSkrJson(SKR_STOP_ACTION)
                .SetUserConfigs(MakeMementoUserConfigs(PERSONAL_DATA_STOP_ACTION))
                .Build(),
            EXPECTED_LOG_STORAGE_DECLINE_BUTTON_ACTION,
            TNonApply::EType::ModSpecific,
            TStringBuf("no valid SkillRec response for source")
        );
    }

    Y_UNIT_TEST(ServiceProactivityInfo) {
        TestExpectedProactivityInfo(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY)
                .Build(),
            EXPECTED_PROACTIVITY_INFO
        );
    }

    Y_UNIT_TEST(ServiceProactivityInfoMarketing) {
        TestExpectedProactivityInfo(
            CreatePostrollModifier,
            InputBuilder()
                .SetSkrJson(SKR_SERVICE_POSTROLL)
                .SetSkillRec(SKILL_REC_PROACTIVITY_MARKETING_POSTROLL)
                .Build(),
            EXPECTED_PROACTIVITY_INFO_MARKETING_POSTROLL
        );
    }
}

} // namespace
