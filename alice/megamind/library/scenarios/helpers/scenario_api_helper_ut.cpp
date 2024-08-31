#include "scenario_api_helper.h"

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/scenarios/protocol/protocol_scenario.h>
#include <alice/megamind/library/stack_engine/stack_engine.h>
#include <alice/megamind/library/request/builder.h>
#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/mock_data_sources.h>
#include <alice/megamind/library/testing/mock_responses.h>
#include <alice/megamind/library/testing/speechkit.h>

#include <alice/protos/api/nlu/generated/features.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <alice/library/blackbox/proto/blackbox.pb.h>
#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/struct.pb.h>
#include <google/protobuf/wrappers.pb.h>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>

#include <kernel/geodb/countries.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice::NMegamind {

using namespace NScenarios;
using namespace ::testing;

constexpr auto TRIVIAL_SK_REQUEST = TStringBuf(R"({
    "request": {
        "event":{
            "name":"",
            "type":"text_input",
            "text":"давай поиграем в города"
        }
    }
})");

constexpr auto BASE_SK_REQUEST = TStringBuf(R"(
{
    "application": {
        "app_id": "com.yandex.alicekit.demo",
        "app_version": "10.0",
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
        "event":{
            "name":"",
            "type":"text_input",
            "text":"давай поиграем в города"
        },
        "additional_options": {
            "bass_options": {
                "user_agent": "AliceKit/4.0",
                "client_ip": "127.0.0.1",
                "filtration_level": 0,
                "screen_scale_factor": 3.5,
                "video_gallery_limit": 42
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
                "keyboard",
                "no_microphone",
                "music_player_allow_shots",
                "music_sdk_client",
                "open_dialogs_in_tabs",
                "open_link_search_viewport",
                "tts_play_placeholder",
                "cloud_push_implementation",
                "image_recognizer",
                "show_view"
            ],
            "unsupported_features": [
                "cec_available",
                "open_address_book"
            ],
            "radiostations": [
                "Авторадио",
                "Максимум",
                "Монте-Карло"
            ]
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
        "voice_session": true,
        "smart_home": {
            "payload": {
                "capabilities": [
                    {
                        "instance": "on",
                        "type": "devices.capabilities.on_off",
                        "analytics_name": "включение/выключение"
                    }
                ],
                "colors": [],
                "devices": [
                    {
                        "created": 1576763702,
                        "external_id": "03007884c914242b048f.yandexstation",
                        "properties": [],
                        "type": "devices.types.smart_speaker.yandex.station",
                        "capabilities": [],
                        "analytics_type": "Умное устройство",
                        "id": "5bcd1df8-dbc1-4258-935a-5e9de70d1230",
                        "quasar_info": {
                            "device_id": "03007884c914242b048f",
                            "platform": "yandexstation"
                        },
                        "room_id": "5756f535-52a3-483f-9307-89c7b69c4681",
                        "original_type": "devices.types.smart_speaker.yandex.station",
                        "name": "Яндекс Станция",
                        "groups": []
                    }
                ],
                "groups": [],
                "rooms": [
                    {
                        "name": "Спальня",
                        "id": "5756f535-52a3-483f-9307-89c7b69c4681"
                    }
                ],
                "scenarios": []
            }
        },
        "megamind_cookies": "{\"uaas_tests\":[247071]}"

    },
    "session": null
}
)");

constexpr auto SK_EVENT_TEXT_INPUT = TStringBuf(R"(
{
    "name": "",
    "type": "text_input",
    "text": "давай поиграем в Города"
}
)");

constexpr auto SK_EVENT_VOICE_INPUT = TStringBuf(R"(
{
    "type": "voice_input",
    "hypothesis_number": 2,
    "end_of_utterance": true,
    "asr_result": {
        "confidence": 1.0,
        "normalized": "давай поиграем в города",
        "utterance": "давай поиграем в города",
        "words": [
            {
                "value": "давай",
                "confidence": 1.0
            },
            {
                "value": "поиграем",
                "confidence": 1.0
            },
            {
                "value": "в",
                "confidence": 1.0
            },
            {
                "value": "города",
                "confidence": 1.0
            }
        ]
    },
    "biometry_scoring": {
        "status": "foo",
        "request_id": "bar",
        "scores_with_mode": [
            {
                "mode": "baz",
                "scores": [
                    {
                        "score": 0.8,
                        "user_id": "alice"
                    }
                ]
            },
            {
                "mode": "bazz",
                "scores": [
                    {
                        "score": 0.7,
                        "user_id": "bob"
                    }
                ]
            }
        ]
    },
    "biometry_classification": {
        "status": "foo",
        "scores": [
            {
                "classname": "jedi",
                "confidence": 0.75,
                "tag": "force_user"
            },
            {
                "classname": "sith",
                "confidence": 0.25,
                "tag": "force_user"
            }
        ]
    }
}
)");

constexpr auto SK_EVENT_SERVER_ACTION = TStringBuf(R"(
{
    "name": "callback_name",
    "type": "server_action",
    "ignore_answer": true,
    "payload": {
        "@scenario_name": "ScenarioName",
        "payload_key": "PayloadValue"
    }
}
)");

constexpr auto SK_EVENT_IMAGE_INPUT = TStringBuf(R"(
{
    "type": "image_input",
    "payload": {
        "img_url": "https://avatars.mds.yandex.net/get-alice/some-funny-img"
    }
}
)");

constexpr auto SK_EVENT_MUSIC_INPUT = TStringBuf(R"(
{
    "type": "music_input",
    "music_result": {
        "data": {
            "match": {
                "a": 1,
                "b": "c",
                "c": true
            },
            "recognition-id": "84c38430-00fa-11e8-8a9a-00163ea01244",
            "engine": "YANDEX",
            "url": "http://yandex.ru/music"
        },
        "result": "success",
        "error_text": null
    }
}
)");

constexpr auto BASE_PROTOCOL_REQUEST = TStringBuf(R"(
{
    "base_request": {
        "client_info": {
            "app_id": "com.yandex.alicekit.demo",
            "app_version": "10.0",
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
        "device_state": {
            "sound_level": 0,
            "sound_muted": true,
            "is_tv_plugged_in": true
        },
        "dialog_id": null,
        "experiments": {
            "find_poi_gallery": "1",
            "iot": "1",
            "personal_assistant.scenarios.music_play": -5.947592122,
            "personal_assistant.scenarios.video_play": -4.624749712
        },
        "interfaces": {
            "can_change_alarm_sound": false,
            "can_open_dialogs_in_tabs": true,
            "can_open_keyboard": true,
            "can_open_link": true,
            "can_open_link_intent": true,
            "can_open_link_search_viewport": true,
            "can_open_link_turboapp": false,
            "can_open_link_yellowskin": true,
            "can_open_quasar_screen": true,
            "can_open_whocalls": true,
            "can_open_yandex_auth": true,
            "can_recognize_image": true,
            "can_render_div2_cards": true,
            "can_render_div_cards": true,
            "can_server_action": true,
            "can_show_gif": true,
            "can_show_timer": true,
            "has_access_to_battery_power_state": true,
            "has_bluetooth": false,
            "has_cec": false,
            "has_cloud_push": true,
            "has_led_display": false,
            "has_microphone": false,
            "has_music_player": true,
            "has_music_player_shots": true,
            "has_music_sdk_client": true,
            "has_reliable_speakers": false,
            "has_screen": true,
            "is_tv_plugged": true,
            "outgoing_phone_calls": true,
            "supports_absolute_volume_change": true,
            "supports_any_player": true,
            "supports_buttons": true,
            "supports_div_cards_rendering": true,
            "supports_feedback": true,
            "supports_mute_unmute_volume": true,
            "supports_player_continue_directive": true,
            "supports_player_dislike_directive": true,
            "supports_player_like_directive": true,
            "supports_player_next_track_directive": true,
            "supports_player_pause_directive": true,
            "supports_player_previous_track_directive": true,
            "supports_player_rewind_directive": true,
            "supports_show_view": true,
            "supports_show_view_layer_content": true,
            "tts_play_placeholder": true,
            "voice_session": true
        },
        "location": {
            "accuracy": 24.21999931,
            "lat": 55.7364953,
            "lon": 37.6404265,
            "recency": 23450,
            "speed": 0
        },
        "options": {
            "user_agent": "AliceKit/4.0",
            "client_ip": "127.0.0.1",
            "filtration_level": 0,
            "screen_scale_factor": 3.5,
            "video_gallery_limit": 42,
            "raw_personal_data": "{}",
            "radio_stations": [
                "Авторадио",
                "Максимум",
                "Монте-Карло"
            ],
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
            "megamind_cookies": {
                "uaas_tests": [
                    247071
                ]
            },
            "can_use_user_logs": true
        },
        "user_preferences": {
            "filtration_mode": "NoFilter"
        },
        "user_classification": {
            "age": "Adult"
        },
        "random_seed": 42,
        "user_language": "L_RUS",
        "request_id": "d34df00d-c135-4227-8cf8-386d7d989237",
        "server_time_ms": 1575317078,
        "state": {},
        "memento": {
            "user_configs": {}
        }
    }
}
)");

constexpr auto BASE_PROTOCOL_REQUEST_DATA_SOURCES = TStringBuf(R"(
{
    "data_sources": {}
}
)");

constexpr auto DATA_SOURCES_WITH_USER_LOCATION = TStringBuf(R"(
{
    "data_sources": {
        "4": {
            "user_location": {
                "user_region": 213,
                "user_tld": "ru",
                "user_country": 225
            }
        }
    }
}
)");

constexpr auto DATA_SOURCES_WITH_BEGEMOT_MARKUP = TStringBuf(R"(
{
    "data_sources": {
        "6": {
            "begemot_external_markup": {
                "Delimiters": [
                    {},
                    {
                        "EndChar": 5,
                        "BeginChar": 2,
                        "Text": " + "
                    },
                    {}
                ],
                "ProcessedRequest": "10 + 10",
                "OriginalRequest": "10 + 10",
                "Morph": [
                    {
                        "Lemmas": [
                            {
                                "Text": "00000000010"
                            }
                        ],
                        "Tokens": {
                            "End": 1,
                            "Begin": 0
                        }
                    },
                    {
                        "Lemmas": [
                            {
                                "Text": "00000000010"
                            }
                        ],
                        "Tokens": {
                            "End": 2,
                            "Begin": 1
                        }
                    }
                ],
                "Tokens": [
                    {
                        "EndChar": 2,
                        "BeginChar": 0,
                        "Text": "10"
                    },
                    {
                        "EndChar": 7,
                        "BeginChar": 5,
                        "Text": "10"
                    }
                ],
                "DirtyLang": {
                    "DirtyLangClass": "DIRTY"
                }
            }
        }
    }
}
)");

constexpr auto DATA_SOURCES_WITH_DIALOG_HISTORY_WITHOUT_PREV_RESPONSE = TStringBuf(R"(
{
    "data_sources": {
        "7": {
            "dialog_history": {
                "phrases": ["Привет!", "Хэллоу", "Как дела?", "Норм"],
                "dialog_turns": [
                    {"request": "Привет!", "response": "Хэллоу", "rewritten_request": "Привет!", "scenario_name": "SomeScenario", "client_time_ms": 1},
                    {"request": "Как дела?", "response": "Норм", "rewritten_request": "Как дела?", "scenario_name": "SomeScenario", "server_time_ms": 2, "client_time_ms": 3}
                ]
            }
        }
    }
}
)");

constexpr auto DATA_SOURCES_WITH_DIALOG_HISTORY_WITH_PREV_RESPONSE = TStringBuf(R"(
{
    "data_sources": {
        "7": {
            "dialog_history": {
                "phrases": ["Привет!", "Хэллоу", "Как дела?", "Норм"],
                "dialog_turns": [
                    {"request": "Привет!", "response": "Хэллоу", "rewritten_request": "Привет!", "scenario_name": "SomeScenario", "client_time_ms": 1},
                    {"request": "Как дела?", "response": "Норм", "rewritten_request": "Как дела?", "scenario_name": "SomeScenario", "server_time_ms": 2, "client_time_ms": 3}
                ],
            }
        },
        "27": {
            "response_history": {
                "prev_response": {
                    "layout": {
                        "output_speech": "Хэллоу"
                    },
                    "actions": {
                        "action": {
                            "frame": {
                                "name": "semantic_frame"
                            }
                        }
                    }
                }
            }
        }
    }
}
)");

constexpr auto DATA_SOURCES_WITH_SMART_HOME = TStringBuf(R"(
{
    "data_sources": {
        "20": {
            "smart_home": {
                "payload": {
                    "capabilities": [
                        {
                            "instance": "on",
                            "type": "devices.capabilities.on_off"
                        }
                    ],
                    "colors": [],
                    "devices": [
                        {
                            "created": 1576763702.12345,
                            "type": "devices.types.smart_speaker.yandex.station",
                            "capabilities": [],
                            "id": "5bcd1df8-dbc1-4258-935a-5e9de70d1230",
                            "quasar_info": {
                                "device_id": "03007884c914242b048f",
                                "platform": "yandexstation"
                            },
                            "room_id": "5756f535-52a3-483f-9307-89c7b69c4681",
                            "name": "Яндекс Станция",
                            "groups": []
                        }
                    ],
                    "groups": [],
                    "rooms": [
                        {
                            "name": "Спальня",
                            "id": "5756f535-52a3-483f-9307-89c7b69c4681"
                        }
                    ],
                    "scenarios": []
                }
            }
        }
    }
}
)");

constexpr auto BEGEMOT_MARKUP_RESPONSE = TStringBuf(R"(
{
    "AliceResponse": {
        "ExternalMarkup": {
            "JSON": {
                "Delimiters": [
                    {},
                    {
                        "EndByte": 5,
                        "BeginByte": 2,
                        "EndChar": 5,
                        "BeginChar": 2,
                        "Text": " + "
                    },
                    {}
                ],
                "ProcessedRequest": "10 + 10",
                "OriginalRequest": "10 + 10",
                "Morph": [
                    {
                        "Lemmas": [
                            {
                                "Text": "00000000010"
                            }
                        ],
                        "Tokens": {
                            "End": 1,
                            "Begin": 0
                        }
                    },
                    {
                        "Lemmas": [
                            {
                                "Text": "00000000010"
                            }
                        ],
                        "Tokens": {
                            "End": 2,
                            "Begin": 1
                        }
                    }
                ],
                "Tokens": [
                    {
                        "EndByte": 2,
                        "BeginByte": 0,
                        "EndChar": 2,
                        "BeginChar": 0,
                        "Text": "10"
                    },
                    {
                        "EndByte": 7,
                        "BeginByte": 5,
                        "EndChar": 7,
                        "BeginChar": 5,
                        "Text": "10"
                    }
                ],
                "DirtyLang": {
                    "Class": "DIRTY"
                }
            }
        }
    }
}
)");

constexpr auto PROTOCOL_RUN_TEXT = TStringBuf(R"(
{
    "input": {
        "semantic_frames": [
            {
                "name": "personal_assistant.scenarios.external_skill",
                "slots": []
            }
        ],
        "text": {
            "raw_utterance": "давай поиграем в Города",
            "utterance": "давай поиграем в города"
        }
    }
}
)");

constexpr auto PROTOCOL_RUN_VOICE = TStringBuf(R"(
{
    "input": {
        "semantic_frames": [
            {
                "name": "personal_assistant.scenarios.external_skill",
                "slots": []
            }
        ],
        "voice": {
            "utterance": "давай поиграем в города",
            "asr_data": [
                {
                    "confidence": 1.0,
                    "normalized": "давай поиграем в города",
                    "utterance": "давай поиграем в города",
                    "words": [
                        {
                            "value": "давай",
                            "confidence": 1.0
                        },
                        {
                            "value": "поиграем",
                            "confidence": 1.0
                        },
                        {
                            "value": "в",
                            "confidence": 1.0
                        },
                        {
                            "value": "города",
                            "confidence": 1.0
                        }
                    ]
                }
            ],
            "biometry_scoring": {
                "status": "foo",
                "request_id": "bar",
                "scores_with_mode": [
                    {
                        "mode": "baz",
                        "scores": [
                            {
                                "score": 0.8,
                                "user_id": "alice"
                            }
                        ]
                    },
                    {
                        "mode": "bazz",
                        "scores": [
                            {
                                "score": 0.7,
                                "user_id": "bob"
                            }
                        ]
                    }
                ]
            },
            "biometry_classification": {
                "status": "foo",
                "scores": [
                    {
                        "classname": "jedi",
                        "confidence": 0.75,
                        "tag": "force_user"
                    },
                    {
                        "classname": "sith",
                        "confidence": 0.25,
                        "tag": "force_user"
                    }
                ]
            }
        }
    }
}
)");

constexpr auto PROTOCOL_RUN_SERVER_ACTION = TStringBuf(R"(
{
    "input": {
        "callback": {
            "ignore_answer": true,
            "name": "callback_name",
            "is_led_silent": true,
            "payload": {
                "payload_key": "PayloadValue"
            }
        },
        "semantic_frames": [
            {
                "name": "personal_assistant.scenarios.external_skill",
                "slots": []
            }
        ]
    }
}
)");

constexpr auto PROTOCOL_RUN_IMAGE_INPUT = TStringBuf(R"(
{
    "input": {
        "semantic_frames": [
            {
                "name": "personal_assistant.scenarios.external_skill",
                "slots": []
            }
        ],
        "image": {
            "url": "https://avatars.mds.yandex.net/get-alice/some-funny-img"
        }
    }
}
)");

constexpr auto PROTOCOL_RUN_MUSIC_INPUT = TStringBuf(R"(
{
    "input": {
        "semantic_frames": [
            {
                "name": "personal_assistant.scenarios.external_skill",
                "slots": []
            }
        ],
        "music": {
            "music_result": {
                "data": {
                    "match": {
                        "a": 1,
                        "b": "c",
                        "c": true
                    },
                    "recognition-id": "84c38430-00fa-11e8-8a9a-00163ea01244",
                    "engine": "YANDEX",
                    "url": "http://yandex.ru/music"
                },
                "result": "success",
                "error_text": null
            }
        }
    }
}
)");

constexpr auto PROTOCOL_APPLY_TEXT = TStringBuf(R"(
{
    "arguments": {
        "@type": "type.googleapis.com/google.protobuf.StringValue",
        "value": "some_payload"
    },
    "input": {
        "semantic_frames": [
            {
                "name": "personal_assistant.scenarios.external_skill",
                "slots": []
            }
        ],
        "text": {
            "raw_utterance": "давай поиграем в Города",
            "utterance": "давай поиграем в города"
        }
    }
}
)");

static constexpr TStringBuf MORDOVIA_VIDEO_VIEW_STATE = TStringBuf(R"(
    {
        "greetings": [
            {
                "activation": "Включи FM-радио",
                "type": "info",
                "thumb": "https:\/\/avatars.mds.yandex.net\/get-dialogs\/998463\/qasar_logo_2_poi\/logo-bg-image-quasar-half"
            },
            {
                "activation": "Включи шум волн",
                "type": "info",
                "thumb": "https:\/\/avatars.mds.yandex.net\/get-dialogs\/1676983\/quasar_logo_3_music\/logo-bg-image-quasar-half"
            }
        ],
        "user_subscription_type": "",
        "sections": [
            {
                "type": "main",
                "active": true,
                "items": [
                    {
                        "metaforback": {
                            "subscriptions": [
                                "YA_PLUS_KP",
                                "YA_PLUS_3M",
                                "YA_PREMIUM",
                                "KP_BASIC",
                                "YA_PLUS"
                            ],
                            "serial_id": "4b1f5cde6088dbd6b16c65f3f7e5c136",
                            "title": "Отчаянные домохозяйки",
                            "ontoid": "ruw150260",
                            "uuid": "45a32d68f98505e8b680ad59da5b9322",
                            "season_id": "4b976286f0db7ee1b8e742fe6a151aa9",
                            "streams": [
                                {
                                    "stream_type": "DASH",
                                    "url": "https:\/\/strm.yandex.ru\/vh-ott-converted\/ott-content\/487259609\/45a32d68-f985-05e8-b680-ad59da5b9322.ism\/manifest_quality.mpd",
                                    "drmConfig": {
                                        "servers": {
                                            "com.widevine.alpha": "https:\/\/widevine-proxy.ott.yandex.ru\/proxy"
                                        },
                                        "advanced": {
                                            "com.widevine.alpha": {
                                                "serverCertificateUrl": "https:\/\/widevine-proxy.ott.yandex.ru\/certificate"
                                            }
                                        },
                                        "requestParams": {
                                            "verificationRequired": true,
                                            "monetizationModel": "SVOD",
                                            "contentId": "45a32d68f98505e8b680ad59da5b9322",
                                            "productId": 2,
                                            "serviceName": "ya-station",
                                            "expirationTimestamp": 1592576987,
                                            "puid": 853882730,
                                            "contentTypeId": 21,
                                            "watchSessionId": "5d6f9ce90acd4aa698d3422498acfa89",
                                            "signature": "a0e2b91404f65339ecc6a64e9d59e589c86fa226",
                                            "version": "V4"
                                        }
                                    }
                                }
                            ],
                            "thumbnail": "\/\/avatars.mds.yandex.net\/get-vh\/1067189\/17109576777893233799-xgdXN9vzTC7HgqZrhM0HgA-1546068918\/orig",
                            "url": "https:\/\/strm.yandex.ru\/vh-ott-converted\/ott-content\/487259609-45a32d68f98505e8b680ad59da5b9322\/master.hd_quality.m3u8"
                        },
                        "number": 1,
                        "visible": true,
                        "type": "video",
                        "active": true,
                        "title": "Отчаянные домохозяйки",
                        "metaforlog": {
                            "ugc_rating": "",
                            "supertag_title": "Кинопоиск HD",
                            "supertag": "subscription",
                            "onto_otype": "Film\/Series@on",
                            "restriction_age": 16,
                            "genres": "драма, мелодрама, комедия, детектив",
                            "is_special_project": 0,
                            "onto_id": "ruw150260",
                            "release_year": "2004",
                            "onto_category": "series",
                            "rating_kp": 8.069999695,
                            "theme_title": "Мелодрама",
                            "can_play_on_station": 1,
                            "content_type_name": "vod-episode"
                        }
                    },
                    {
                        "metaforback": {
                            "subscriptions": [],
                            "title": "Премьера фильма Мумий Тролля",
                            "ontoid": "",
                            "uuid": "47ea3dcb4399770dbdff16bed4c0a0a5",
                            "streams": [
                                {
                                    "options": [
                                        "default",
                                        "has-live",
                                        "live-to-vod"
                                    ],
                                    "url": "https:\/\/strm.yandex.ru\/vh-special-converted\/vod-content\/5310335291832671829.m3u8?end=1590170415&from=unknown&partner_id=576132&start=1590165111&target_ref=https%3A\/\/yastatic.net&uuid=47ea3dcb4399770dbdff16bed4c0a0a5&video_category_id=1017"
                                }
                            ],
                            "thumbnail": "https:\/\/yastatic.net\/s3\/home\/station\/mordovia\/mumi_troll_small.png",
                            "duration": 5312,
                            "url": "https:\/\/strm.yandex.ru\/vh-special-converted\/vod-content\/5310335291832671829.m3u8?end=1590170415&from=unknown&partner_id=576132&start=1590165111&target_ref=https%3A\/\/yastatic.net&uuid=47ea3dcb4399770dbdff16bed4c0a0a5&video_category_id=1017"
                        },
                        "number": 2,
                        "visible": true,
                        "type": "video",
                        "active": true,
                        "title": "Премьера фильма Мумий Тролля",
                        "metaforlog": {
                            "number": 2,
                            "supertag_title": "Спецсобытие",
                            "supertag": "special_event",
                            "restriction_age": 6,
                            "is_special_project": 1,
                            "theme_title": "SOS Матросу ",
                            "can_play_on_station": 1,
                            "content_type_name": "episode"
                        }
                    }
                ],
                "id": "main"
            }
        ]
    }
)");

static constexpr ELanguage REQUEST_LANG = LANG_RUS;

const TString ORIGINAL_UTTERANCE = "давай поиграем в Города";
const TString NORMALIZED_UTTERANCE = "давай поиграем в города";

const TSemanticFrame SemanticFrame = []() {
    TSemanticFrame frame;
    frame.SetName("personal_assistant.scenarios.external_skill");
    return frame;
}();

NJson::TJsonValue CreateSpeechKitRequest(TStringBuf event) {
    auto request = NJson::ReadJsonFastTree(BASE_SK_REQUEST);
    request["request"].InsertValue("event", NJson::ReadJsonFastTree(event));
    return request;
}

NJson::TJsonValue CreateSpeechKitRequestWithVideoState(TStringBuf stateKey, TStringBuf stateValue) {
    auto request = NJson::ReadJsonFastTree(BASE_SK_REQUEST);
    request["request"]["device_state"]["video"].InsertValue(stateKey, NJson::ReadJsonFastTree(stateValue));
    return request;
}

NJson::TJsonValue CreateProtocolRequest(TStringBuf requestData,
                                        TStringBuf dataSources = BASE_PROTOCOL_REQUEST_DATA_SOURCES)
{
    auto request = NJson::ReadJsonFastTree(BASE_PROTOCOL_REQUEST);
    const auto data = NJson::ReadJsonFastTree(requestData);
    for (const auto& [k, v] : data.GetMap()) {
        request.InsertValue(k, v);
    }
    const auto dataSourcesJson = NJson::ReadJsonFastTree(dataSources);
    for (const auto& [k, v] : dataSourcesJson.GetMap()) {
        request.InsertValue(k, v);
    }
    return request;
}

TScenarioConfig MakeConfig(const TVector<EDataSourceType>& dataSourceTypes) {
    TScenarioConfig config{};
    for (const auto& type : dataSourceTypes) {
        config.AddDataSources()->SetType(type);
    }
    return config;
}

NMegamind::TMementoDataView GetDefaultMementoData() {
    static const NMegamind::TMementoData defaultMementoData{};
    return NMegamind::TMementoDataView(defaultMementoData,
                                       /* scenarioName= */ Default<TString>(),
                                       /* deviceId= */ Default<TString>(),
                                       /* uuid= */ Default<TString>());
}

TScenarioRunRequest BuildScenarioRunRequestWithPolyglot(const bool scenarioSupportsArabic) {
    const auto request = TSpeechKitRequestBuilder(CreateSpeechKitRequest(SK_EVENT_TEXT_INPUT)).Build();

    TMockResponses responses;
    TMockContext ctx;
    EXPECT_CALL(ctx, ScenarioConfig(_)).WillOnce(ReturnRef(Default<TScenarioInfraConfig>()));
    EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(Default<IContext::TExpFlags>()));
    EXPECT_CALL(ctx, Responses()).WillOnce(ReturnRef(responses));
    EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(request));
    EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
    EXPECT_CALL(ctx, GetSeed()).WillRepeatedly(Return(42));
    EXPECT_CALL(ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_ARA));
    EXPECT_CALL(ctx, PolyglotUtterance()).WillRepeatedly(Return(request->GetRequest().GetEvent().GetText()));
    EXPECT_CALL(ctx, TranslatedUtterance()).WillRepeatedly(Return("Translated utterance"));
    EXPECT_CALL(ctx, NormalizedTranslatedUtterance()).WillRepeatedly(Return("Normalized translated utterance"));
    EXPECT_CALL(ctx, NormalizedPolyglotUtterance()).WillRepeatedly(Return("Normalized polyglot utterance"));

    TMockDataSources dataSources;
    EXPECT_CALL(dataSources, GetDataSource(EDataSourceType::DIALOG_HISTORY)).WillOnce(ReturnRef(Default<NScenarios::TDataSource>()));

    auto scenarioConfig = MakeConfig({EDataSourceType::DIALOG_HISTORY});
    scenarioConfig.AddLanguages(::NAlice::ELang::L_RUS);
    if (scenarioSupportsArabic) {
        scenarioConfig.AddLanguages(::NAlice::ELang::L_ARA);
    }

    return ConstructRunRequest(
        scenarioConfig, CreateRequest(IEvent::CreateEvent(request.Event()), request), ctx, dataSources,
        /* session= */ {.ScenarioState = {}, .IsNewSession = false}, GetDefaultMementoData(),
        /* semanticFrames= */ {SemanticFrame}, /* passDataSourcesInRequest= */ true);

}

Y_UNIT_TEST_SUITE(TestScenarioApiHelper) {
    Y_UNIT_TEST(TestRunRequestWithText) {
        const auto request = TSpeechKitRequestBuilder(CreateSpeechKitRequest(SK_EVENT_TEXT_INPUT)).Build();

        UNIT_ASSERT_EQUAL(request.Event().GetType(), text_input);

        const auto expectedRequest = JsonToProto<TScenarioRunRequest>(CreateProtocolRequest(PROTOCOL_RUN_TEXT));

        TMockContext ctx;
        TScenarioInfraConfig infraConfig;
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillOnce(ReturnRef(infraConfig));
        IContext::TExpFlags expFlags;
        EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));
        TMockResponses responses;
        EXPECT_CALL(ctx, Responses()).WillOnce(ReturnRef(responses));
        TMockDataSources dataSources;
        NScenarios::TDataSource dialogHistory{};
        EXPECT_CALL(dataSources, GetDataSource(EDataSourceType::DIALOG_HISTORY)).WillOnce(ReturnRef(dialogHistory));
        const auto& apiRequest = NImpl::ConstructRunRequest(
            MakeConfig({EDataSourceType::DIALOG_HISTORY}), request, ctx, dataSources,
            /* seed= */ 42, REQUEST_LANG, /* session= */ {.ScenarioState = {}, .IsNewSession = false},
            GetDefaultMementoData(), /* semanticFrames= */ {SemanticFrame},
            CreateRequest(IEvent::CreateEvent(request.Event()), request), NORMALIZED_UTTERANCE,
            Nothing(), TRTLogger::NullLogger(), /* passDataSourcesInRequest= */ true);

        UNIT_ASSERT_MESSAGES_EQUAL(expectedRequest, apiRequest);
    }

    Y_UNIT_TEST(TestRunRequestWithPolyglotRuOnly) {
        const auto scenarioRunRequest = BuildScenarioRunRequestWithPolyglot(false);
        UNIT_ASSERT_EQUAL(ELang::L_RUS, scenarioRunRequest.GetBaseRequest().GetUserLanguage());
        UNIT_ASSERT_EQUAL("Normalized translated utterance", scenarioRunRequest.GetInput().GetText().GetUtterance());
    }

    Y_UNIT_TEST(TestRunRequestWithPolyglotRuAr) {
        const auto scenarioRunRequest = BuildScenarioRunRequestWithPolyglot(true);
        UNIT_ASSERT_EQUAL(ELang::L_ARA, scenarioRunRequest.GetBaseRequest().GetUserLanguage());
        UNIT_ASSERT_EQUAL("Normalized polyglot utterance", scenarioRunRequest.GetInput().GetText().GetUtterance());
    }

    Y_UNIT_TEST(TestStackOwnerInRunRequest) {
        const auto request = TSpeechKitRequestBuilder(CreateSpeechKitRequest(SK_EVENT_TEXT_INPUT)).Build();

        UNIT_ASSERT_EQUAL(request.Event().GetType(), text_input);

        const auto expectedRequest = [] {
            auto request = JsonToProto<TScenarioRunRequest>(CreateProtocolRequest(PROTOCOL_RUN_TEXT));
            request.MutableBaseRequest()->SetIsStackOwner(true);
            return request;
        }();

        TMockContext ctx;
        TScenarioInfraConfig infraConfig;
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillOnce(ReturnRef(infraConfig));
        IContext::TExpFlags expFlags;
        EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));
        TMockResponses responses;
        EXPECT_CALL(ctx, Responses()).WillOnce(ReturnRef(responses));
        TMockDataSources dataSources;
        NScenarios::TDataSource dialogHistory{};
        EXPECT_CALL(dataSources, GetDataSource(EDataSourceType::DIALOG_HISTORY)).WillOnce(ReturnRef(dialogHistory));
        constexpr auto scenarioName = "scenarioName";
        const auto scenarioConfig = [&] {
            auto config = MakeConfig({EDataSourceType::DIALOG_HISTORY});
            config.SetName(scenarioName);
            return config;
        }();
        const auto requestModel =
            std::move(
                TRequestBuilder{CreateRequest(IEvent::CreateEvent(request.Event()), request)}.SetStackEngineCore([&] {
                    NMegamind::TStackEngine stackEngine{};
                    stackEngine.StartNewSession("requestId", "productScenarioName", scenarioName);
                    return std::move(stackEngine).ReleaseCore();
                }()))
                .Build();
        const auto apiRequest = NImpl::ConstructRunRequest(
            scenarioConfig, request, ctx, dataSources,
            /* seed= */ 42, REQUEST_LANG, /* session= */ {.ScenarioState = {}, .IsNewSession = false},
            GetDefaultMementoData(), /* semanticFrames= */ {SemanticFrame}, requestModel, NORMALIZED_UTTERANCE,
            Nothing(), TRTLogger::NullLogger(), /* passDataSourcesInRequest= */ true);

        UNIT_ASSERT_MESSAGES_EQUAL(expectedRequest, apiRequest);
    }

    Y_UNIT_TEST(TestRunRequestWithState) {
        const auto& request = TSpeechKitRequestBuilder(CreateSpeechKitRequest(SK_EVENT_TEXT_INPUT)).Build();
        auto expectedRequest = JsonToProto<TScenarioRunRequest>(CreateProtocolRequest(PROTOCOL_RUN_TEXT));
        expectedRequest.MutableBaseRequest()->SetIsNewSession(true);
        TState state;
        google::protobuf::StringValue value;
        value.set_value("state");
        state.MutableState()->PackFrom(value);
        expectedRequest.MutableBaseRequest()->MutableState()->PackFrom(value);

        TMockContext ctx;
        TScenarioInfraConfig infraConfig;
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillOnce(ReturnRef(infraConfig));
        IContext::TExpFlags expFlags;
        EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));
        TMockResponses responses;
        EXPECT_CALL(ctx, Responses()).WillOnce(ReturnRef(responses));
        TMockDataSources dataSources;
        NScenarios::TDataSource dialogHistory{};
        EXPECT_CALL(dataSources, GetDataSource(EDataSourceType::DIALOG_HISTORY)).WillOnce(ReturnRef(dialogHistory));

        const auto& apiRequest = NImpl::ConstructRunRequest(
            MakeConfig({EDataSourceType::DIALOG_HISTORY}), request, ctx, dataSources,
            /* seed= */ 42, REQUEST_LANG, /* session= */ {.ScenarioState = state, .IsNewSession = true},
            GetDefaultMementoData(), /* semanticFrames= */ {SemanticFrame},
            CreateRequest(IEvent::CreateEvent(request.Event()), request), NORMALIZED_UTTERANCE,
            Nothing(), TRTLogger::NullLogger(), /* passDataSourcesInRequest= */ true);

        UNIT_ASSERT_MESSAGES_EQUAL(expectedRequest, apiRequest);
    }

    Y_UNIT_TEST(TestApplyRequestWithText) {
        const auto& request = TSpeechKitRequestBuilder(CreateSpeechKitRequest(SK_EVENT_TEXT_INPUT)).Build();

        const auto expectedRequest = JsonToProto<TScenarioApplyRequest>(
            CreateProtocolRequest(PROTOCOL_APPLY_TEXT)
        );

        google::protobuf::StringValue argumentsValue;
        *argumentsValue.mutable_value() = "some_payload";
        google::protobuf::Any arguments;
        arguments.PackFrom(argumentsValue);

        const auto& apiRequest = NImpl::ConstructApplyRequest(
            MakeConfig({EDataSourceType::BLACK_BOX}), request, /* seed= */ 42, REQUEST_LANG,
            /* session= */ {.ScenarioState = {}, .IsNewSession = false}, GetDefaultMementoData(),
            /* semanticFrames= */ {SemanticFrame}, arguments,
            CreateRequest(IEvent::CreateEvent(request.Event()), request), NORMALIZED_UTTERANCE,
            Nothing(), /* input= */ {},
            TWizardResponse());

        UNIT_ASSERT_MESSAGES_EQUAL(expectedRequest, apiRequest);
    }

    Y_UNIT_TEST(TestRunRequestWithVoice) {
        const auto& request = TSpeechKitRequestBuilder(CreateSpeechKitRequest(SK_EVENT_VOICE_INPUT)).Build();

        UNIT_ASSERT_EQUAL(request.Event().GetType(), voice_input);

        const auto expectedRequest = JsonToProto<TScenarioRunRequest>(CreateProtocolRequest(PROTOCOL_RUN_VOICE));

        TMockContext ctx;
        TScenarioInfraConfig infraConfig;
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillOnce(ReturnRef(infraConfig));
        IContext::TExpFlags expFlags;
        EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));
        TMockResponses responses;
        EXPECT_CALL(ctx, Responses()).WillOnce(ReturnRef(responses));
        TMockDataSources dataSources;
        NScenarios::TDataSource dialogHistory{};
        EXPECT_CALL(dataSources, GetDataSource(EDataSourceType::DIALOG_HISTORY)).WillOnce(ReturnRef(dialogHistory));

        const auto& apiRequest = NImpl::ConstructRunRequest(
            MakeConfig({EDataSourceType::DIALOG_HISTORY}), request, ctx, dataSources,
            /* seed= */ 42, REQUEST_LANG, /* session= */ {.ScenarioState = {}, .IsNewSession = false},
            GetDefaultMementoData(), /* semanticFrames= */ {SemanticFrame},
            CreateRequest(IEvent::CreateEvent(request.Event()), request), NORMALIZED_UTTERANCE,
            Nothing(), TRTLogger::NullLogger(), /* passDataSourcesInRequest= */ true);

        UNIT_ASSERT_MESSAGES_EQUAL(expectedRequest, apiRequest);
    }

    Y_UNIT_TEST(TestRunRequestWithServerAction) {
        const auto request = TSpeechKitRequestBuilder(CreateSpeechKitRequest(SK_EVENT_SERVER_ACTION)).Build();

        UNIT_ASSERT_EQUAL(request.Event().GetType(), server_action);

        const auto expectedRequest =
            JsonToProto<TScenarioRunRequest>(CreateProtocolRequest(PROTOCOL_RUN_SERVER_ACTION));

        TMockContext ctx;
        TScenarioInfraConfig infraConfig;
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillOnce(ReturnRef(infraConfig));
        IContext::TExpFlags expFlags;
        EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));
        TMockResponses responses;
        EXPECT_CALL(ctx, Responses()).WillOnce(ReturnRef(responses));
        TMockDataSources dataSources;
        NScenarios::TDataSource dialogHistory{};
        EXPECT_CALL(dataSources, GetDataSource(EDataSourceType::DIALOG_HISTORY)).WillOnce(ReturnRef(dialogHistory));
        const auto& apiRequest = NImpl::ConstructRunRequest(
            MakeConfig({EDataSourceType::DIALOG_HISTORY}), request, ctx, dataSources,
            /* seed= */ 42, REQUEST_LANG, /* session= */ {.ScenarioState = {}, .IsNewSession = false},
            GetDefaultMementoData(), /* semanticFrames= */ {SemanticFrame},
            CreateRequest(IEvent::CreateEvent(request.Event()), request), NORMALIZED_UTTERANCE,
            Nothing(), TRTLogger::NullLogger(), /* passDataSourcesInRequest= */ true);

        UNIT_ASSERT_MESSAGES_EQUAL(expectedRequest, apiRequest);
    }

    Y_UNIT_TEST(TestRunRequestWithImage) {
        const auto request = TSpeechKitRequestBuilder(CreateSpeechKitRequest(SK_EVENT_IMAGE_INPUT)).Build();

        UNIT_ASSERT_EQUAL(request.Event().GetType(), image_input);

        const auto expectedRequest = JsonToProto<TScenarioRunRequest>(CreateProtocolRequest(PROTOCOL_RUN_IMAGE_INPUT));

        TMockContext ctx;
        TScenarioInfraConfig infraConfig;
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillOnce(ReturnRef(infraConfig));
        IContext::TExpFlags expFlags;
        EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));
        TMockResponses responses;
        EXPECT_CALL(ctx, Responses()).WillOnce(ReturnRef(responses));
        TMockDataSources dataSources;
        NScenarios::TDataSource dialogHistory{};
        EXPECT_CALL(dataSources, GetDataSource(EDataSourceType::DIALOG_HISTORY)).WillOnce(ReturnRef(dialogHistory));
        const auto& apiRequest = NImpl::ConstructRunRequest(
            MakeConfig({EDataSourceType::DIALOG_HISTORY}), request, ctx, dataSources,
            /* seed= */ 42, REQUEST_LANG, /* session= */ {.ScenarioState = {}, .IsNewSession = false},
            GetDefaultMementoData(), /* semanticFrames= */ {SemanticFrame},
            CreateRequest(IEvent::CreateEvent(request.Event()), request), NORMALIZED_UTTERANCE,
            Nothing(), TRTLogger::NullLogger(), /* passDataSourcesInRequest= */ true);

        UNIT_ASSERT_MESSAGES_EQUAL(expectedRequest, apiRequest);
    }

    Y_UNIT_TEST(TestRunRequestWithMusic) {
        const auto& request = TSpeechKitRequestBuilder(CreateSpeechKitRequest(SK_EVENT_MUSIC_INPUT)).Build();

        UNIT_ASSERT_EQUAL(request.Event().GetType(), music_input);

        const auto expectedRequest = JsonToProto<TScenarioRunRequest>(CreateProtocolRequest(PROTOCOL_RUN_MUSIC_INPUT));

        TMockContext ctx;
        TScenarioInfraConfig infraConfig;
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillOnce(ReturnRef(infraConfig));
        IContext::TExpFlags expFlags;
        EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));
        TMockResponses responses;
        EXPECT_CALL(ctx, Responses()).WillOnce(ReturnRef(responses));
        TMockDataSources dataSources;
        NScenarios::TDataSource dialogHistory{};
        EXPECT_CALL(dataSources, GetDataSource(EDataSourceType::DIALOG_HISTORY)).WillOnce(ReturnRef(dialogHistory));
        const auto& apiRequest = NImpl::ConstructRunRequest(
            MakeConfig({EDataSourceType::DIALOG_HISTORY}), request, ctx, dataSources,
            /* seed= */ 42, REQUEST_LANG, /* session= */ {.ScenarioState = {}, .IsNewSession = false},
            GetDefaultMementoData(), /* semanticFrames= */ {SemanticFrame},
            CreateRequest(IEvent::CreateEvent(request.Event()), request), NORMALIZED_UTTERANCE,
            Nothing(), TRTLogger::NullLogger(), /* passDataSourcesInRequest= */ true);

        UNIT_ASSERT_MESSAGES_EQUAL(expectedRequest, apiRequest);
    }

    Y_UNIT_TEST(TestFiltrationLevel) {
        TMaybe<ui32> filtrationLevels[] = {Nothing(), 0, 1, 2};

        for (const auto filtrationLevel : filtrationLevels) {
            NJson::TJsonValue requestJson = JsonFromString(TRIVIAL_SK_REQUEST);
            if (filtrationLevel) {
                requestJson["request"]["additional_options"]["bass_options"]["filtration_level"] = *filtrationLevel;
            }
            const auto& request = TSpeechKitRequestBuilder(requestJson).Build();

            NScenarios::TScenarioBaseRequest baseRequest;
            NImpl::ConstructBaseRequest(
                &baseRequest, /* config= */ {}, request, /* seed= */ 42, REQUEST_LANG,
                /* session= */ {.ScenarioState = {}, .IsNewSession = false}, GetDefaultMementoData(),
                /* requestModel= */ CreateRequest(IEvent::CreateEvent(request.Event()), request),
                TWizardResponse());
            UNIT_ASSERT_VALUES_EQUAL(filtrationLevel.GetOrElse(1), baseRequest.GetOptions().GetFiltrationLevel());
        }
    }

    Y_UNIT_TEST(TestPermissions) {
        NJson::TJsonValue requestJson = JsonFromString(TRIVIAL_SK_REQUEST);

        auto& permissions = requestJson["request"]["additional_options"]["permissions"];
        {
            auto& permission = permissions[0];
            permission["name"] = TStringBuf("location");
            permission["granted"] = true;
        }
        {
            auto& permission = permissions[1];
            permission["name"] = TStringBuf("call");
            permission["granted"] = false;
        }

        const auto& request = TSpeechKitRequestBuilder(requestJson).Build();

        NScenarios::TScenarioBaseRequest baseRequest;
        NImpl::ConstructBaseRequest(&baseRequest, /* config= */ {}, request, /* seed= */ 42, REQUEST_LANG,
                                    /* session= */ {.ScenarioState = {}, .IsNewSession = false},
                                    GetDefaultMementoData(),
                                    /* requestModel= */ CreateRequest(IEvent::CreateEvent(request.Event()), request),
                                    TWizardResponse());

        UNIT_ASSERT_VALUES_EQUAL(baseRequest.GetOptions().GetPermissions(0).GetName(), "location");
        UNIT_ASSERT(baseRequest.GetOptions().GetPermissions(0).GetGranted());

        UNIT_ASSERT_VALUES_EQUAL(baseRequest.GetOptions().GetPermissions(1).GetName(), "call");
        UNIT_ASSERT(!baseRequest.GetOptions().GetPermissions(1).GetGranted());
    }

    Y_UNIT_TEST(TestFavaoriteLocations) {
        NJson::TJsonValue requestJson = JsonFromString(TRIVIAL_SK_REQUEST);
        auto& favourites = requestJson["request"]["additional_options"]["favourites"];
        TFavouriteLocation protoFavourites[2];
        {
            auto& favourite = favourites[0];
            auto& protoFavourite = protoFavourites[0];
            favourite["title"] = TStringBuf("Home");
            protoFavourite.SetTitle("Home");
            favourite["subtitle"] = TStringBuf("Дом");
            protoFavourite.SetSubTitle("Дом");
            favourite["lat"] = 13.37;
            protoFavourite.SetLat(13.37);
            favourite["lon"] = 73.57;
            protoFavourite.SetLon(73.57);
        }
        {
            auto& favourite = favourites[1];
            auto& protoFavourite = protoFavourites[1];
            favourite["title"] = TStringBuf("Work");
            protoFavourite.SetTitle("Work");
            favourite["lat"] = 73.57;
            protoFavourite.SetLat(73.57);
            favourite["lon"] = 13.37;
            protoFavourite.SetLon(13.37);
        }

        const auto& request = TSpeechKitRequestBuilder(requestJson).Build();

        NScenarios::TScenarioBaseRequest baseRequest;
        NImpl::ConstructBaseRequest(&baseRequest, /* config= */ {}, request, /* seed= */ 42, REQUEST_LANG,
                                    /* session= */ {.ScenarioState = {}, .IsNewSession = false},
                                    GetDefaultMementoData(),
                                    /* requestModel= */ CreateRequest(IEvent::CreateEvent(request.Event()), request),
                                    TWizardResponse());

        UNIT_ASSERT_MESSAGES_EQUAL(protoFavourites[0], baseRequest.GetOptions().GetFavouriteLocations(0));
        UNIT_ASSERT_MESSAGES_EQUAL(protoFavourites[1], baseRequest.GetOptions().GetFavouriteLocations(1));
    }

    Y_UNIT_TEST(TestRunRequestWithUserLocation) {
        const auto request = TSpeechKitRequestBuilder(CreateSpeechKitRequest(SK_EVENT_MUSIC_INPUT)).Build();

        UNIT_ASSERT_EQUAL(request.Event().GetType(), music_input);

        const auto expectedRequest = JsonToProto<TScenarioRunRequest>(CreateProtocolRequest(PROTOCOL_RUN_MUSIC_INPUT,
                                                                                            DATA_SOURCES_WITH_USER_LOCATION));

        TUserLocation location{"ru", NGeoDB::MOSCOW_ID, "Europe/Moscow", NGeoDB::RUSSIA_ID};
        TMockResponses responses;
        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources(&responses,
                                 /* userLocation= */ &location,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr, TRTLogger::NullLogger(),
                                 /* deviceState= */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing());

        TMockContext ctx;
        EXPECT_CALL(ctx, Responses()).WillOnce(ReturnRef(responses));
        TScenarioInfraConfig infraConfig;
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillOnce(ReturnRef(infraConfig));
        IContext::TExpFlags expFlags;
        EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));
        const auto& apiRequest = NImpl::ConstructRunRequest(
            MakeConfig({EDataSourceType::USER_LOCATION}), request, ctx, dataSources,
            /* seed= */ 42, REQUEST_LANG, /* session= */ {.ScenarioState = {}, .IsNewSession = false},
            GetDefaultMementoData(), /* semanticFrames= */ {SemanticFrame},
            CreateRequest(IEvent::CreateEvent(request.Event()), request), NORMALIZED_UTTERANCE,
            Nothing(), TRTLogger::NullLogger(), /* passDataSourcesInRequest= */ true);

        UNIT_ASSERT_MESSAGES_EQUAL(expectedRequest, apiRequest);
    }

    Y_UNIT_TEST(TestRunRequestWithSmartHome) {
        const auto request = TSpeechKitRequestBuilder(CreateSpeechKitRequest(SK_EVENT_VOICE_INPUT)).Build();

        TMockResponses responses;
        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources(&responses,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr, &request->GetRequest().GetSmartHomeInfo(),
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr, TRTLogger::NullLogger(),
                                 /* deviceState= */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing());

        TMockContext ctx;
        EXPECT_CALL(ctx, Responses()).WillOnce(ReturnRef(responses));
        TScenarioInfraConfig infraConfig;
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillOnce(ReturnRef(infraConfig));
        IContext::TExpFlags expFlags;
        EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));
        const auto apiRequest = NImpl::ConstructRunRequest(
            MakeConfig({EDataSourceType::SMART_HOME_INFO}), request, ctx, dataSources,
            /* seed= */ 42, REQUEST_LANG, /* session= */ {.ScenarioState = {}, .IsNewSession = false},
            GetDefaultMementoData(), /* semanticFrames= */ {SemanticFrame},
            CreateRequest(IEvent::CreateEvent(request.Event()), request), NORMALIZED_UTTERANCE,
            Nothing(), TRTLogger::NullLogger(), /* passDataSourcesInRequest= */ true);

        const auto expectedRequest = JsonToProto<TScenarioRunRequest>(
            CreateProtocolRequest(PROTOCOL_RUN_VOICE, DATA_SOURCES_WITH_SMART_HOME));

        UNIT_ASSERT_MESSAGES_EQUAL(expectedRequest, apiRequest);
    }

    Y_UNIT_TEST(TestRunRequestWithBegemotMarkup) {
        const auto request = TSpeechKitRequestBuilder(CreateSpeechKitRequest(SK_EVENT_MUSIC_INPUT)).Build();

        UNIT_ASSERT_EQUAL(request.Event().GetType(), music_input);

        const auto expectedRequest = JsonToProto<TScenarioRunRequest>(CreateProtocolRequest(PROTOCOL_RUN_MUSIC_INPUT,
                                                                                            DATA_SOURCES_WITH_BEGEMOT_MARKUP));

        const auto begemotResponseJson = JsonFromString(BEGEMOT_MARKUP_RESPONSE);
        auto begemotResponse = JsonToProto<NBg::NProto::TAlicePolyglotMergeResponseResult>(begemotResponseJson);
        TMockResponses responses;
        responses.SetWizardResponse(TWizardResponse(std::move(begemotResponse)));
        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources(&responses,
                                 /* userLocation= */ nullptr,
                                 /* dialogHistory= */ nullptr,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr, TRTLogger::NullLogger(),
                                 /* deviceState= */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing());

        TMockContext ctx;
        TScenarioInfraConfig infraConfig;
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillOnce(ReturnRef(infraConfig));
        IContext::TExpFlags expFlags;
        EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));
        EXPECT_CALL(ctx, Responses()).WillOnce(ReturnRef(responses));

        const auto& apiRequest = NImpl::ConstructRunRequest(
            MakeConfig({EDataSourceType::BEGEMOT_EXTERNAL_MARKUP}), request, ctx, dataSources,
            /* seed= */ 42, REQUEST_LANG, /* session= */ {.ScenarioState = {}, .IsNewSession = false},
            GetDefaultMementoData(), /* semanticFrames= */ {SemanticFrame},
            CreateRequest(IEvent::CreateEvent(request.Event()), request), NORMALIZED_UTTERANCE,
            Nothing(), TRTLogger::NullLogger(), /* passDataSourcesInRequest= */ true);

        UNIT_ASSERT_MESSAGES_EQUAL(expectedRequest, apiRequest);
    }

    Y_UNIT_TEST(TestRunRequestWithDialogHistory) {
        const auto request = TSpeechKitRequestBuilder(CreateSpeechKitRequest(SK_EVENT_MUSIC_INPUT)).Build();

        UNIT_ASSERT_EQUAL(request.Event().GetType(), music_input);

        const auto expectedRequest = JsonToProto<TScenarioRunRequest>(
            CreateProtocolRequest(PROTOCOL_RUN_MUSIC_INPUT, DATA_SOURCES_WITH_DIALOG_HISTORY_WITHOUT_PREV_RESPONSE)
        );

        TDialogHistory dialogHistory({{"Привет!", "Привет!", "Хэллоу", "SomeScenario", 0, 1}, {"Как дела?", "Как дела?", "Норм", "SomeScenario", 2, 3}});

        TMockResponses responses;
        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources(&responses,
                                 /* userLocation= */ nullptr, &dialogHistory,
                                 /* actions = */ nullptr,
                                 /* layout = */ nullptr,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr, TRTLogger::NullLogger(),
                                 /* deviceState= */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing());

        TMockContext ctx;
        EXPECT_CALL(ctx, Responses()).WillOnce(ReturnRef(responses));
        TScenarioInfraConfig infraConfig;
        infraConfig.MutableDialogManagerParams()->SetDialogHistoryAllowed(true);
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillOnce(ReturnRef(infraConfig));
        IContext::TExpFlags expFlags;
        EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));

        const auto& apiRequest = NImpl::ConstructRunRequest(
            MakeConfig({EDataSourceType::DIALOG_HISTORY}), request, ctx, dataSources,
            /* seed= */ 42, REQUEST_LANG, /* session= */ {.ScenarioState = {}, .IsNewSession = false},
            GetDefaultMementoData(), /* semanticFrames= */ {SemanticFrame},
            CreateRequest(IEvent::CreateEvent(request.Event()), request), NORMALIZED_UTTERANCE,
            Nothing(), TRTLogger::NullLogger(), /* passDataSourcesInRequest= */ true);

        UNIT_ASSERT_MESSAGES_EQUAL(expectedRequest, apiRequest);
    }

    Y_UNIT_TEST(TestRunRequestWithDialogHistoryWhichHasPrevResponse) {
        const auto request = TSpeechKitRequestBuilder(CreateSpeechKitRequest(SK_EVENT_MUSIC_INPUT)).Build();

        UNIT_ASSERT_EQUAL(request.Event().GetType(), music_input);

        const auto expectedRequest = JsonToProto<TScenarioRunRequest>(
            CreateProtocolRequest(PROTOCOL_RUN_MUSIC_INPUT, DATA_SOURCES_WITH_DIALOG_HISTORY_WITH_PREV_RESPONSE)
        );

        TLayout layout;
        layout.SetOutputSpeech("Хэллоу");
        google::protobuf::Map<TString, NScenarios::TFrameAction> actions;
        actions["action"].MutableFrame()->SetName("semantic_frame");

        TDialogHistory dialogHistory({{"Привет!", "Привет!", "Хэллоу", "SomeScenario", 0, 1}, {"Как дела?", "Как дела?", "Норм", "SomeScenario", 2, 3}});

        TMockResponses responses;
        NiceMock<TMockGlobalContext> globalCtx;
        NTesting::TTestAppHostCtx ahCtx{globalCtx};
        TDataSources dataSources(&responses,
                                 /* userLocation= */ nullptr, &dialogHistory, &actions, &layout,
                                 /* smartHomeInfo= */ nullptr,
                                 /* videoViewState= */ nullptr,
                                 /* notificationState= */ nullptr,
                                 /* skillDiscoverySaasCandidates= */ nullptr,
                                 /* auxiliaryConfig= */ nullptr, TRTLogger::NullLogger(),
                                 /* deviceState= */ {},
                                 /* ioTUserInfo= */ nullptr,
                                 /* appInfo= */ nullptr,
                                 ahCtx.ItemProxyAdapter(),
                                 /* rawPersonalData= */ {},
                                 /* videoCurrentlyPlaying= */ nullptr,
                                 /* contactsList= */ nullptr,
                                 /* environmentState= */ nullptr,
                                 /* tandemEnvironmentState= */ nullptr,
                                 /* webSearchQuery */ "",
                                 /* whisperInfo= */ Nothing(),
                                 /* guestData= */ Nothing(),
                                 /* guestOptions= */ Nothing());

        TMockContext ctx;
        EXPECT_CALL(ctx, Responses()).WillOnce(ReturnRef(responses));
        TScenarioInfraConfig infraConfig;
        infraConfig.MutableDialogManagerParams()->SetDialogHistoryAllowed(true);
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillOnce(ReturnRef(infraConfig));
        IContext::TExpFlags expFlags;
        EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));

        const auto& apiRequest = NImpl::ConstructRunRequest(
            MakeConfig({EDataSourceType::DIALOG_HISTORY, EDataSourceType::RESPONSE_HISTORY}), request, ctx,
            dataSources, /* seed= */ 42, REQUEST_LANG, /* session= */ {.ScenarioState = {}, .IsNewSession = false},
            GetDefaultMementoData(), /* semanticFrames= */ {SemanticFrame},
            CreateRequest(IEvent::CreateEvent(request.Event()), request), NORMALIZED_UTTERANCE,
            Nothing(), TRTLogger::NullLogger(), /* passDataSourcesInRequest= */ true);

        UNIT_ASSERT_MESSAGES_EQUAL(expectedRequest, apiRequest);
    }

    Y_UNIT_TEST(TestLoadLaasRegionIfLocationIsNotProvided) {
        NJson::TJsonValue requestJson = JsonFromString(TRIVIAL_SK_REQUEST);
        requestJson["request"]["laas_region"] = NJson::ReadJsonFastTree(TStringBuf(R"({
            "latitude": 55.753215,
            "longitude": 37.622504,
            "location_accuracy": 15000,
            "location_unixtime": 1582618722
        })"));

        const auto& request = TSpeechKitRequestBuilder(requestJson).Build();

        NScenarios::TScenarioBaseRequest baseRequest;
        NImpl::ConstructBaseRequest(&baseRequest, /* config= */ {}, request, /* seed= */ 42, REQUEST_LANG,
                                    /* session= */ {.ScenarioState = {}, .IsNewSession = false},
                                    GetDefaultMementoData(),
                                    /* requestModel= */ CreateRequest(IEvent::CreateEvent(request.Event()), request),
                                    TWizardResponse());

        UNIT_ASSERT_DOUBLES_EQUAL(baseRequest.GetLocation().GetLat(), 55.753215, 1e-8);
        UNIT_ASSERT_DOUBLES_EQUAL(baseRequest.GetLocation().GetLon(), 37.622504, 1e-8);
        UNIT_ASSERT_VALUES_EQUAL(baseRequest.GetLocation().GetAccuracy(), 15000);
    }

    Y_UNIT_TEST(TestPreferLaasRegionToLocation) {
        NJson::TJsonValue requestJson = JsonFromString(TRIVIAL_SK_REQUEST);

        requestJson["request"]["laas_region"] = NJson::ReadJsonFastTree(TStringBuf(R"({
            "latitude": 50,
            "longitude": 30,
            "location_accuracy": 10,
            "location_unixtime": 1582618722
        })"));

        requestJson["request"]["location"] = NJson::ReadJsonFastTree(TStringBuf(R"({
            "lat": 60,
            "lon": 40,
            "accuracy": 20,
            "location_unixtime": 1582618722
        })"));

        const auto& request = TSpeechKitRequestBuilder(requestJson).Build();

        NScenarios::TScenarioBaseRequest baseRequest;
        NImpl::ConstructBaseRequest(&baseRequest, /* config= */ {}, request, /* seed= */ 42, REQUEST_LANG,
                                    /* session= */ {.ScenarioState = {}, .IsNewSession = false},
                                    GetDefaultMementoData(),
                                    /* requestModel= */ CreateRequest(IEvent::CreateEvent(request.Event()), request),
                                    TWizardResponse());

        UNIT_ASSERT_VALUES_EQUAL(baseRequest.GetLocation().GetLat(), 50);
        UNIT_ASSERT_VALUES_EQUAL(baseRequest.GetLocation().GetLon(), 30);
    }

    Y_UNIT_TEST(TestPreferLocationToLaasRegion) {
        NJson::TJsonValue requestJson = JsonFromString(TRIVIAL_SK_REQUEST);

        requestJson["request"]["laas_region"] = NJson::ReadJsonFastTree(TStringBuf(R"({
            "latitude": 50,
            "longitude": 30,
            "location_accuracy": 20,
            "location_unixtime": 1582618722
        })"));

        requestJson["request"]["location"] = NJson::ReadJsonFastTree(TStringBuf(R"({
            "lat": 60,
            "lon": 40,
            "accuracy": 10,
            "location_unixtime": 1582618722
        })"));

        const auto& request = TSpeechKitRequestBuilder(requestJson).Build();

        NScenarios::TScenarioBaseRequest baseRequest;
        NImpl::ConstructBaseRequest(&baseRequest, /* config= */ {}, request, /* seed= */ 42, REQUEST_LANG,
                                    /* session= */ {.ScenarioState = {}, .IsNewSession = false},
                                    GetDefaultMementoData(),
                                    /* requestModel= */ CreateRequest(IEvent::CreateEvent(request.Event()), request),
                                    TWizardResponse());

        UNIT_ASSERT_VALUES_EQUAL(baseRequest.GetLocation().GetLat(), 60);
        UNIT_ASSERT_VALUES_EQUAL(baseRequest.GetLocation().GetLon(), 40);
    }

    Y_UNIT_TEST(TestDeviceIdOverrideByQuasar) {
        NJson::TJsonValue requestJson = JsonFromString(TRIVIAL_SK_REQUEST);

        requestJson["application"] = NJson::ReadJsonFastTree(TStringBuf(R"({
            "device_manufacturer": "Yandex",
            "device_model": "Station",
            "device_id": "4099798c8542c80a4f05ad611053fba8",
        })"));

        requestJson["request"]["device_state"] = NJson::ReadJsonFastTree(TStringBuf(R"({
            "device_id": "04107884c914600809cf"
        })"));

        const auto& request = TSpeechKitRequestBuilder(requestJson).Build();

        NScenarios::TScenarioBaseRequest baseRequest;
        NImpl::ConstructBaseRequest(&baseRequest, /* config= */ {}, request, /* seed= */ 42, REQUEST_LANG,
                                    /* session= */ {.ScenarioState = {}, .IsNewSession = false},
                                    GetDefaultMementoData(),
                                    /* requestModel= */ CreateRequest(IEvent::CreateEvent(request.Event()), request),
                                    TWizardResponse());
        NImpl::ConstructDeviceState(*baseRequest.MutableDeviceState(), /* config=*/ {}, request->GetRequest().GetDeviceState());

        UNIT_ASSERT_VALUES_EQUAL(baseRequest.GetClientInfo().GetDeviceId(), "04107884c914600809cf");
        UNIT_ASSERT_VALUES_EQUAL(baseRequest.GetDeviceState().GetDeviceId(), "04107884c914600809cf");
    }

    Y_UNIT_TEST(ConstructDeviceStateWithDataSource) {
        TScenarioConfig scenarioConfig{};
        TDeviceState sourceDeviceState{};
        sourceDeviceState.MutableVideo()->SetCurrentScreen("screen");
        sourceDeviceState.MutableNavigator()->AddStates("states");

        {
            TDeviceState deviceState{};
            NImpl::ConstructDeviceState(deviceState, scenarioConfig, sourceDeviceState);
            UNIT_ASSERT_MESSAGES_EQUAL(sourceDeviceState, deviceState);
        }

        {
            TDeviceState deviceState{};
            scenarioConfig.AddDataSources()->SetType(EDataSourceType::DEVICE_STATE_NAVIGATOR);
            NImpl::ConstructDeviceState(deviceState, scenarioConfig, sourceDeviceState);
            UNIT_ASSERT_MESSAGES_EQUAL(deviceState, TDeviceState::default_instance());
            scenarioConfig.ClearDataSources();
        }

        {
            TDeviceState deviceState{};
            scenarioConfig.AddDataSources()->SetType(EDataSourceType::EMPTY_DEVICE_STATE);
            NImpl::ConstructDeviceState(deviceState, scenarioConfig, sourceDeviceState);
            UNIT_ASSERT_MESSAGES_EQUAL(deviceState, TDeviceState::default_instance());
            scenarioConfig.ClearDataSources();
        }
    }

    Y_UNIT_TEST(ConstructSaasSkillDiscoverysSource) {
        const auto config = MakeConfig({EDataSourceType::SKILL_DISCOVERY_GC});

        {
            NScenarios::TSkillDiscoverySaasCandidates saasResult;
            saasResult.AddSaasCandidate()->SetSkillId("skill_id");

            NiceMock<TMockGlobalContext> globalCtx;
            NTesting::TTestAppHostCtx ahCtx{globalCtx};
            TDataSources dataSources(
                /* responses= */ nullptr,
                /* userLocation= */ nullptr,
                /* dialogHistory= */ nullptr,
                /* actions = */ nullptr,
                /* layout = */ nullptr,
                /* smartHomeInfo= */ nullptr,
                /* videoViewState= */ nullptr,
                /* notificationState= */ nullptr, &saasResult,
                /* auxiliaryConfig= */ nullptr, TRTLogger::NullLogger(),
                /* deviceState= */ {},
                /* ioTUserInfo= */ nullptr,
                /* appInfo= */ nullptr,
                ahCtx.ItemProxyAdapter(),
                /* rawPersonalData= */ {},
                /* videoCurrentlyPlaying= */ nullptr,
                /* contactsList= */ nullptr,
                /* environmentState= */ nullptr,
                /* tandemEnvironmentState= */ nullptr,
                /* webSearchQuery */ "",
                /* whisperInfo= */ Nothing(),
                /* guestData= */ Nothing(),
                /* guestOptions= */ Nothing());

            IContext::TExpFlags expFlags;
            ::google::protobuf::Map<::google::protobuf::int32, NScenarios::TDataSource> dataSourcesProto;
            NImpl::ConstructDataSources(&dataSourcesProto, config, dataSources, expFlags, /* dialogHistoryAllowed= */ false,
                                        /* passDataSourcesInRequest= */ true);

            UNIT_ASSERT_VALUES_EQUAL(1, dataSourcesProto.count(EDataSourceType::SKILL_DISCOVERY_GC));
            const auto& result = dataSourcesProto.at(EDataSourceType::SKILL_DISCOVERY_GC);

            UNIT_ASSERT_MESSAGES_EQUAL(saasResult, result.GetSkillDiscoveryGcSaasCandidates());
        }
        {
            NScenarios::TSkillDiscoverySaasCandidates saasResult;

            NiceMock<TMockGlobalContext> globalCtx;
            NTesting::TTestAppHostCtx ahCtx{globalCtx};
            TDataSources dataSources(
                /* responses= */ nullptr,
                /* userLocation= */ nullptr,
                /* dialogHistory= */ nullptr,
                /* actions = */ nullptr,
                /* layout = */ nullptr,
                /* smartHomeInfo= */ nullptr,
                /* videoViewState= */ nullptr,
                /* notificationState= */ nullptr, &saasResult,
                /* auxiliaryConfig= */ nullptr, TRTLogger::NullLogger(),
                /* deviceState= */ {},
                /* ioTUserInfo= */ nullptr,
                /* appInfo= */ nullptr,
                ahCtx.ItemProxyAdapter(),
                /* rawPersonalData= */ {},
                /* videoCurrentlyPlaying= */ nullptr,
                /* contactsList= */ nullptr,
                /* environmentState= */ nullptr,
                /* tandemEnvironmentState= */ nullptr,
                /* webSearchQuery */ "",
                /* whisperInfo= */ Nothing(),
                /* guestData= */ Nothing(),
                /* guestOptions= */ Nothing());

            IContext::TExpFlags expFlags;
            ::google::protobuf::Map<::google::protobuf::int32, NScenarios::TDataSource> dataSourcesProto;
            NImpl::ConstructDataSources(&dataSourcesProto, config, dataSources, expFlags, /* dialogHistoryAllowed= */ false,
                                        /* passDataSourcesInRequest= */ true);

            UNIT_ASSERT_VALUES_EQUAL(0, dataSourcesProto.count(EDataSourceType::SKILL_DISCOVERY_GC));
        }

    }

    Y_UNIT_TEST(TestFiltrationMode) {
        constexpr auto EnsureCorrectFiltrationMode = [](const auto& jsonRequest, const auto& expected) {
            const auto request = TSpeechKitRequestBuilder(jsonRequest).Build();
            NScenarios::TScenarioBaseRequest baseRequest;
            NImpl::ConstructBaseRequest(
                &baseRequest, /* config= */ {}, request, /* seed= */ 42, REQUEST_LANG,
                /* session= */ {.ScenarioState = {}, .IsNewSession = false}, GetDefaultMementoData(),
                /* requestModel= */ CreateRequest(IEvent::CreateEvent(request.Event()), request),
                TWizardResponse());

            UNIT_ASSERT_VALUES_EQUAL(static_cast<int>(baseRequest.GetUserPreferences().GetFiltrationMode()),
                                     static_cast<int>(expected));
        };

        for (const auto [level, expected] : THashMap<int, NScenarios::TUserPreferences::EFiltrationMode>{
                 {0, NScenarios::TUserPreferences_EFiltrationMode_NoFilter},
                 {1, NScenarios::TUserPreferences_EFiltrationMode_Moderate},
                 {2, NScenarios::TUserPreferences_EFiltrationMode_FamilySearch},
             }) {
            NJson::TJsonValue request = JsonFromString(TRIVIAL_SK_REQUEST);
            request["request"]["additional_options"]["bass_options"]["filtration_level"] = level;

            EnsureCorrectFiltrationMode(request, expected);
        }

        for (const auto [level, expected] : THashMap<TStringBuf, NScenarios::TUserPreferences::EFiltrationMode>{
                 {TStringBuf("without"), NScenarios::TUserPreferences_EFiltrationMode_NoFilter},
                 {TStringBuf("medium"), NScenarios::TUserPreferences_EFiltrationMode_Moderate},
                 {TStringBuf("children"), NScenarios::TUserPreferences_EFiltrationMode_FamilySearch},
             }) {
            NJson::TJsonValue request = JsonFromString(TRIVIAL_SK_REQUEST);
            request["request"]["device_state"]["device_config"]["content_settings"] = level;

            EnsureCorrectFiltrationMode(request, expected);
        }
    }

    Y_UNIT_TEST(TestVideoViewState) {
        const auto requestPageState = TSpeechKitRequestBuilder(CreateSpeechKitRequestWithVideoState("page_state", MORDOVIA_VIDEO_VIEW_STATE)).Build();
        const auto requestViewState = TSpeechKitRequestBuilder(CreateSpeechKitRequestWithVideoState("view_state", MORDOVIA_VIDEO_VIEW_STATE)).Build();

        NScenarios::TScenarioBaseRequest baseRequestPageState;
        NImpl::ConstructBaseRequest(
            &baseRequestPageState, /* config= */ {}, requestPageState, /* seed= */ 42, REQUEST_LANG,
            /* session= */ {.ScenarioState = {}, .IsNewSession = false}, GetDefaultMementoData(),
            /* requestModel= */ CreateRequest(IEvent::CreateEvent(requestPageState.Event()), requestPageState),
            TWizardResponse());

        NScenarios::TScenarioBaseRequest baseRequestViewState;
        NImpl::ConstructBaseRequest(
            &baseRequestViewState, /* config= */ {}, requestViewState, /* seed= */ 42, REQUEST_LANG,
            /* session= */ {.ScenarioState = {}, .IsNewSession = false}, GetDefaultMementoData(),
            /* requestModel= */ CreateRequest(IEvent::CreateEvent(requestViewState.Event()), requestViewState),
            TWizardResponse());

        const auto& viewStateFromPageState = baseRequestPageState.GetDeviceState().GetVideo().GetViewState();
        const auto& viewStateFromSpeechkitRequest = baseRequestViewState.GetDeviceState().GetVideo().GetViewState();
        UNIT_ASSERT_MESSAGES_EQUAL(viewStateFromPageState, viewStateFromSpeechkitRequest);
    }

    Y_UNIT_TEST(TestImageInputCaptureModes) {
        auto request = NJson::ReadJsonFastTree(BASE_SK_REQUEST);
        request["request"].InsertValue("event", NJson::ReadJsonFastTree(SK_EVENT_IMAGE_INPUT));

        using EImageCaptureMode = NScenarios::TInput::TImage::ECaptureMode;

        const THashMap<TString, EImageCaptureMode> modes = {
            {"", EImageCaptureMode::TInput_TImage_ECaptureMode_Undefined},
            {"voice_text", EImageCaptureMode::TInput_TImage_ECaptureMode_OcrVoice},
            {"text", EImageCaptureMode::TInput_TImage_ECaptureMode_Ocr},
            {"photo", EImageCaptureMode::TInput_TImage_ECaptureMode_Photo},
            {"market", EImageCaptureMode::TInput_TImage_ECaptureMode_Market},
            {"document", EImageCaptureMode::TInput_TImage_ECaptureMode_Document},
            {"clothes", EImageCaptureMode::TInput_TImage_ECaptureMode_Clothes},
            {"details", EImageCaptureMode::TInput_TImage_ECaptureMode_Details},
            {"similar_like", EImageCaptureMode::TInput_TImage_ECaptureMode_SimilarLike},
            {"similar_people", EImageCaptureMode::TInput_TImage_ECaptureMode_SimilarPeople},
            {"similar_people_frontal", EImageCaptureMode::TInput_TImage_ECaptureMode_SimilarPeopleFrontal},
            {"barcode", EImageCaptureMode::TInput_TImage_ECaptureMode_Barcode},
            {"translate", EImageCaptureMode::TInput_TImage_ECaptureMode_Translate},
            {"similar_artwork", EImageCaptureMode::TInput_TImage_ECaptureMode_SimilarArtwork}
        };

        for (const auto& [mode, expectedMode] : modes) {
            request["request"]["event"]["payload"]["capture_mode"] = mode;
            const auto skRequest = TSpeechKitRequestBuilder(request).Build();

            TMockContext ctx;
            TScenarioInfraConfig infraConfig;
            EXPECT_CALL(ctx, ScenarioConfig(_)).WillOnce(ReturnRef(infraConfig));
            IContext::TExpFlags expFlags;
            EXPECT_CALL(ctx, ExpFlags()).WillOnce(ReturnRef(expFlags));
            TMockResponses responses;
            EXPECT_CALL(ctx, Responses()).WillOnce(ReturnRef(responses));
            TMockDataSources dataSources;
            const auto& apiRequest = NImpl::ConstructRunRequest(
                MakeConfig({}), skRequest, ctx, dataSources,
                /* seed= */ 42, REQUEST_LANG, /* session= */ {.ScenarioState = {}, .IsNewSession = false},
                GetDefaultMementoData(), /* semanticFrames= */ {SemanticFrame},
                CreateRequest(IEvent::CreateEvent(skRequest.Event()), skRequest), NORMALIZED_UTTERANCE,
                Nothing(), TRTLogger::NullLogger(), /* passDataSourcesInRequest= */ true);
            const auto& actualCaptureMode = apiRequest.GetInput().GetImage().GetCaptureMode();
            UNIT_ASSERT_EQUAL_C(actualCaptureMode, expectedMode,
                "Expected " << NScenarios::TInput::TImage::ECaptureMode_Name(expectedMode) <<
                " but found " << NScenarios::TInput::TImage::ECaptureMode_Name(actualCaptureMode));
        }
    }

    Y_UNIT_TEST(TestAgeUserClassification) {
        NJson::TJsonValue requestJson = JsonFromString(R"({
            "request": {
                "event": {
                    "name": "",
                    "type": "text_input",
                    "text": "давай поиграем в города",
                    "biometry_classification": {
                        "simple": [{
                            "classname": "child",
                            "tag": "children"
                        }]
                    }
                }
            }
        })");


        const auto& request = TSpeechKitRequestBuilder(requestJson).Build();

        NScenarios::TScenarioBaseRequest baseRequest;
        auto event = IEvent::CreateEvent(request.Event());
        const auto requestModel = CreateRequest(std::move(event), request);
        NImpl::ConstructBaseRequest(&baseRequest, /* config= */ {}, request, /* seed= */ 42, REQUEST_LANG,
                                    /* session= */ {.ScenarioState = {}, .IsNewSession = false},
                                    GetDefaultMementoData(), requestModel,
                                    TWizardResponse());

        const auto& actualAge = baseRequest.GetUserClassification().GetAge();
        UNIT_ASSERT_EQUAL_C(actualAge, TUserClassification_EAge_Child,
                            "got " << TUserClassification_EAge_Name(actualAge) << " instead");
    }

    Y_UNIT_TEST(TestGenderUserClassification) {
        const TStringBuf unknown = "unknown";
        const TStringBuf male = "male";
        const TStringBuf female = "female";

        for (const TStringBuf& gender : {unknown, male, female}) {
            NJson::TJsonValue requestJson = JsonFromString(R"({
                "request": {
                    "event": {
                        "name": "",
                        "type": "text_input",
                        "text": "давай поиграем в города",
                        "biometry_classification": {
                            "simple": [{
                                "classname": "unknown",
                                "tag": "gender"
                            }]
                        }
                    }
                }
            })");
            requestJson["request"]["event"]["biometry_classification"]["simple"][0]["classname"] = gender;
            const auto& request = TSpeechKitRequestBuilder(requestJson).Build();

            NScenarios::TScenarioBaseRequest baseRequest;
            auto event = IEvent::CreateEvent(request.Event());
            const auto requestModel = CreateRequest(std::move(event), request);
            NImpl::ConstructBaseRequest(&baseRequest, {}, request, /* seed= */ 924, REQUEST_LANG,
                /* session= */ {.ScenarioState = {}, .IsNewSession = false},
                GetDefaultMementoData(), requestModel, TWizardResponse());

            const auto& actualGender = baseRequest.GetUserClassification().GetGender();
            UNIT_ASSERT_EQUAL_C(
                actualGender, (
                    gender == unknown ? TUserClassification_EGender_Unknown : (
                        gender == male ? TUserClassification_EGender_Male : TUserClassification_EGender_Female
                    )
                ), "got " << TUserClassification_EGender_Name(actualGender) << " instead"
            );
        }
    }

    Y_UNIT_TEST(TestMemento) {
        auto requestJson = CreateSpeechKitRequest(SK_EVENT_VOICE_INPUT);
        requestJson["request"]["experiments"][EXP_ENABLE_MEMENTO_SURFACE_DATA] = "1";
        const auto& request = TSpeechKitRequestBuilder(std::move(requestJson)).Build();

        auto event = IEvent::CreateEvent(request.Event());
        const auto requestModel = CreateRequest(std::move(event), request);
        NMegamind::NMementoApi::TRespGetAllObjects mementoResponse;
        mementoResponse.MutableUserConfigs()->MutableConfigForTests()->SetDefaultSource("source");
        auto& scenarioData = *mementoResponse.MutableScenarioData();

        const TString currentScenarioName = "scenarioA";
        const TString currentDeviceId = "deviceIdA";
        const TString anotherScenarioName = "scenarioB";
        const TString anotherDeviceId = "deviceIdB";

        auto& surfaceScenarioDataMap = *mementoResponse.MutableSurfaceScenarioData();
        auto& surfaceScenarioData = *surfaceScenarioDataMap[currentDeviceId].MutableScenarioData();

        for (const auto& scenarioName : {currentScenarioName, anotherScenarioName}) {
            google::protobuf::StringValue value;
            *value.mutable_value() = scenarioName + "_Data";
            scenarioData[scenarioName].PackFrom(value);
            surfaceScenarioData[scenarioName].PackFrom(value);
        }

        NMegamind::TMementoData mementoData{mementoResponse};
        NMegamind::TMementoDataView mementoDataView{mementoData, currentScenarioName, currentDeviceId, Default<TString>()};

        NScenarios::TScenarioBaseRequest baseRequest;
        TScenarioConfig scenarioConfig{};
        scenarioConfig.AddMementoUserConfigs()->SetConfigKey(
            ru::yandex::alice::memento::proto::EConfigKey::CK_CONFIG_FOR_TESTS);
        NImpl::ConstructBaseRequest(&baseRequest, scenarioConfig, request, /* seed= */ 42, REQUEST_LANG,
                                    /* session= */ {.ScenarioState = {}, .IsNewSession = false}, mementoDataView,
                                    requestModel,
                                    TWizardResponse());

        auto expectedRequest = JsonToProto<TScenarioRunRequest>(CreateProtocolRequest(PROTOCOL_RUN_VOICE));
        expectedRequest.MutableBaseRequest()->ClearDeviceState();
        auto& exps = *expectedRequest.MutableBaseRequest()->MutableExperiments()->mutable_fields();
        exps[TString{EXP_ENABLE_MEMENTO_SURFACE_DATA}].set_string_value("1");
        const auto mementoJson = NJson::ReadJsonFastTree(R"(
            {
                "user_configs": {
                },
                "scenario_data": {
                    "@type": "type.googleapis.com/google.protobuf.StringValue",
                    "value": "scenarioA_Data"
                },
                "surface_scenario_data": {
                    "@type": "type.googleapis.com/google.protobuf.StringValue",
                    "value": "scenarioA_Data"
                }
            }
        )");
        auto expectedMemento = JsonToProto<NScenarios::TMementoData>(mementoJson);
        expectedMemento.MutableUserConfigs()->MutableConfigForTests()->SetDefaultSource("source");
        expectedRequest.MutableBaseRequest()->MutableMemento()->CopyFrom(expectedMemento);

        UNIT_ASSERT_MESSAGES_EQUAL(expectedRequest.GetBaseRequest(), baseRequest);
    }

    Y_UNIT_TEST(TestConstructMomentoDataSources) {
        using ru::yandex::alice::memento::proto::EConfigKey;
        using ru::yandex::alice::memento::proto::TUserConfigs;

        const auto sourceUserConfigs = [] {
            TUserConfigs userConfigs{};
            userConfigs.MutableConfigForTests()->SetDefaultSource("source");
            return userConfigs;
        }();
        const auto sourceUserConfigsLarge = [](TUserConfigs configs) {
            configs.MutableNewConfig()->SetDefaultSource("source");
            return configs;
        }(sourceUserConfigs);

        TScenarioConfig scenarioConfig{};

        TUserConfigs userConfigs{};
        NImpl::ConstructMementoUserConfigs(userConfigs, scenarioConfig, sourceUserConfigsLarge);
        UNIT_ASSERT_MESSAGES_EQUAL(userConfigs, TUserConfigs::default_instance());

        scenarioConfig.AddMementoUserConfigs()->SetConfigKey(EConfigKey::CK_CONFIG_FOR_TESTS);
        NImpl::ConstructMementoUserConfigs(userConfigs, scenarioConfig, sourceUserConfigsLarge);
        UNIT_ASSERT_MESSAGES_EQUAL(userConfigs, sourceUserConfigs);

        userConfigs.Clear();

        scenarioConfig.AddMementoUserConfigs()->SetConfigKey(EConfigKey::CK_NEWS);
        NImpl::ConstructMementoUserConfigs(userConfigs, scenarioConfig, sourceUserConfigsLarge);
        UNIT_ASSERT_MESSAGES_EQUAL(userConfigs, sourceUserConfigsLarge);
    }

    Y_UNIT_TEST(TestConstructNluFeatures) {
        TNluFeatureParams featureParams;

        // the only livelong enum, it shouldn't be used but it's the only way to test code
        const NNluFeatures::ENluFeature feature = NNluFeatures::ENluFeature::RESERVED;
        featureParams.SetFeature(feature);

        TScenarioConfig scenarioConfig{};
        scenarioConfig.MutableNluFeatures()->Add(std::move(featureParams));

        NBg::NProto::TAlicePolyglotMergeResponseResult begemotResponse;
        const float featureValue = 0.5F;
        begemotResponse.MutableAliceResponse()->MutableAliceNluFeatures()->MutableFeatureContainer()->MutableFeatures()->Add(featureValue);

        const TWizardResponse wizardResponse(std::move(begemotResponse));

        ::google::protobuf::RepeatedPtrField<NScenarios::TNluFeature> featuresProto;

        NImpl::ConstructNluFeatures(featuresProto, scenarioConfig, wizardResponse);

        UNIT_ASSERT_EQUAL(1, featuresProto.size());
        UNIT_ASSERT_EQUAL(feature, featuresProto[0].GetFeature());
        UNIT_ASSERT_EQUAL(featureValue, featuresProto[0].GetValue());
    }
} // Y_UNIT_TEST_SUITE(TestScenarioApiHelper)

} // namespace NAlice::NMegamind
