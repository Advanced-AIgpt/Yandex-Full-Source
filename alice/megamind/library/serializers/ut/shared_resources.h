#pragma once

#include <alice/megamind/library/models/buttons/action_button_model.h>
#include <alice/megamind/library/models/cards/div2_card_model.h>
#include <alice/megamind/library/models/cards/div_card_model.h>
#include <alice/megamind/library/models/cards/text_card_model.h>
#include <alice/megamind/library/models/cards/text_with_button_card_model.h>
#include <alice/megamind/library/models/directives/add_contact_book_asr_directive_model.h>
#include <alice/megamind/library/models/directives/alarm_new_directive_model.h>
#include <alice/megamind/library/models/directives/alarm_set_sound_directive_model.h>
#include <alice/megamind/library/models/directives/audio_play_directive_model.h>
#include <alice/megamind/library/models/directives/callback_directive_model.h>
#include <alice/megamind/library/models/directives/close_dialog_directive_model.h>
#include <alice/megamind/library/models/directives/defer_apply_directive_model.h>
#include <alice/megamind/library/models/directives/end_dialog_session_directive_model.h>
#include <alice/megamind/library/models/directives/find_contacts_directive_model.h>
#include <alice/megamind/library/models/directives/music_play_directive_model.h>
#include <alice/megamind/library/models/directives/open_dialog_directive_model.h>
#include <alice/megamind/library/models/directives/open_settings_directive_model.h>
#include <alice/megamind/library/models/directives/player_rewind_directive_model.h>
#include <alice/megamind/library/models/directives/set_search_filter_directive_model.h>
#include <alice/megamind/library/models/directives/set_timer_directive_model.h>
#include <alice/megamind/library/models/directives/theremin_play_directive_model.h>
#include <alice/megamind/library/models/directives/universal_client_directive_model.h>
#include <alice/megamind/library/models/directives/update_dialog_info_directive_model.h>

#include <util/generic/strbuf.h>

namespace NAlice::NMegamind {

inline constexpr TStringBuf ACTION_ID = "TestActionId";
inline constexpr TStringBuf ANOTHER_ACTION_ID = "AnotherTestActionId";
inline constexpr TStringBuf AD_BLOCK_ID = "TestAdBlockId";
inline constexpr TStringBuf ANALYTICS_TYPE = "TestAnalyticsType";
inline constexpr TStringBuf COMMAND = "TestCommand";
inline constexpr TStringBuf DIALOG_ID = "TestDialogId";
inline constexpr TStringBuf FAKE_GUID = "deadbeef";
inline constexpr TStringBuf FRAME_NAME = "test_frame";
inline constexpr TStringBuf IMAGE_URL = "TestImageUrl";
inline constexpr TStringBuf INNER_ANALYTICS_TYPE = "TestInnerAnalyticsType";
inline constexpr TStringBuf INNER_TITLE = "TestInnerTitle";
inline constexpr TStringBuf INNER_URL = "TestInnerUrl";
inline constexpr TStringBuf KEY = "TestKey";
inline constexpr auto META = TStringBuf(R"({"TestKey":"TestValue"})");
inline constexpr TStringBuf REQUEST_ID = "TestRequestId";
inline constexpr TStringBuf SESSION_ID = "TestSessionId";
inline constexpr TStringBuf SCENARIO_NAME = "TestScenarioName";
inline constexpr TStringBuf TEXT = "TestText";
inline constexpr TStringBuf TYPE_TEXT = "TestTypeText";
inline constexpr TStringBuf UTTERANCE = "TestUtterance";
inline constexpr TStringBuf TITLE = "TestTitle";
inline constexpr TStringBuf UID = "TestUid";
inline constexpr TStringBuf URI = "TestUri";
inline constexpr TStringBuf VALUE = "TestValue";
inline constexpr TStringBuf VIEW_KEY = "mordovia_screen";
inline constexpr TStringBuf SPLASH_DIV = "{}";
inline constexpr TStringBuf UNESCAPED_CHARS = "+@; ";
inline constexpr TStringBuf SKILL_ID_KEY = "skillId";
inline constexpr TStringBuf SKILL_ID_VALUE = "test-skill-id-value";
inline constexpr TStringBuf SCREEN_ID = "TestScreenId";
inline constexpr TStringBuf TEST_MULTIROOM_TOKEN = "TestMultiroomToken";
inline constexpr TStringBuf OWNER_PUID = "TestOwnerPuid";
inline constexpr TStringBuf GUEST_PUID = "TestGuestPuid";

inline constexpr auto ACTION_BUTTON = TStringBuf(R"(
{
    "type": "action",
    "title": "TestTitle",
    "directives": [
        {
            "name": "start_image_recognizer",
            "payload": {},
            "sub_name": "TestInnerAnalyticsType",
            "type": "client_action"
        }
    ]
}
)");

inline constexpr auto ADD_CARD_WITH_ACTION_SPACE = TStringBuf(R"(
{
    "name": "add_card",
    "payload": {
        "div2_card": {
            "hide_borders": true,
            "body": {
                "TestKey": "dialog-action:\/\/?directives=%5B%7B%22ignore_answer%22%3Afalse%2C%22name%22%3A%22%40%40mm_semantic_frame%22%2C%22payload%22%3A%7B%22analytics%22%3A%7B%22origin%22%3A%22Undefined%22%2C%22origin_info%22%3A%22%22%2C%22product_scenario%22%3A%22%22%2C%22purpose%22%3A%22%22%7D%2C%22typed_semantic_frame%22%3A%7B%22search_semantic_frame%22%3A%7B%22query%22%3A%7B%22string_value%22%3A%22query_text%22%7D%7D%7D%7D%2C%22type%22%3A%22server_action%22%7D%2C%7B%22ignore_answer%22%3Atrue%2C%22is_led_silent%22%3Atrue%2C%22name%22%3A%22on_suggest%22%2C%22payload%22%3A%7B%22%40request_id%22%3A%22TestRequestId%22%2C%22%40scenario_name%22%3A%22Vins%22%2C%22button_id%22%3A%22deadbeef%22%2C%22caption%22%3A%22DeepLink%22%2C%22request_id%22%3A%22TestRequestId%22%2C%22scenario_name%22%3A%22TestScenarioName%22%7D%2C%22type%22%3A%22server_action%22%7D%5D"
            }
        },
        "image_url": "image_url",
        "card_show_time_sec": 10,
        "title": "title",
        "type": "type",
        "carousel_id": "id1",
        "div2_templates": {
            "TestKey": "dialog-action:\/\/?directives=%5B%7B%22ignore_answer%22%3Afalse%2C%22name%22%3A%22%40%40mm_semantic_frame%22%2C%22payload%22%3A%7B%22analytics%22%3A%7B%22origin%22%3A%22Undefined%22%2C%22origin_info%22%3A%22%22%2C%22product_scenario%22%3A%22%22%2C%22purpose%22%3A%22%22%7D%2C%22typed_semantic_frame%22%3A%7B%22search_semantic_frame%22%3A%7B%22query%22%3A%7B%22string_value%22%3A%22query_text%22%7D%7D%7D%7D%2C%22type%22%3A%22server_action%22%7D%2C%7B%22ignore_answer%22%3Atrue%2C%22is_led_silent%22%3Atrue%2C%22name%22%3A%22on_suggest%22%2C%22payload%22%3A%7B%22%40request_id%22%3A%22TestRequestId%22%2C%22%40scenario_name%22%3A%22Vins%22%2C%22button_id%22%3A%22deadbeef%22%2C%22caption%22%3A%22DeepLink%22%2C%22request_id%22%3A%22TestRequestId%22%2C%22scenario_name%22%3A%22TestScenarioName%22%7D%2C%22type%22%3A%22server_action%22%7D%5D"
        },
        "card_id": "id2",
        "action_space_id": "ACTION_SPACE_1",
        "teaser_config": {
            "teaser_type": "type",
            "teaser_id": "id"
        }
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto THEMED_ACTION_BUTTON = TStringBuf(R"(
{
    "type": "themed_action",
    "title": "TestTitle",
    "text": "TestText",
    "theme": {
        "image_url": "TestImageUrl"
    },
    "directives": [
        {
            "name": "start_image_recognizer",
            "payload": {},
            "sub_name": "TestInnerAnalyticsType",
            "type": "client_action"
        }
    ]
}
)");

inline constexpr auto ACTION_BUTTON_WITH_ON_SUGGEST = TStringBuf(R"(
{
    "type": "action",
    "title": "TestTitle",
    "directives": [
        {
            "name": "start_image_recognizer",
            "payload": {},
            "sub_name": "TestInnerAnalyticsType",
            "type": "client_action"
        },
        {
            "name": "on_suggest",
            "payload": {
                "@request_id": "TestRequestId",
                "button_id": "deadbeef",
                "caption": "TestTitle",
                "request_id": "TestRequestId",
                "@scenario_name": "Vins",
                "scenario_name": "TestScenarioName"
            },
            "ignore_answer": true,
            "is_led_silent": true,
            "type": "server_action"
        }
    ]
}
)");

inline constexpr auto SEARCH_ACTION_BUTTON_WITH_ON_SUGGEST = TStringBuf(R"(
{
    "type": "action",
    "title": "🔍 \"My button title\"",
    "directives": [
        {
            "name": "@@mm_semantic_frame",
            "payload": {
                "typed_semantic_frame": {
          "search_semantic_frame": {
            "query": {
              "string_value": "Search text"
            }
          }
        },
                "analytics": {
          "product_scenario": "",
          "origin": "Scenario",
          "purpose": "search",
          "origin_info": ""
        },
                "utterance": "Search text"
            },
            "type": "server_action"
        },
        {
            "name": "on_suggest",
            "payload": {
                "@request_id": "TestRequestId",
                "button_id": "deadbeef",
                "caption": "My button title",
                "request_id": "TestRequestId",
                "@scenario_name": "Vins",
                "scenario_name": "TestScenarioName"
            },
            "ignore_answer": true,
            "is_led_silent": true,
            "type": "server_action"
        }
    ]
}
)");

inline constexpr auto THEMED_ACTION_BUTTON_WITH_ON_SUGGEST = TStringBuf(R"(
{
    "type": "themed_action",
    "title": "TestTitle",
    "theme": {
        "image_url": "TestImageUrl"
    },
    "directives": [
        {
            "name": "start_image_recognizer",
            "payload": {},
            "sub_name": "TestInnerAnalyticsType",
            "type": "client_action"
        },
        {
            "name": "on_suggest",
            "payload": {
                "@request_id": "TestRequestId",
                "button_id": "deadbeef",
                "caption": "TestTitle",
                "request_id": "TestRequestId",
                "@scenario_name": "Vins",
                "scenario_name": "TestScenarioName"
            },
            "ignore_answer": true,
            "is_led_silent": true,
            "type": "server_action"
        }
    ]
}
)");

inline constexpr auto ACTION_BUTTON_WITH_CALLBACK = TStringBuf(R"(
{
    "type": "action",
    "title": "TestTitle",
    "directives": [
        {
            "name": "TestText",
            "payload": {
                "@scenario_name": "TestScenarioName",
                "@request_id": "TestRequestId",
                "TestKey": "TestValue"
            },
            "ignore_answer": false,
            "is_led_silent": true,
            "type": "server_action"
        },
        {
            "name": "on_suggest",
            "payload": {
                "@request_id": "TestRequestId",
                "button_id": "deadbeef",
                "caption": "TestTitle",
                "request_id": "TestRequestId",
                "@scenario_name": "Vins",
                "scenario_name": "TestScenarioName"
            },
            "ignore_answer": true,
            "is_led_silent": true,
            "type": "server_action"
        }
    ]
}
)");

inline constexpr auto DIV2_CARD = TStringBuf(R"(
{
    "type": "div2_card",
    "body": {
        "TestKey": "TestValue"
    },
    "has_borders": true
}
)");

inline constexpr auto DIV2_CARD_WITHOUT_BORDERS = TStringBuf(R"(
{
    "type": "div2_card",
    "body": {
        "TestKey": "TestValue"
    },
    "has_borders": false
}
)");

inline constexpr auto DIV_CARD = TStringBuf(R"(
{
    "type": "div_card",
    "text": "...",
    "body": {
        "TestKey": "TestValue"
    }
}
)");

inline constexpr auto DIV_CARD_WITH_DEEP_LINKS = TStringBuf(R"(
{
    "type": "div_card",
    "text": "...",
    "body": {
        "TestUri": "dialog-action://?directives=%5B%7B%22name%22%3A%22start_image_recognizer%22%2C%22payload%22%3A%7B%7D%2C%22sub_name%22%3A%22TestInnerAnalyticsType%22%2C%22type%22%3A%22client_action%22%7D%2C%7B%22ignore_answer%22%3Atrue%2C%22is_led_silent%22%3Atrue%2C%22name%22%3A%22on_suggest%22%2C%22payload%22%3A%7B%22%40request_id%22%3A%22TestRequestId%22%2C%22%40scenario_name%22%3A%22Vins%22%2C%22button_id%22%3A%22deadbeef%22%2C%22caption%22%3A%22DeepLink%22%2C%22request_id%22%3A%22TestRequestId%22%2C%22scenario_name%22%3A%22TestScenarioName%22%7D%2C%22type%22%3A%22server_action%22%7D%5D",
        "TestKey": [
            "dialog-action://?directives=%5B%7B%22name%22%3A%22start_image_recognizer%22%2C%22payload%22%3A%7B%7D%2C%22sub_name%22%3A%22TestInnerAnalyticsType%22%2C%22type%22%3A%22client_action%22%7D%2C%7B%22ignore_answer%22%3Atrue%2C%22is_led_silent%22%3Atrue%2C%22name%22%3A%22on_suggest%22%2C%22payload%22%3A%7B%22%40request_id%22%3A%22TestRequestId%22%2C%22%40scenario_name%22%3A%22Vins%22%2C%22button_id%22%3A%22deadbeef%22%2C%22caption%22%3A%22DeepLink%22%2C%22request_id%22%3A%22TestRequestId%22%2C%22scenario_name%22%3A%22TestScenarioName%22%7D%2C%22type%22%3A%22server_action%22%7D%5D"
        ]
    }
}
)");

inline constexpr auto DIV_CARD_WITH_INVALID_DEEP_LINKS = TStringBuf(R"(
{
    "type": "div_card",
    "text": "...",
    "body": {
        "TestUri": "",
        "TestKey": [""]
    }
}
)");

inline constexpr auto DIV_CARD_WITH_COMPLEX_DEEP_LINKS = TStringBuf(R"(
{
    "type": "div_card",
    "text": "...",
    "body": {
        "TestUri": "dialog-action://?directives=%5B%7B%22name%22%3A%22start_image_recognizer%22%2C%22payload%22%3A%7B%7D%2C%22sub_name%22%3A%22TestInnerAnalyticsType%22%2C%22type%22%3A%22client_action%22%7D%2C%7B%22ignore_answer%22%3Atrue%2C%22is_led_silent%22%3Atrue%2C%22name%22%3A%22on_suggest%22%2C%22payload%22%3A%7B%22%40request_id%22%3A%22TestRequestId%22%2C%22%40scenario_name%22%3A%22Vins%22%2C%22button_id%22%3A%22deadbeef%22%2C%22caption%22%3A%22DeepLink%22%2C%22request_id%22%3A%22TestRequestId%22%2C%22scenario_name%22%3A%22TestScenarioName%22%7D%2C%22type%22%3A%22server_action%22%7D%5D",
        "TestKey": {
            "TestKey": [
                "dialog-action://?directives=%5B%7B%22name%22%3A%22start_image_recognizer%22%2C%22payload%22%3A%7B%7D%2C%22sub_name%22%3A%22TestInnerAnalyticsType%22%2C%22type%22%3A%22client_action%22%7D%2C%7B%22ignore_answer%22%3Atrue%2C%22is_led_silent%22%3Atrue%2C%22name%22%3A%22on_suggest%22%2C%22payload%22%3A%7B%22%40request_id%22%3A%22TestRequestId%22%2C%22%40scenario_name%22%3A%22Vins%22%2C%22button_id%22%3A%22deadbeef%22%2C%22caption%22%3A%22DeepLink%22%2C%22request_id%22%3A%22TestRequestId%22%2C%22scenario_name%22%3A%22TestScenarioName%22%7D%2C%22type%22%3A%22server_action%22%7D%5D"
            ],
            "TestUri": [
                {
                    "TestKey": [
                        "dialog-action://?directives=%5B%7B%22name%22%3A%22start_image_recognizer%22%2C%22payload%22%3A%7B%7D%2C%22sub_name%22%3A%22TestInnerAnalyticsType%22%2C%22type%22%3A%22client_action%22%7D%2C%7B%22ignore_answer%22%3Atrue%2C%22is_led_silent%22%3Atrue%2C%22name%22%3A%22on_suggest%22%2C%22payload%22%3A%7B%22%40request_id%22%3A%22TestRequestId%22%2C%22%40scenario_name%22%3A%22Vins%22%2C%22button_id%22%3A%22deadbeef%22%2C%22caption%22%3A%22DeepLink%22%2C%22request_id%22%3A%22TestRequestId%22%2C%22scenario_name%22%3A%22TestScenarioName%22%7D%2C%22type%22%3A%22server_action%22%7D%5D"
                    ]
                }
            ],
            "TestImageUrl": [
                1,
                "1",
                true,
                null
            ]
        },
        "TestImageUrl": {
            "TestDialogId": 1,
            "TestKey": true,
            "TestCommand": null
        }
    }
}
)");

inline constexpr auto DIV_CARD_WITH_CUSTOM_ESCAPES = TStringBuf(R"(
{
    "type": "div_card",
    "text": "...",
    "body": {
        "TestUri": "dialog-action://?directives=%5B%7B%22ignore_answer%22%3Afalse%2C%22is_led_silent%22%3Afalse%2C%22name%22%3A%22%2B%40%3B%20%22%2C%22payload%22%3A%7B%22%2B%40%3B%20%22%3A%22%2B%40%3B%20%22%2C%22%40request_id%22%3A%22TestRequestId%22%2C%22%40scenario_name%22%3A%22TestScenarioName%22%7D%2C%22type%22%3A%22server_action%22%7D%2C%7B%22ignore_answer%22%3Atrue%2C%22is_led_silent%22%3Atrue%2C%22name%22%3A%22on_suggest%22%2C%22payload%22%3A%7B%22%40request_id%22%3A%22TestRequestId%22%2C%22%40scenario_name%22%3A%22Vins%22%2C%22button_id%22%3A%22deadbeef%22%2C%22caption%22%3A%22DeepLink%22%2C%22request_id%22%3A%22TestRequestId%22%2C%22scenario_name%22%3A%22TestScenarioName%22%7D%2C%22type%22%3A%22server_action%22%7D%5D",
    }
}
)");

inline constexpr auto TEXT_CARD = TStringBuf(R"(
{
    "type": "simple_text",
    "text": "TestText"
}
)");

inline constexpr auto TEXT_WITH_BUTTON = TStringBuf(R"(
{
    "type": "text_with_button",
    "text": "TestText",
    "buttons": [
        {
            "type": "action",
            "title": "TestTitle",
            "directives": []
        }
    ]
}
)");

inline constexpr auto TEXT_WITH_BUTTON_WITH_ON_SUGGEST = TStringBuf(R"(
{
    "type": "text_with_button",
    "text": "TestText",
    "buttons": [
        {
            "type": "action",
            "title": "TestTitle",
            "directives": [
                {
                    "name": "on_suggest",
                    "payload": {
                        "@request_id": "TestRequestId",
                        "button_id": "deadbeef",
                        "caption": "TestTitle",
                        "request_id": "TestRequestId",
                        "@scenario_name": "Vins",
                        "scenario_name": "TestScenarioName"
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

inline constexpr auto ALARM_NEW_DIRECTIVE = TStringBuf(R"(
{
    "name": "alarm_new",
    "payload": {
        "on_success": {
            "@request_id": "TestRequestId",
            "@scenario_name": "TestScenarioName",
            "TestKey": "TestValue"
        },
        "on_fail": {
            "@request_id": "TestRequestId",
            "@scenario_name": "TestScenarioName",
            "TestTitle": "TestInnerTitle"
        },
        "state": "TestText"
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto ALARM_SET_MAX_LEVEL = TStringBuf(R"(
{
    "name": "alarm_set_max_level",
    "payload": {
        "new_level": 5
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto ALARM_SET_SOUND_DIRECTIVE = TStringBuf(R"(
{
    "name": "alarm_set_sound",
    "payload": {
        "server_action": {
            "name": "TestInnerAnalyticsType",
            "payload": {
                "@scenario_name": "TestScenarioName",
                "@request_id": "TestRequestId",
                "TestKey": "TestValue"
            },
            "type": "server_action",
            "is_led_silent": true,
            "ignore_answer": true
        },
        "sound_alarm_setting": {
            "info": {
                "active": true,
                "album": {
                    "genre": "local-indie",
                    "id": "4006712",
                    "title": "Петь"
                },
                "artists": [
                    {
                        "composer": true,
                        "id": "2561847",
                        "is_various": true,
                        "name": "Владимир Шахрин"
                    }
                ],
                "available": true,
                "color": "#FF0000",
                "coverUri": "https://avatars.yandex.net/get-music-content/98892/06aff6d0.a.4006712-1/200x200",
                "filters": {
                    "activity": "wake-up",
                    "epoch": "nineties",
                    "genre": "meditation",
                    "isNew": true,
                    "isPersonal": true,
                    "isPopular": true,
                    "mood": "energetic",
                    "personality": "is_personal",
                    "special_playlist": "playlist_of_the_day"
                },
                "first_track": {
                    "album": {
                        "genre": "local-indie",
                        "id": "4006712",
                        "title": "Петь"
                    },
                    "artists": [
                        {
                            "composer": true,
                            "id": "2561847",
                            "is_various": true,
                            "name": "Владимир Шахрин"
                        }
                    ],
                    "coverUri": "https://avatars.yandex.net/get-music-content/98892/06aff6d0.a.4006712-1/200x200",
                    "id": "32858088",
                    "subtype": "music",
                    "title": "Петь",
                    "type": "track",
                    "uri": "https://music.yandex.ru/album/4006712/track/32858088/?from=alice&mob=0"
                },
                "first_track_uri": "http://music.yandex.ru/track/32858088",
                "for_alarm": true,
                "frequency": "103.4",
                "genre": "soundtrack",
                "id": "32858088",
                "imageUrl": "avatars.mds.yandex.net/get-music-misc/49997/mayak-225/%%",
                "name": "LOBODA",
                "officialSiteUrl": "https://radiomayak.ru",
                "partnerId": "139316",
                "radioId": "mayak",
                "session_id": "DB5yi6vE",
                "showRecognition": true,
                "streamUrl": "https://strm.yandex.ru/fm/fm_mayak/fm_mayak0.m3u8",
                "subtype": "music",
                "techName": "fm_love_novosibirsk",
                "title": "Петь",
                "type": "track",
                "uri": "https://music.yandex.ru/album/4006712/track/32858088/?from=alice&mob=0",
                "uuid": "4dbcbdb4ae87d073a23a8d47cb7e35ab"
            },
            "repeat": true,
            "type": "music"
        }
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto ALARM_STOP = TStringBuf(R"(
{
    "name": "alarm_stop",
    "payload": {},
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto CAR_DIRECTIVE = TStringBuf(R"(
{
    "name": "car",
    "payload": {
        "application": "car",
        "intent": "media_select",
        "params": {
            "radio": "Эхо Москвы"
        }
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto CLOSE_DIALOG_DIRECTIVE = TStringBuf(R"(
{
    "name": "close_dialog",
    "payload": {
        "dialog_id": "TestScenarioName:TestDialogId"
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto CLOSE_DIALOG_DIRECTIVE_WITH_SCREEN_ID = TStringBuf(R"(
{
    "name": "close_dialog",
    "payload": {
        "dialog_id": "TestScenarioName:TestDialogId",
        "screen_id": "test_screen_id"
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto END_DIALOG_SESSION_DIRECTIVE = TStringBuf(R"(
{
    "name": "end_dialog_session",
    "payload": {
        "dialog_id": "TestScenarioName:TestDialogId"
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto FIND_CONTACTS_DIRECTIVE = TStringBuf(R"(
{
    "name": "find_contacts",
    "payload": {
        "form": "TestValue",
        "values": [
            "TestText"
        ],
        "request": [
            {
                "values": [
                    "TestValue"
                ],
                "tag": "TestDialogId"
            }
        ],
        "mimetypes_whitelist": {
            "name": [
                "TestAdBlockId"
            ],
            "column": [
                "TestActionId"
            ]
        },
        "on_permission_denied_payload": {
            "@request_id": "TestRequestId",
            "@scenario_name": "TestScenarioName",
            "TestKey": "TestValue"
        }
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto GO_DOWN_DIRECTIVE = TStringBuf(R"(
{
    "name": "go_down",
    "sub_name": "TestAnalyticsType",
    "payload": {},
    "type": "client_action"
}
)");

inline constexpr auto GO_TOP_DIRECTIVE = TStringBuf(R"(
{
    "name": "go_top",
    "sub_name": "TestAnalyticsType",
    "payload": {},
    "type": "client_action"
}
)");

inline constexpr auto GO_UP_DIRECTIVE = TStringBuf(R"(
{
    "name": "go_up",
    "sub_name": "TestAnalyticsType",
    "payload": {},
    "type": "client_action"
}
)");

inline constexpr auto MORDOVIA_COMMAND_DIRECTIVE = TStringBuf(R"(
{
    "name": "mordovia_command",
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "payload": {
        "command": "TestCommand",
        "meta": "{\"TestKey\":\"TestValue\"}",
        "scenario_name": "TestScenarioName",
        "scenario": "TestScenarioName:mordovia_screen",
        "view_key": "mordovia_screen"
    }
}
)");

inline constexpr auto MUSIC_RECOGNITION = TStringBuf(R"(
{
    "name": "music_recognition",
    "payload": {
        "album": {
            "genre": "soundtrack",
            "id": "59592",
            "title": "The Matrix Reloaded: The Album"
        },
        "artists": [
            {
                "composer": true,
                "id": "1151",
                "is_various": true,
                "name": "Justin Timberlake"
            }
        ],
        "coverUri": "https://avatars.yandex.net/get-music-content/28589/a86f9db5.a.59592-1/200x200",
        "id": "555822",
        "subtype": "music",
        "title": "When The World Ends",
        "type": "track",
        "uri": "https://music.yandex.ru/album/59592/track/555822/?from=alice&mob=0&play=1"
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto MORDOVIA_SHOW_DIRECTIVE = TStringBuf(R"(
{
    "name": "mordovia_show",
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "payload": {
        "url": "TestUri",
        "is_full_screen": true,
        "scenario": "TestScenarioName:mordovia_screen",
        "view_key": "mordovia_screen",
        "scenario_name": "TestScenarioName",
        "splash_div": "{}",
        "callback_prototype": {
            "name": "some name",
            "type": "server_action",
            "is_led_silent": true,
            "ignore_answer": true,
            "payload": {
                "@request_id": "TestRequestId",
                "@scenario_name": "TestScenarioName",
                "key": "value"
            }
        },
        "go_back": false
    }
}
)");

inline constexpr auto REMINDERS_CREATE_DIRECTIVE = TStringBuf(R"(
{
    "name": "reminders_set",
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "payload": {
        "scenario_name": "TestScenarioName",
        "id": "guid",
        "text": "remind me this",
        "epoch": "12345678",
        "timezone": "Europe/Moscow"
    }
}
)");

inline constexpr auto REMINDERS_CREATE_DIRECTIVE_FULL = TStringBuf(R"(
{
    "name": "reminders_set",
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "payload": {
        "scenario_name": "TestScenarioName",
        "id": "guid",
        "text": "remind me this",
        "epoch": "12345678",
        "timezone": "Europe/Moscow",
        "on_success_callback": {
            "name": "reminders_on_success_callback",
            "type": "server_action",
            "is_led_silent": false,
            "ignore_answer": false,
            "payload": {
                "@request_id": "TestRequestId",
                "@scenario_name": "TestScenarioName",
                "key": "value"
            }
        },
        "on_fail_callback": {
            "name": "reminders_on_fail_callback",
            "type": "server_action",
            "is_led_silent": false,
            "ignore_answer": false,
            "payload": {
                "@request_id": "TestRequestId",
                "@scenario_name": "TestScenarioName",
                "key1": "value1"
            }
        },
        "on_shoot_frame": {
            "name": "@@mm_semantic_frame",
            "ignore_answer": false,
            "type": "server_action",
            "payload": {
                "typed_semantic_frame": {
                    "reminders_on_shoot_semantic_frame": {
                        "id": {
                            "string_value": "guid"
                        },
                        "text": {
                            "string_value": "remind me this"
                        },
                        "epoch": {
                            "epoch_value": "1234567"
                        },
                        "timezone": {
                            "string_value": "Europe/Moscow"
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "reminders",
                    "origin": "Scenario",
                    "origin_info": "",
                    "purpose": "alice.reminders.shoot"
                }
            }
        }
    }
}
)");

inline constexpr auto REMINDERS_CANCEL_DIRECTIVE = TStringBuf(R"(
{
    "name": "reminders_cancel",
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "payload": {
        "scenario_name": "TestScenarioName",
        "Id": ["guid"],
        "id": ["guid"],
        "action": "id"
    }
}
)");

inline constexpr auto REMINDERS_CANCEL_DIRECTIVE_FULL = TStringBuf(R"(
{
    "name": "reminders_cancel",
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "payload": {
        "scenario_name": "TestScenarioName",
        "id": ["guid"],
        "Id": ["guid"],
        "action": "id",
        "on_success_callback": {
            "name": "reminders_on_success_callback",
            "type": "server_action",
            "is_led_silent": false,
            "ignore_answer": false,
            "payload": {
                "@request_id": "TestRequestId",
                "@scenario_name": "TestScenarioName",
                "key": "value"
            }
        },
        "on_fail_callback": {
            "name": "reminders_on_fail_callback",
            "type": "server_action",
            "is_led_silent": false,
            "ignore_answer": false,
            "payload": {
                "@request_id": "TestRequestId",
                "@scenario_name": "TestScenarioName",
                "key1": "value1"
            }
        }
    }
}
)");

inline constexpr auto MUSIC_PLAY_DIRECTIVE = TStringBuf(R"(
{
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "name": "music_play",
    "room_device_ids": [
        "device_id_1",
        "device_id_2",
        "device_id_5"
    ],
    "payload": {
        "uid": "TestUid",
        "session_id": "TestSessionId",
        "offset": 42,
        "alarm_id": "TestAlarmId",
        "first_track_id": "TestFirstTrackId"
    }
}
)");

inline constexpr auto MUSIC_PLAY_DIRECTIVE_WITH_ENDPOINT_ID = TStringBuf(R"(
{
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "name": "music_play",
    "room_device_ids": [
        "device_id_1",
        "device_id_2",
        "device_id_5"
    ],
    "payload": {
        "uid": "TestUid",
        "session_id": "TestSessionId",
        "offset": 42,
        "alarm_id": "TestAlarmId",
        "first_track_id": "TestFirstTrackId"
    },
    "endpoint_id": "kekid"
}
)");

inline constexpr auto MUSIC_PLAY_DIRECTIVE_2 = TStringBuf(R"(
{
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "name": "music_play",
    "room_device_ids": [
        "device_id_3",
        "device_id_4"
    ],
    "payload": {
        "uid": "TestUid",
        "session_id": "TestSessionId",
        "offset": 42
    }
}
)");

inline constexpr auto MUSIC_PLAY_DIRECTIVE_EMPTY_ROOM = TStringBuf(R"(
{
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "name": "music_play",
    "room_device_ids": [
        "device_id_1",
        "device_id_2",
        "device_id_3",
        "device_id_4"
    ],
    "payload": {
        "uid": "TestUid",
        "session_id": "TestSessionId",
        "offset": 42
    }
}
)");

inline constexpr auto MUSIC_PLAY_DIRECTIVE_WITH_LOCATION_INFO = TStringBuf(R"(
{
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "name": "music_play",
    "room_device_ids": [
        "device_id_1"
    ],
    "payload": {
        "uid": "TestUid",
        "session_id": "TestSessionId",
        "offset": 42
    }
}
)");

inline constexpr auto NAVIGATE_BROWSER_DIRECTIVE_BASE = TStringBuf(R"(
{
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "name": "navigate_browser",
    "payload": {
        "command_name": "%s"
    }
}
)");

inline constexpr auto OPEN_DIALOG_DIRECTIVE = TStringBuf(R"(
{
    "name": "open_dialog",
    "payload": {
        "directives": [
            {
                "name": "start_image_recognizer",
                "payload": {},
                "sub_name": "TestInnerAnalyticsType",
                "type": "client_action"
            }
        ],
        "dialog_id": "TestScenarioName:TestDialogId"
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto OPEN_SETTINGS_DIRECTIVE_MODEL_BASE = TStringBuf(R"(
{
    "name": "open_settings",
    "payload": {
        "target": "%s"
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto OPEN_URI_DIRECTIVE = TStringBuf(R"(
{
    "name": "open_uri",
    "payload": {
        "uri": "TestUri"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto OPEN_URI_DIRECTIVE_WITH_SCREEN_ID = TStringBuf(R"(
{
    "name": "open_uri",
    "payload": {
        "uri": "TestUri",
        "screen_id": "TestScreenId"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto PLAYER_CONTINUE_DIRECTIVE = TStringBuf(R"(
{
    "name": "player_continue",
    "payload": {
        "player": "music"
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto PLAYER_DISLIKE_DIRECTIVE = TStringBuf(R"(
{
    "name": "player_dislike",
    "payload": {
        "uid": "TestUid"
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto PLAYER_LIKE_DIRECTIVE = TStringBuf(R"(
{
    "name": "player_like",
    "payload": {
        "uid": "TestUid"
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto PLAYER_NEXT_TRACK_DIRECTIVE = TStringBuf(R"(
{
    "name": "player_next_track",
    "payload": {
        "player": "music",
        "uid": "TestUid"
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto PLAYER_PREVIOUS_TRACK_DIRECTIVE = TStringBuf(R"(
{
    "name": "player_previous_track",
    "payload": {
        "player": "music"
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto PLAYER_REPLAY_DIRECTIVE = TStringBuf(R"(
{
    "name": "player_replay",
    "payload": {},
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto PLAYER_SHUFFLE_DIRECTIVE = TStringBuf(R"(
{
    "name": "player_shuffle",
    "payload": {},
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto PLAYER_PAUSE_DIRECTIVE = TStringBuf(R"(
{
    "name": "player_pause",
    "payload": {
        "smooth": true,
        "room_id": "kitchen"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "multiroom_session_id": "12345"
}
)");

inline constexpr auto PLAYER_REWIND_DIRECTIVE_ABSOLUTE = TStringBuf(R"(
{
    "name": "player_rewind",
    "payload": {
        "amount": 30,
        "type": "absolute"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto PLAYER_REWIND_DIRECTIVE_BACKWARD = TStringBuf(R"(
{
    "name": "player_rewind",
    "payload": {
        "amount": 30,
        "type": "backward"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto PLAYER_REWIND_DIRECTIVE_FORWARD = TStringBuf(R"(
{
    "name": "player_rewind",
    "payload": {
        "amount": 30,
        "type": "forward"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto RADIO_PLAY_DIRECTIVE_WITH_ALARM_ID = TStringBuf(R"(
{
    "name": "radio_play",
    "payload": {
        "active": true,
        "available": true,
        "color": "#0071BB",
        "frequency": "102.9",
        "imageUrl": "avatars.mds.yandex.net/get-music-misc/28592/komsomolskaya_pravda-225/%%",
        "officialSiteUrl": "http://mariafm.ru",
        "radioId": "nashe",
        "score": 0.4435845617,
        "showRecognition": true,
        "streamUrl": "https://strm.yandex.ru/fm/fm_dacha_main/fm_dacha_main0.m3u8",
        "title": "Радио Комсомольская правда",
        "alarm_id": "deadface-4487-421e-866d-a3f087990a34"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto RADIO_PLAY_DIRECTIVE = TStringBuf(R"(
{
    "name": "radio_play",
    "payload": {
        "active": true,
        "available": true,
        "color": "#0071BB",
        "frequency": "102.9",
        "imageUrl": "avatars.mds.yandex.net/get-music-misc/28592/komsomolskaya_pravda-225/%%",
        "officialSiteUrl": "http://mariafm.ru",
        "radioId": "nashe",
        "score": 0.4435845617,
        "showRecognition": true,
        "streamUrl": "https://strm.yandex.ru/fm/fm_dacha_main/fm_dacha_main0.m3u8",
        "title": "Радио Комсомольская правда"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto SCREEN_ON_DIRECTIVE = TStringBuf(R"(
{
    "name": "screen_on",
    "type": "client_action",
    "payload": {},
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto SCREEN_OFF_DIRECTIVE = TStringBuf(R"(
{
    "name": "screen_off",
    "type": "client_action",
    "payload": {},
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto SET_COOKIES_DIRECTIVE = TStringBuf(R"(
{
    "name": "set_cookies",
    "type": "client_action",
    "payload": {
        "value": "{\"uaas\": \"t123456.e1600000000.sDEADBEEF123456DEADBEEF123456DEAD\"}"
    },
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto SET_SEARCH_FILTER_DIRECTIVE_MODERATE = TStringBuf(R"(
{
    "name": "set_search_filter",
    "payload": {
        "new_level": "moderate"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto SET_SEARCH_FILTER_DIRECTIVE_NONE = TStringBuf(R"(
{
    "name": "set_search_filter",
    "payload": {
        "new_level": "none"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto SET_SEARCH_FILTER_DIRECTIVE_STRICT = TStringBuf(R"(
{
    "name": "set_search_filter",
    "payload": {
        "new_level": "strict"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto SET_TIMER_DIRECTIVE = TStringBuf(R"(
{
    "name": "set_timer",
    "payload": {
        "duration": 42,
        "listening_is_possible": true
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto SET_TIMER_TIMESTAMP_DIRECTIVE = TStringBuf(R"(
{
    "name": "set_timer",
    "payload": {
        "on_success": {
            "@request_id": "TestRequestId",
            "@scenario_name": "TestScenarioName",
            "TestKey": "TestValue"
        },
        "on_fail": {
            "@request_id": "TestRequestId",
            "@scenario_name": "TestScenarioName",
            "TestTitle": "TestInnerTitle"
        },
        "directives": [
            {
                "name": "type",
                "payload": {
                    "text": "TestText"
                },
                "type": "client_action",
                "sub_name": "TestAnalyticsType"
            }
        ],
        "timestamp": 42
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto SHOW_GALLERY_DIRECTIVE = TStringBuf(R"(
{
    "name" : "show_gallery",
    "payload" : {
        "items" : [
            {
                "available" : 1,
                "cover_url_2x3" : "http://avatars.mds.yandex.net/get-ott/223007/2a0000016b88eaa2e45eabcae89683e7947e/328x492",
                "debug_info" : {
                    "web_page_url" : "http://www.kinopoisk.ru/film/464963"
                },
                "genre" : "фэнтези, боевик, драма, мелодрама, приключения",
                "misc_ids" : {
                    "kinopoisk" : "464963"
                },
                "name" : "Игра престолов",
                "normalized_name" : "игра престолов",
                "provider_info" : [
                    {
                        "misc_ids" : {
                            "kinopoisk" : "464963"
                        },
                        "provider_item_id" : "47bab88d43ac0a82ad62bfbbaf302e07",
                        "provider_name" : "kinopoisk",
                        "type" : "tv_show"
                    }
                ],
                "provider_item_id" : "47bab88d43ac0a82ad62bfbbaf302e07",
                "provider_name" : "kinopoisk",
                "release_year" : 2019,
                "relevance" : 104354335,
                "relevance_prediction" : 0.1113237796,
                "seasons_count" : 8,
                "type" : "tv_show"
            },
            {
                "availability_request" : {
                    "kinopoisk" : {
                        "id" : "41c0c244907ac6cfa769cf4760b146fc"
                    },
                    "type" : "film"
                },
                "available" : 1,
                "cover_url_2x3" : "http://avatars.mds.yandex.net/get-ott/239697/2a0000016b8965176ec1309f87d6f6d36189/328x492",
                "debug_info" : {
                    "web_page_url" : "http://www.kinopoisk.ru/film/1260303"
                },
               "duration" : 6507,
                "genre" : "документальный",
                "misc_ids" : {
                    "kinopoisk" : "1260303"
                },
                "name" : "Игра престолов. Последний дозор",
                "normalized_name" : "игра престолов последний дозор",
                "provider_info" : [
                    {
                        "misc_ids" : {
                            "kinopoisk" : "1260303"
                        },
                        "provider_item_id" : "41c0c244907ac6cfa769cf4760b146fc",
                        "provider_name" : "kinopoisk",
                        "type" : "movie"
                    }
                ],
                "provider_item_id" : "41c0c244907ac6cfa769cf4760b146fc",
                "provider_name" : "kinopoisk",
                "release_year" : 2019,
                "relevance" : 104354245,
                "relevance_prediction" : 0.09411265535,
                "type" : "movie"
            }
       ]
   },
   "sub_name" : "video_show_gallery",
   "type" : "client_action"
}
)");

inline constexpr auto SHOW_SEASON_GALLERY_DIRECTIVE = TStringBuf(R"(
{
    "name" : "show_season_gallery",
    "payload" : {
        "items" : [
              {
                  "availability_request" : {
                      "kinopoisk" : {
                          "id" : "4b7e75f953b028e5b856bbaf23d5459f",
                          "season_id" : "402aa1031e8c647f9d7948543b3d91ac",
                          "tv_show_id" : "46c5df252dc1a790b82d1a00fcf44812"
                      },
                      "type" : "episode"
                  },
                  "duration" : 1273,
                  "episode" : 1,
                  "name" : "Рик и Морти - Сезон 1 - Серия 1 - Пилотная серия",
                  "normalized_name" : "рик и морти - сезон 1 - серия 1 - пилотная серия",
                  "provider_info" : [
                      {
                          "episode" : 1,
                            "provider_item_id" : "4b7e75f953b028e5b856bbaf23d5459f",
                            "provider_name" : "kinopoisk",
                            "provider_number" : 1,
                            "season" : 1,
                            "tv_show_item_id" : "46c5df252dc1a790b82d1a00fcf44812",
                            "tv_show_season_id" : "402aa1031e8c647f9d7948543b3d91ac",
                            "type" : "tv_show_episode"
                        }
                    ],
                    "provider_item_id" : "4b7e75f953b028e5b856bbaf23d5459f",
                    "provider_name" : "kinopoisk",
                    "provider_number" : 1,
                    "season" : 1,
                    "seasons_count" : 4,
                    "thumbnail_url_16x9" : "http://avatars.mds.yandex.net/get-ott/1534341/2a0000016907010bc88b4faed45938edc080/672x438",
                    "tv_show_item_id" : "46c5df252dc1a790b82d1a00fcf44812",
                    "tv_show_season_id" : "402aa1031e8c647f9d7948543b3d91ac",
                    "type" : "tv_show_episode"
                },
                {
                "availability_request" : {
                    "kinopoisk" : {
                        "id" : "429e7ffab30c266da49b8075f2dde9f1",
                        "season_id" : "402aa1031e8c647f9d7948543b3d91ac",
                        "tv_show_id" : "46c5df252dc1a790b82d1a00fcf44812"
                    },
                    "type" : "episode"
                },
                "duration" : 1267,
                "episode" : 2,
                "name" : "Рик и Морти - Сезон 1 - Серия 2 - Пёс-газонокосильщик",
                "normalized_name" : "рик и морти - сезон 1 - серия 2 - пёс-газонокосильщик",
                "provider_info" : [
                    {
                        "episode" : 2,
                        "provider_item_id" : "429e7ffab30c266da49b8075f2dde9f1",
                        "provider_name" : "kinopoisk",
                        "provider_number" : 2,
                        "season" : 1,
                        "tv_show_item_id" : "46c5df252dc1a790b82d1a00fcf44812",
                        "tv_show_season_id" : "402aa1031e8c647f9d7948543b3d91ac",
                        "type" : "tv_show_episode"
                    }
                ],
                "provider_item_id" : "429e7ffab30c266da49b8075f2dde9f1",
                "provider_name" : "kinopoisk",
                 "provider_number" : 2,
                "season" : 1,
                "seasons_count" : 4,
                "thumbnail_url_16x9" : "http://avatars.mds.yandex.net/get-ott/374297/2a00000168333c3125f90ba72065a2856de0/672x438",
                "tv_show_item_id" : "46c5df252dc1a790b82d1a00fcf44812",
                "tv_show_season_id" : "402aa1031e8c647f9d7948543b3d91ac",
                "type" : "tv_show_episode"
            }
          ],
          "season" : 1,
          "tv_show_item" : {
            "cover_url_2x3" : "http://avatars.mds.yandex.net/get-ott/2385704/2a0000016e880714351dd8b87275dc97a14d/328x492",
            "debug_info" : {
                "web_page_url" : "http://www.kinopoisk.ru/film/685246"
            },
            "genre" : "мультфильм, комедия, фантастика, приключения",
            "misc_ids" : {
                "kinopoisk" : "685246"
            },
            "name" : "Рик и Морти",
            "normalized_name" : "рик и морти",
            "provider_info" : [
                {
                    "misc_ids" : {
                        "kinopoisk" : "685246"
                    },
                    "provider_item_id" : "46c5df252dc1a790b82d1a00fcf44812",
                    "provider_name" : "kinopoisk",
                    "type" : "tv_show"
                }
            ],
            "provider_item_id" : "46c5df252dc1a790b82d1a00fcf44812",
            "provider_name" : "kinopoisk",
            "release_year" : 2013,
            "relevance" : 104933976,
            "relevance_prediction" : 0.1386299241,
            "seasons" : [
                {
                    "number" : 1
                },
                {
                    "number" : 2
                },
                {
                    "number" : 3
                },
                {
                    "number" : 4
                }
            ],
            "seasons_count" : 4,
            "type" : "tv_show"
        }
    },
    "sub_name" : "video_show_season_gallery",
    "type" : "client_action"
}
)");

// FIXME: availability
inline constexpr auto SHOW_PAY_PUSH_SCREEN_DIRECTIVE = TStringBuf(R"(
{
    "name": "show_pay_push_screen",
    "payload": {
        "item": {
            "availability_request": {
                "kinopoisk": {
                    "id": "40c88f22eed260b4be435b75457f318d"
                },
                "type": "film"
            },
            "available": 1,
            "cover_url_16x9": "http://avatars.mds.yandex.net/get-ott/2385704/2a0000016e40c4acb8e81472433f49005730/1920x1080",
            "cover_url_2x3": "http://avatars.mds.yandex.net/get-ott/2419418/2a0000016e40c4b01665e947337ec80a2ee8/328x492",
            "debug_info": {
                "web_page_url": "http://www.kinopoisk.ru/film/1007842"
            },
            "duration": 7236,
            "genre": "боевик, триллер",
            "min_age": 16,
            "misc_ids": {
                "kinopoisk": "1007842"
            },
            "name": "Падение ангела",
            "provider_info": [
                {
                    "misc_ids": {
                        "kinopoisk": "1007842"
                    },
                    "provider_item_id": "40c88f22eed260b4be435b75457f318d",
                    "provider_name": "kinopoisk",
                    "type": "movie"
                }
            ],
            "provider_item_id": "40c88f22eed260b4be435b75457f318d",
            "provider_name": "kinopoisk",
            "rating": 6.208000183,
            "release_year": 2019,
            "thumbnail_url_16x9": "http://avatars.mds.yandex.net/get-ott/2385704/2a0000016e40c4acb8e81472433f49005730/672x438",
            "thumbnail_url_16x9_small": "http://avatars.mds.yandex.net/get-ott/2385704/2a0000016e40c4acb8e81472433f49005730/88x88",
            "thumbnail_url_2x3_small": "http://avatars.mds.yandex.net/get-ott/2419418/2a0000016e40c4b01665e947337ec80a2ee8/132x132",
            "type": "movie"
        }
    },
    "sub_name": "video_show_pay_push_screen",
    "type": "client_action"
}
)");

inline constexpr auto SHOW_VIDEO_DESCRIPTION_DIRECTIVE = TStringBuf(R"(
{
    "name" : "show_description",
    "payload" : {
        "item" : {
            "actors" : "Джастин Ройланд, Крис Парнелл, Спенсер Грэммер",
            "cover_url_16x9" : "http://avatars.mds.yandex.net/get-ott/223007/2a0000016e88070f48fe7a60995def446f16/1920x1080",
            "cover_url_2x3" : "http://avatars.mds.yandex.net/get-ott/2385704/2a0000016e880714351dd8b87275dc97a14d/328x492",
            "debug_info" : {
                "web_page_url" : "http://www.kinopoisk.ru/film/685246"
            },
            "description" : "В центре сюжета - школьник по имени Морти и его дедушка Рик...",
            "directors" : "Пит Мишелс, Уэсли Арчер, Брайан Ньютон",
            "genre" : "мультфильм, комедия, фантастика, приключения",
            "min_age" : 18,
            "misc_ids" : {
                "kinopoisk" : "685246"
            },
            "name" : "Рик и Морти",
            "normalized_name" : "рик и морти",
            "provider_info" : [
                {
                    "misc_ids" : {
                        "kinopoisk" : "685246"
                    },
                    "provider_item_id" : "46c5df252dc1a790b82d1a00fcf44812",
                    "provider_name" : "kinopoisk",
                    "type" : "tv_show"
                }
            ],
            "provider_item_id" : "46c5df252dc1a790b82d1a00fcf44812",
            "provider_name" : "kinopoisk",
            "rating" : 8.876000404,
            "release_year" : 2013,
            "relevance" : 104933976,
            "relevance_prediction" : 0.1386299241,
            "seasons_count" : 4,
            "thumbnail_url_16x9" : "http://avatars.mds.yandex.net/get-ott/223007/2a0000016e88070f48fe7a60995def446f16/672x438",
            "thumbnail_url_16x9_small" : "http://avatars.mds.yandex.net/get-ott/223007/2a0000016e88070f48fe7a60995def446f16/88x88",
            "thumbnail_url_2x3_small" : "http://avatars.mds.yandex.net/get-ott/2385704/2a0000016e880714351dd8b87275dc97a14d/132x132",
            "type" : "tv_show"
        }
    },
    "sub_name" : "video_show_description",
    "type" : "client_action"
}
)");

inline constexpr auto SOUND_SET_LEVEL_DIRECTIVE = TStringBuf(R"(
{
    "name": "sound_set_level",
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "payload": {
        "new_level": 7
    }
}
)");

inline constexpr auto SOUND_SET_LEVEL_DIRECTIVE_IN_LOCATION = TStringBuf(R"(
{
    "name": "sound_set_level",
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "payload": {
        "new_level": 6,
        "room_id": "kitchen"
    }
}
)");

inline constexpr auto SOUND_SET_LEVEL_DIRECTIVE_WITH_MULTIROOM = TStringBuf(R"(
{
    "name": "sound_set_level",
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "multiroom_session_id": "12345",
    "payload": {
        "new_level": 7
    }
}
)");

inline constexpr auto START_IMAGE_RECOGNIZER_DIRECTIVE = TStringBuf(R"(
{
    "name": "start_image_recognizer",
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "payload": {
        "camera_type": "front",
        "image_search_mode": 1,
        "image_search_mode_name": "market"
    }
}
)");

inline constexpr auto THEREMIN_PLAY_DIRECTIVE_WITH_EXTERNAL_SET = TStringBuf(R"(
{
    "name": "theremin_play",
    "payload": {
        "external_set": {
            "stop_on_ceil": true,
            "no_overlay_samples": true,
            "repeat_sound_inside": true,
            "samples": [
                {
                    "url": "TestUri"
                }
            ]
        }
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto THEREMIN_PLAY_DIRECTIVE_WITH_INTERNAL_SET = TStringBuf(R"(
{
    "name": "theremin_play",
    "payload": {
        "internal_set": {
            "mode": 324
        }
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto TYPE_TEXT_DIRECTIVE = TStringBuf(R"(
{
    "name": "type",
    "payload": {
        "text": "TestText"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto TYPE_TEXT_SILENT_DIRECTIVE = TStringBuf(R"(
{
    "name": "type_silent",
    "payload": {
        "text": "TestText"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto UPDATE_DIALOG_INFO_DIRECTIVE = TStringBuf(R"(
{
    "name": "update_dialog_info",
    "payload": {
        "style": {
            "user_bubble_fill_color": "TestUserBubbleFillColor",
            "oknyx_error_colors": [
                "TestOknyxErrorColor"
            ],
            "skill_actions_text_color": "TestSkillActionsTextColor",
            "suggest_border_color": "TestSuggestBorderColor",
            "skill_bubble_fill_color": "TestSkillBubbleFillColor",
            "suggest_text_color": "TestSuggestTextColor",
            "oknyx_normal_colors": [
                "TestOknyxNormalColor"
            ],
            "suggest_fill_color": "TestSuggestFillColor",
            "user_bubble_text_color": "TestUserBubbleTextColor",
            "oknyx_logo": "TestOknyxLogo",
            "skill_bubble_text_color": "TestSkillBubbleTextColor"
        },
        "dark_style": {
            "user_bubble_fill_color": "TestUserBubbleFillColorDark",
            "oknyx_error_colors": [
                "TestOknyxErrorColorDark"
            ],
            "skill_actions_text_color": "TestSkillActionsTextColorDark",
            "suggest_border_color": "TestSuggestBorderColorDark",
            "skill_bubble_fill_color": "TestSkillBubbleFillColorDark",
            "suggest_text_color": "TestSuggestTextColorDark",
            "oknyx_normal_colors": [
                "TestOknyxNormalColorDark"
            ],
            "suggest_fill_color": "TestSuggestFillColorDark",
            "user_bubble_text_color": "TestUserBubbleTextColorDark",
            "oknyx_logo": "TestOknyxLogoDark",
            "skill_bubble_text_color": "TestSkillBubbleTextColorDark"
        },
        "menu_items": [
            {
                "url": "TestInnerUrl",
                "title": "TestInnerTitle"
            }
        ],
        "image_url": "TestImageUrl",
        "url": "TestUri",
        "title": "TestTitle",
        "ad_block_id": "TestAdBlockId"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto VIDEO_PLAY_DIRECTIVE = TStringBuf(R"(
{
    "name": "video_play",
    "payload": {
        "item": {
            "available": 1,
            "description": "Михалков - власть, гимн, BadComedian / вДудь - YouTube. Открывай счет для бизнеса в Альфа-Банке...",
            "duration": 6548,
            "name": "Михалков - власть, гимн, BadComedian",
            "next_items": [
                {
                    "available": 1,
                    "description": "вДудь VS Михалков..!  - YouTube. сергей михеев,николай стариков,владимир соловьёв,анатолий шарий,александр семченко,дмитрий пучков,геополитика...",
                    "duration": 2398,
                    "name": "вДудь VS Михалков..!",
                    "play_uri": "youtube://pl0vRkF8YWY",
                    "price_from": 4,
                    "provider_info": [
                        {
                            "available": 1,
                            "provider_item_id": "pl0vRkF8YWY",
                            "provider_name": "youtube",
                            "type": "video"
                        }
                    ],
                    "provider_item_id": "pl0vRkF8YWY",
                    "provider_name": "youtube",
                    "source_host": "www.youtube.com",
                    "thumbnail_url_16x9": "https://avatars.mds.yandex.net/get-vthumb/903110/bf75f87d3ca3b056853e531e6d6f6e5b/800x360",
                    "thumbnail_url_16x9_small": "https://avatars.mds.yandex.net/get-vthumb/903110/bf75f87d3ca3b056853e531e6d6f6e5b/800x360",
                    "type": "video",
                    "view_count": 1
                },
                {
                    "available": 1,
                    "description": "Михалков, Дудь, Путин. Михалков,Дудь,Путин.",
                    "duration": 313,
                    "name": "Дудь загнал Михалкова в угол вопросами про Путина - фрагмент интервью у Дудя",
                    "play_uri": "youtube://eQsqt55soek",
                    "price_from": 15,
                    "provider_info": [
                        {
                            "available": 1,
                            "provider_item_id": "eQsqt55soek",
                            "provider_name": "youtube",
                            "type": "video"
                        }
                    ],
                    "provider_item_id": "eQsqt55soek",
                    "provider_name": "youtube",
                    "source_host": "www.youtube.com",
                    "thumbnail_url_16x9": "https://avatars.mds.yandex.net/get-vthumb/876582/90f2f04e641de50f896ae4cb2c131a25/800x360",
                    "thumbnail_url_16x9_small": "https://avatars.mds.yandex.net/get-vthumb/876582/90f2f04e641de50f896ae4cb2c131a25/800x360",
                    "type": "video",
                    "view_count": 2
                }
            ],
            "normalized_name": "михалков - власть гимн badcomedian english subs",
            "play_uri": "youtube://6cjcgu865ok",
            "provider_info": [
                {
                    "available": 1,
                    "provider_item_id": "6cjcgu865ok",
                    "provider_name": "youtube",
                    "type": "video"
                }
            ],
            "provider_item_id": "6cjcgu865ok",
            "provider_name": "youtube",
            "source_host": "www.youtube.com",
            "thumbnail_url_16x9": "https://avatars.mds.yandex.net/get-vthumb/752126/4e091aa4f646ef479e2f6aebd37b11ba/800x360",
            "thumbnail_url_16x9_small": "https://avatars.mds.yandex.net/get-vthumb/752126/4e091aa4f646ef479e2f6aebd37b11ba/800x360",
            "type": "video",
            "view_count": 3
        },
        "next_item": {
            "available": 1,
            "description": "вДудь VS Михалков..!  - YouTube. сергей михеев,николай стариков,владимир соловьёв,анатолий шарий,александр семченко,дмитрий пучков,геополитика...",
            "duration": 2398,
            "name": "вДудь VS Михалков..!",
            "play_uri": "youtube://pl0vRkF8YWY",
            "price_from": 4,
            "provider_info": [
                {
                    "available": 1,
                    "provider_item_id": "pl0vRkF8YWY",
                    "provider_name": "youtube",
                    "type": "video"
                }
            ],
            "provider_item_id": "pl0vRkF8YWY",
            "provider_name": "youtube",
            "source_host": "www.youtube.com",
            "thumbnail_url_16x9": "https://avatars.mds.yandex.net/get-vthumb/903110/bf75f87d3ca3b056853e531e6d6f6e5b/800x360",
            "thumbnail_url_16x9_small": "https://avatars.mds.yandex.net/get-vthumb/903110/bf75f87d3ca3b056853e531e6d6f6e5b/800x360",
            "type": "video",
            "view_count": 5
        },
        "uri": "youtube://6cjcgu865ok"
    },
    "sub_name": "video_play",
    "type": "client_action"
}
)");

inline constexpr auto YANDEXNAVI_DIRECTIVE = TStringBuf(R"(
{
    "name": "yandexnavi",
    "payload": {
        "application": "yandexnavi",
        "intent": "map_search",
        "params": {
            "text": "TestText"
        }
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto CALLBACK_DIRECTIVE = TStringBuf(R"(
{
    "name": "TestAnalyticsType",
    "payload": {
        "@scenario_name": "TestScenarioName",
        "@request_id": "TestRequestId",
        "TestKey": "TestValue"
    },
    "type": "server_action",
    "is_led_silent": true,
    "ignore_answer": true
}
)");

inline constexpr auto CALLBACK_DIRECTIVE_WITH_MULTIROOM = TStringBuf(R"(
{
    "name": "TestAnalyticsType",
    "payload": {
        "@scenario_name": "TestScenarioName",
        "@request_id": "TestRequestId",
        "TestKey": "TestValue"
    },
    "type": "server_action",
    "is_led_silent": true,
    "ignore_answer": false,
    "multiroom_session_id": "12345"
}
)");

inline constexpr auto GET_NEXT_CALLBACK_DIRECTIVE = TStringBuf(R"(
{
    "name": "@@mm_stack_engine_get_next",
    "payload": {
        "@scenario_name": "TestScenarioName",
        "@request_id": "TestRequestId",
        "stack_session_id": "session_id",
        "stack_product_scenario_name": "product_scenario_name"
    },
    "type": "server_action",
    "is_led_silent": true,
    "ignore_answer": false,
    "multiroom_session_id": "12345"
}
)");

inline constexpr auto ADD_CONTACT_BOOK_ASR_DIRECTIVE = TStringBuf(R"(
{
    "name": "add_contact_book_asr",
    "payload": {},
    "type": "uniproxy_action"
}
)");

inline constexpr auto DEFER_APPLY_DIRECTIVE = TStringBuf(R"(
{
    "name": "defer_apply",
    "payload": {
        "session": "TestSession"
    },
    "type": "uniproxy_action"
}
)");

inline constexpr auto VISUAL_GO_BACK_DIRECTIVE = TStringBuf(R"(
{
    "name": "go_back",
    "sub_name": "go_back",
    "payload": {
        "visual": {}
    },
    "type": "client_action"
}
)");

inline constexpr auto HISTORICAL_GO_BACK_DIRECTIVE = TStringBuf(R"(
{
    "name": "go_back",
    "sub_name": "go_back",
    "payload": {
        "historical": {
            "history_url": "http://example.com"
        }
    },
    "type": "client_action"
}
)");

inline constexpr auto NATIVE_GO_BACK_DIRECTIVE = TStringBuf(R"(
{
    "name": "go_back",
    "sub_name": "go_back",
    "payload": {
        "native": {}
    },
    "type": "client_action"
}
)");

inline constexpr auto SHOW_TV_GALLERY_DIRECTIVE = TStringBuf(R"(
{
    "name": "show_tv_gallery",
    "sub_name": "TestAnalyticsType",
    "type": "client_action",
    "payload": {
        "items": [{
            "channel_type": "personal",
            "description": "Эфир этого канала формируется автоматически на основании ваших предпочтений — того, что вы любите смотреть.",
            "name": "Мой Эфир",
            "provider_item_id": "4461546c4debdcffbab506fd75246e19",
            "provider_name": "strm",
            "relevance": 100500,
            "thumbnail_url_16x9": "https://avatars.mds.yandex.net/get-vh/1583218/2a0000016ad564e3344385fcfa3a92eec7f4/640x360",
            "thumbnail_url_16x9_small": "https://avatars.mds.yandex.net/get-vh/1583218/2a0000016ad564e3344385fcfa3a92eec7f4/640x360",
            "tv_episode_name": "Пол это Лава в Роблокс! Котёнок Лайк против Котика Игромана / Roblox The Floor is Lava",
            "tv_stream_info": {
                "channel_type": "personal",
                "is_personal": 1,
                "tv_episode_id": "45f85199853131d2b50b3c78410b5c59",
                "tv_episode_name": "Пол это Лава в Роблокс! Котёнок Лайк против Котика Игромана / Roblox The Floor is Lava"
            },
            "type": "tv_stream"
        }]
    }
}
)");

inline constexpr auto MESSENGER_CALL_TO_RECIPIENT = TStringBuf(R"(
{
    "name": "messenger_call",
    "type": "client_action",
    "payload": {
        "call_to_recipient": {
            "recipient": {
                "name": "name",
                "guid": "guid"
            }
        }
    }
}
)");

inline constexpr auto MESSENGER_ACCEPT_CALL = TStringBuf(R"(
{
    "name": "messenger_call",
    "type": "client_action",
    "payload": {
        "accept_call": {
            "call_guid": "callguid"
        }
    }
}
)");

inline constexpr auto MESSENGER_DECLINE_CURRENT_CALL = TStringBuf(R"(
{
    "name": "messenger_call",
    "type": "client_action",
    "payload": {
        "decline_current_call": {
            "call_guid": "callguid"
        }
    }
}
)");

inline constexpr auto MESSENGER_DECLINE_INCOMING_CALL = TStringBuf(R"(
{
    "name": "messenger_call",
    "type": "client_action",
    "payload": {
        "decline_incoming_call": {
            "call_guid": "callguid"
        }
    }
}
)");

inline constexpr auto SAVE_VOICEPRINT = TStringBuf(R"(
{
    "type": "uniproxy_action",
    "name": "save_voiceprint",
    "payload": {
        "requests": [
            "bf0d764f-064d-411f-916a-98f4d4fc8b60",
            "f3ec7027-60f7-4972-93ef-d6a6ce8a984b",
            "d3421dfa-84eb-4d20-8b40-b559b9c21da0",
            "9db13883-fd5d-4ea3-abe6-7f6fa2bb7838",
            "e57a7c3e-a248-4737-a602-8e6352b0f1d9"
        ],
        "user_id": "687820164",
        "user_type": "Guest",
        "pers_id": "PersId-4e978544-ef24409e-5880270f-46853b36"
    }
}
)");

inline constexpr auto REMOVE_VOICEPRINT = TStringBuf(R"(
{
    "name": "remove_voiceprint",
    "payload": {
        "user_id": "851083725",
        "pers_id": "PersId-4e978544-ef24409e-5880270f-46853b36"
    },
    "type": "uniproxy_action"
}
)");

inline constexpr auto MULTIACCOUNT_REMOVE_ACCOUNT = TStringBuf(R"(
{
    "name": "multiaccount_remove_account",
    "payload": {
        "puid": 851083725,
    },
    "type": "client_action"
}
)");

inline constexpr auto CHANGE_AUDIO_DIRECTIVE = TStringBuf(R"(
{
    "name": "change_audio",
    "payload": {
        "language": "eng",
        "title": "Английская"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto CHANGE_SUBTITLES_DIRECTIVE_OFF = TStringBuf(R"(
{
    "name": "change_subtitles",
    "payload": {
        "language": null,
        "title": null,
        "enable": false
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto CHANGE_SUBTITLES_DIRECTIVE_ON = TStringBuf(R"(
{
    "name": "change_subtitles",
    "payload": {
        "language": "eng",
        "title": "Английские",
        "enable": true
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto SHOW_VIDEO_SETTINGS_DIRECTIVE = TStringBuf(R"(
{
    "name": "show_video_settings",
    "payload": {},
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto DRAW_LED_SCREEN_DIRECTIVE = TStringBuf(R"(
{
    "name": "draw_led_screen",
    "payload": {
        "animation_sequence": [
            {
                "frontal_led_image": "https://quasar.s3.yandex.net/led_screen/cloud-3.gif"
            },
            {
                "frontal_led_image": "https://quasar.s3.yandex.net/led_screen/cloud-4.gif",
                "endless": true
            }
        ],
        "till_end_of_speech": true
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto SEND_BUG_REPORT_DIRECTIVE = TStringBuf(R"(
{
    "name": "send_bug_report",
    "payload": {
        "id": "abacabadabacaba"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto OPEN_DISK = TStringBuf(R"(
{
    "name": "open_disk",
    "payload": {
        "disk": "f"
    },
    "sub_name": "TestAnalyticsType",
    "type": "client_action"
}
)");

inline constexpr auto NOTIFY_DIRECTIVE_DELICATE = TStringBuf(R"(
{
    "type": "client_action",
    "name": "notify",
    "payload": {
        "notifications": [
            {
                "text": "Новости дня: Орел убил кукушку и узнал страшное...Читайте дальше на нашем сайте",
                "id": "bf0d159a-064d-411f-916a-98f4d4fc8b60",
                "subscription_id": "42"
            }],
        "ring": "Delicate",
        "version_id": "123123123123"
    },
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto NOTIFY_DIRECTIVE_PROACTIVE = TStringBuf(R"(
{
    "type": "client_action",
    "name": "notify",
    "payload": {
        "notifications": [
            {
                "text": "Новости дня: Орел убил кукушку и узнал страшное...Читайте дальше на нашем сайте",
                "id": "bf0d159a-064d-411f-916a-98f4d4fc8b60",
                "subscription_id": "42"
            }],
        "ring": "Proactive",
        "version_id": "123123123123"
    },
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto NOTIFY_DIRECTIVE_NOSOUND = TStringBuf(R"(
{
    "type": "client_action",
    "name": "notify",
    "payload": {
        "notifications": [
            {
                "text": "Новости дня: Орел убил кукушку и узнал страшное...Читайте дальше на нашем сайте",
                "id": "bf0d159a-064d-411f-916a-98f4d4fc8b60",
                "subscription_id": "42"
            }],
        "ring": "NoSound",
        "version_id": "123123123123"
    },
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto AUDIO_REWIND_DIRECTIVE = TStringBuf(R"(
{
    "name": "audio_player_rewind",
    "payload": {
    "amount_ms": 10,
    "type": "Absolute"
},
"type": "client_action",
"sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto UPDATE_NOTIFICATION_SUBSCRIPTION = TStringBuf(R"(
{
    "name": "update_notification_subscription",
    "payload": {
        "unsubscribe": true,
        "subscription_id": 123
    },
    "type": "uniproxy_action"
}
)");

inline constexpr auto UPDATE_NOTIFICATION_SUBSCRIPTION_DEFAULTS = TStringBuf(R"(
{
    "name": "update_notification_subscription",
    "payload": {
        "unsubscribe": false,
        "subscription_id": 0
    },
    "type": "uniproxy_action"
}
)");

inline constexpr auto MARK_NOTIFICATION_AS_READ = TStringBuf(R"(
{
    "name": "mark_notification_as_read",
    "payload": {
        "notification_id": "123",
        "notification_ids": ["1", "2"]
    },
    "type": "uniproxy_action"
}
)");

inline constexpr auto AUDIO_PLAY_DIRECTIVE = TStringBuf(R"(
{
  "name": "audio_play",
  "payload": {
    "stream": {
      "format": "MP3",
      "type": "Track",
      "id": "token",
      "offset_ms": 5000,
      "url": "https://s3.yandex.net/record.mp3",
      "normalization": {
          "integrated_loudness": -0.7,
          "true_peak": -0.13
      }
    },
    "metadata": {
      "art_image_url": "https://img.jpg",
      "glagol_metadata": {
        "music_metadata": {
          "description": "Описание",
          "id": "12345",
          "type": "Track",
          "prev_track_info": {
            "id": "12344",
            "type": "Track"
          },
          "next_track_info": {
            "id": "12346",
            "type": "Track"
          },
          "shuffled": true,
          "repeat_mode": "All"
        }
      },
      "subtitle": "subtitle",
      "title": "title",
      "hide_progress_bar": false
    },
    "set_pause": false,
    "multiroom_token": "TestMultiroomToken",
    "callbacks": {
      "on_failed": {
        "ignore_answer": true,
        "name": "on_failed",
        "type": "server_action",
        "is_led_silent": true,
        "payload": {
          "@scenario_name": "TestScenarioName",
          "@request_id": "TestRequestId",
          "skillId": "test-skill-id-value"
        }
      },
      "on_finished": {
        "ignore_answer": true,
        "name": "on_finished",
        "type": "server_action",
        "is_led_silent": true,
        "payload": {
          "@scenario_name": "TestScenarioName",
          "@request_id": "TestRequestId",
          "skillId": "test-skill-id-value"
        }
      },
      "on_started": {
        "ignore_answer": true,
        "name": "on_started",
        "type": "server_action",
        "is_led_silent": true,
        "payload": {
          "@scenario_name": "TestScenarioName",
          "@request_id": "TestRequestId",
          "skillId": "test-skill-id-value"
        }
      },
      "on_stopped": {
        "ignore_answer": true,
        "type": "server_action",
        "is_led_silent": true,
        "name": "on_stopped",
        "payload": {
          "@scenario_name": "TestScenarioName",
          "@request_id": "TestRequestId",
          "skillId": "test-skill-id-value"
        }
      }
    },
    "scenario_meta": {
      "skillId": "test-skill-id-value",
      "@scenario_name": "TestScenarioName"
    },
    "background_mode": "Ducking",
    "provider_name": "ЛитРес",
    "screen_type": "Default"
  },
  "type": "client_action",
  "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto AUDIO_PLAY_DIRECTIVE_SOME_CALLBACKS = TStringBuf(R"(
{
  "name": "audio_play",
  "payload": {
    "stream": {
      "format": "MP3",
      "type": "Track",
      "id": "token",
      "offset_ms": 5000,
      "url": "https://s3.yandex.net/record.mp3",
      "normalization": {
          "integrated_loudness": -0.7,
          "true_peak": -0.13
      }
    },
    "metadata": {
      "art_image_url": "https://img.jpg",
      "glagol_metadata": {
        "music_metadata": {
          "description": "Описание",
          "id": "12345",
          "type": "Track",
          "prev_track_info": {
            "id": "12344",
            "type": "Track"
          },
          "next_track_info": {
            "id": "12346",
            "type": "Track"
          },
          "shuffled": true,
          "repeat_mode": "All"
        }
      },
      "subtitle": "subtitle",
      "title": "title",
      "hide_progress_bar": false
    },
    "set_pause": false,
    "multiroom_token": "TestMultiroomToken",
    "callbacks": {
      "on_started": {
        "ignore_answer": true,
        "name": "on_started",
        "type": "server_action",
        "is_led_silent": true,
        "payload": {
          "@scenario_name": "TestScenarioName",
          "@request_id": "TestRequestId",
          "skillId": "test-skill-id-value"
        }
      },
      "on_stopped": {
        "ignore_answer": true,
        "type": "server_action",
        "is_led_silent": true,
        "name": "on_stopped",
        "payload": {
          "@scenario_name": "TestScenarioName",
          "@request_id": "TestRequestId",
          "skillId": "test-skill-id-value"
        }
      }
    },
    "scenario_meta": {
      "skillId": "test-skill-id-value",
      "@scenario_name": "TestScenarioName"
    },
    "background_mode": "Ducking",
    "provider_name": "ЛитРес",
    "screen_type": "Default"
  },
  "type": "client_action",
  "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto AUDIO_PLAY_DIRECTIVE_NO_CALLBACKS = TStringBuf(R"(
{
  "name": "audio_play",
  "payload": {
    "stream": {
      "format": "MP3",
      "type": "Track",
      "id": "token",
      "offset_ms": 5000,
      "url": "https://s3.yandex.net/record.mp3",
      "normalization": {
          "integrated_loudness": -0.7,
          "true_peak": -0.13
      }
    },
    "metadata": {
      "art_image_url": "https://img.jpg",
      "glagol_metadata": {
        "music_metadata": {
          "description": "Описание",
          "id": "12345",
          "type": "Track",
          "prev_track_info": {
            "id": "12344",
            "type": "Track"
          },
          "next_track_info": {
            "id": "12346",
            "type": "Track"
          },
          "shuffled": true,
          "repeat_mode": "All"
        }
      },
      "subtitle": "subtitle",
      "title": "title",
      "hide_progress_bar": false
    },
    "set_pause": false,
    "multiroom_token": "TestMultiroomToken",
    "callbacks": {},
    "scenario_meta": {
      "skillId": "test-skill-id-value",
      "@scenario_name": "TestScenarioName"
    },
    "background_mode": "Ducking",
    "provider_name": "ЛитРес",
    "screen_type": "Default"
  },
  "type": "client_action",
  "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto AUDIO_PLAY_DIRECTIVE_WITHOUT_GLAGOL_METADATA = TStringBuf(R"(
{
  "name": "audio_play",
  "room_device_ids": [
      "device_id_1",
      "device_id_2",
      "device_id_3",
      "device_id_4"
  ],
  "payload": {
    "stream": {
      "type": "Track",
      "format": "MP3",
      "id": "token",
      "offset_ms": 5000,
      "url": "https://s3.yandex.net/record.mp3",
      "normalization": {
          "integrated_loudness": -0.7,
          "true_peak": -0.13
      }
    },
    "metadata": {
      "art_image_url": "https://img.jpg",
      "glagol_metadata": {
          "Stub": {
          }
      },
      "subtitle": "subtitle",
      "title": "title"
    },
    "set_pause": false,
    "multiroom_token": "TestMultiroomToken",
    "callbacks": {
      "on_failed": {
        "ignore_answer": true,
        "name": "on_failed",
        "type": "server_action",
        "is_led_silent": true,
        "payload": {
          "@scenario_name": "TestScenarioName",
          "@request_id": "TestRequestId",
          "skillId": "test-skill-id-value"
        }
      },
      "on_finished": {
        "ignore_answer": true,
        "name": "on_finished",
        "type": "server_action",
        "is_led_silent": true,
        "payload": {
          "@scenario_name": "TestScenarioName",
          "@request_id": "TestRequestId",
          "skillId": "test-skill-id-value"
        }
      },
      "on_started": {
        "ignore_answer": true,
        "name": "on_started",
        "type": "server_action",
        "is_led_silent": true,
        "payload": {
          "@scenario_name": "TestScenarioName",
          "@request_id": "TestRequestId",
          "skillId": "test-skill-id-value"
        }
      },
      "on_stopped": {
        "ignore_answer": true,
        "type": "server_action",
        "is_led_silent": true,
        "name": "on_stopped",
        "payload": {
          "@scenario_name": "TestScenarioName",
          "@request_id": "TestRequestId",
          "skillId": "test-skill-id-value"
        }
      }
    },
    "scenario_meta": {
      "skillId": "test-skill-id-value",
      "@scenario_name": "TestScenarioName"
    },
    "background_mode": "Ducking",
    "provider_name": "ЛитРес",
    "screen_type": "Default"
  },
  "type": "client_action",
  "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto START_BROADCAST_DIRECTIVE = TStringBuf(R"(
{
    "type": "client_action",
    "name": "start_broadcast",
    "payload": {
        "timeout_ms": 30000
    },
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto STOP_BROADCAST_DIRECTIVE = TStringBuf(R"(
{
    "type": "client_action",
    "name": "stop_broadcast",
    "payload": {},
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto IOT_DISCOVERY_START_DIRECTIVE = TStringBuf(R"(
{
    "type": "client_action",
    "name": "iot_discovery_start",
    "payload": {
        "timeout_ms": 30000,
        "device_type": "devices.types.light",
        "ssid": "my_ssid"
    },
    "sub_name": "TestAnalyticsType",
}
)");

inline constexpr auto IOT_DISCOVERY_CREDENTIALS_DIRECTIVE = TStringBuf(R"(
{
    "type": "client_action",
    "name": "iot_discovery_credentials",
    "payload": {
        "ssid": "my_ssid",
        "password": "my_password",
        "token": "token",
        "cipher": "cipher"
    },
    "sub_name": "TestAnalyticsType",
}
)");

inline constexpr auto IOT_DISCOVERY_STOP_DIRECTIVE = TStringBuf(R"(
{
    "type": "client_action",
    "name": "iot_discovery_stop",
    "payload": {},
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto TTS_PLAY_PLACEHOLDER = TStringBuf(R"(
{
    "name": "tts_play_placeholder",
    "sub_name": "TestAnalyticsType",
    "payload": {
        "channel": "Dialog"
    },
    "type": "client_action"
}
)");

inline constexpr auto SETUP_RCU_DIRECTIVE = TStringBuf(R"(
{
    "name": "setup_rcu",
    "sub_name": "TestAnalyticsType",
    "payload": {},
    "type": "client_action"
}
)");

inline constexpr auto SETUP_RCU_AUTO_DIRECTIVE = TStringBuf(R"(
{
    "name": "setup_rcu_auto",
    "sub_name": "TestAnalyticsType",
    "payload": {
        "tv_model": "TestValue"
    },
    "type": "client_action"
}
)");

inline constexpr auto SETUP_RCU_CHECK_DIRECTIVE = TStringBuf(R"(
{
    "name": "setup_rcu_check",
    "sub_name": "TestAnalyticsType",
    "payload": {},
    "type": "client_action"
}
)");

inline constexpr auto SETUP_RCU_MANUAL_DIRECTIVE = TStringBuf(R"(
{
    "name": "setup_rcu_manual",
    "sub_name": "TestAnalyticsType",
    "payload": {},
    "type": "client_action"
}
)");

inline constexpr auto SETUP_RCU_ADVANCED_DIRECTIVE = TStringBuf(R"(
{
    "name": "setup_rcu_advanced",
    "sub_name": "TestAnalyticsType",
    "payload": {},
    "type": "client_action"
}
)");

inline constexpr auto FIND_RCU_DIRECTIVE = TStringBuf(R"(
{
    "name": "find_rcu",
    "sub_name": "TestAnalyticsType",
    "payload": {},
    "type": "client_action"
}
)");

inline constexpr auto FORCE_DISPLAY_CARDS_DIRECTIVE = TStringBuf(R"(
{
    "name": "force_display_cards",
    "sub_name": "TestAnalyticsType",
    "payload": {},
    "type": "client_action"
}
)");

inline constexpr auto LISTEN_DIRECTIVE_DEFAULT = TStringBuf(R"(
{
    "name": "listen",
    "sub_name": "TestAnalyticsType",
    "payload": {},
    "type": "client_action"
}
)");

inline constexpr auto LISTEN_DIRECTIVE_TIMEOUT = TStringBuf(R"(
{
    "name": "listen",
    "sub_name": "TestAnalyticsType",
    "payload": {
        "starting_silence_timeout_ms": 10000
    },
    "type": "client_action"
}
)");

inline constexpr auto UPDATE_DATASYNC_DIRECTIVE = TStringBuf(R"(
{
    "name": "update_datasync",
    "payload": {
      "key": "Test/Key",
      "value": "TestValue",
      "method": "PUT",
      "listening_is_possible": true
    },
    "type": "uniproxy_action"
}
)");

inline constexpr auto UPDATE_DATASYNC_DIRECTIVE_APPLY_FOR_OWNER = TStringBuf(R"(
{
    "name": "update_datasync",
    "payload": {
      "key": "Test/Key",
      "value": "TestValue",
      "method": "PUT",
      "listening_is_possible": true
    },
    "uniproxy_directive_meta": {
      "puid": "TestOwnerPuid"
    },
    "type": "uniproxy_action"
}
)");

inline constexpr auto UPDATE_DATASYNC_DIRECTIVE_APPLY_FOR_CURRENT_USER = TStringBuf(R"(
{
    "name": "update_datasync",
    "payload": {
      "key": "Test/Key",
      "value": "TestValue",
      "method": "PUT",
      "listening_is_possible": true
    },
    "uniproxy_directive_meta": {
      "puid": "TestGuestPuid"
    },
    "type": "uniproxy_action"
}
)");

inline constexpr auto UPDATE_DATASYNC_DIRECTIVE_STRUCT = TStringBuf(R"(
{
    "name": "update_datasync",
    "payload": {
      "key": "Test/Key",
      "value": {
        "subkey": "value",
        "subkey2": 123
      },
      "method": "PUT",
      "listening_is_possible": true
    },
    "type": "uniproxy_action"
}
)");

inline constexpr auto PUSH_MESSAGE_DIRECTIVE = TStringBuf(R"(
{
    "name": "push_message",
    "payload": {
        "title": "dummy title",
        "body": "open me",
        "link": "yandex.ru",
        "push_id": "test_id",
        "push_tag": "test_id",
        "throttle_policy": "alice_push_policy",
        "app_types": ["AT_SEARCH_APP"]
    },
    "type": "uniproxy_action"
}
)");

inline constexpr auto PUSH_MESSAGE_DIRECTIVE_WITH_PLACEHOLDER = TStringBuf(R"(
{
    "name": "push_message",
    "payload": {
        "title": "dummy title",
        "body": "open me",
        "link": "dialog-action://?directives=%5B%7B%22name%22%3A%22start_image_recognizer%22%2C%22payload%22%3A%7B%7D%2C%22sub_name%22%3A%22TestInnerAnalyticsType%22%2C%22type%22%3A%22client_action%22%7D%2C%7B%22ignore_answer%22%3Atrue%2C%22is_led_silent%22%3Atrue%2C%22name%22%3A%22on_suggest%22%2C%22payload%22%3A%7B%22%40request_id%22%3A%22TestRequestId%22%2C%22%40scenario_name%22%3A%22Vins%22%2C%22button_id%22%3A%22deadbeef%22%2C%22caption%22%3A%22DeepLink%22%2C%22request_id%22%3A%22TestRequestId%22%2C%22scenario_name%22%3A%22TestScenarioName%22%7D%2C%22type%22%3A%22server_action%22%7D%5D",
        "push_id": "test_id",
        "push_tag": "test_id",
        "throttle_policy": "alice_push_policy",
        "app_types": ["AT_SEARCH_APP"]
    },
    "type": "uniproxy_action"
}
)");

inline constexpr auto PERSONAL_CARDS_DIRECTIVE = TStringBuf(R"(
{
    "name": "personal_cards",
    "payload": {
        "card": {
            "card_id": "station_billing_12345",
            "button_url": "https://yandex.ru/quasar/id/kinopoisk/promoperiod",
            "text": "Активировать Яндекс.Плюс",
            "date_from": 1596398659,
            "date_to": 1596405859,
            "yandex.station_film": {
                "min_price": 0
            },
        },
        "remove_existing_cards": true
    },
    "type": "uniproxy_action"
}
)");


inline constexpr auto MEMENTO_CHANGE_USER_OBJECTS_DIRECTIVE = TStringBuf(R"(
{
    "name": "update_memento",
    "payload": {
        "user_objects": "CjgIAhI0Cil0eXBlLmdvb2dsZWFwaXMuY29tL2dvb2dsZS5wcm90b2J1Zi5WYWx1ZRIHGgV2YWx1ZRpSChBUZXN0U2NlbmFyaW9OYW1lEj4KKnR5cGUuZ29vZ2xlYXBpcy5jb20vZ29vZ2xlLnByb3RvYnVmLlN0cnVjdBIQCg4KA2tleRIHGgV2YWx1ZQ=="
    },
    "type": "uniproxy_action"
}
)");

inline constexpr auto PUSH_TYPED_SEMANTIC_FRAME_DIRECTIVE = TStringBuf(R"(
{
    "name": "push_typed_semantic_frame",
    "payload": {
        "puid": "13071999",
        "device_id": "MEGADEVICE_GOBLIN_3000",
        "ttl": 228,
        "semantic_frame_request_data": {
            "typed_semantic_frame": {
                "weather_semantic_frame": {
                    "when": {
                        "datetime_value": "13:07:1999"
                    }
                }
            },
            "analytics": {
                "product_scenario": "Weather",
                "origin": "Scenario"
            },
            "origin": {
                "device_id": "device_id_1",
                "uuid": "uuid_1"
            }
        }
    },
    "type": "uniproxy_action"
}
)");

inline constexpr auto MULTIROOM_SEMANTIC_FRAME_DIRECTIVE = TStringBuf(R"(
{
    "name": "multiroom_semantic_frame",
    "room_device_ids": [
        "device_id_2"
    ],
    "payload": {
        "body": {
            "name": "@@mm_semantic_frame",
            "payload": {
                "typed_semantic_frame": {
                    "weather_semantic_frame": {
                        "when": {
                            "datetime_value": "13:07:1999"
                        }
                    }
                },
                "analytics": {
                    "product_scenario": "Weather",
                    "origin": "Scenario"
                },
                "origin": {
                    "device_id": "device_id_1",
                    "uuid": "uuid_1"
                }
            },
            "type": "server_action"
        }
    },
    "type": "client_action"
}
)");

inline constexpr auto ADD_SCHEDULE_ACTION_DIRECTIVE = TStringBuf(R"(
{
    "name": "add_schedule_action",
    "payload": {
        "schedule_action": {
            "Id": "delivery_action",
            "Puid": "339124070",
            "DeviceId": "MOCK_DEVICE_ID",
            "StartPolicy": {
                "StartAtTimestampMs": 123
            },
            "SendPolicy": {
                "SendOncePolicy": {
                    "RetryPolicy": {
                        "MaxRetries": 1,
                        "RestartPeriodScaleMs": 200,
                        "RestartPeriodBackOff": 2,
                        "MinRestartPeriodMs": 10000,
                        "MaxRestartPeriodMs": 100000
                    }
                }
            },
            "Action": {
                "OldNotificatorRequest": {
                    "Delivery": {
                        "puid": "339124070",
                        "device_id": "MOCK_DEVICE_ID",
                        "ttl": 1,
                        "semantic_frame_request_data": {
                            "typed_semantic_frame": {
                                "iot_broadcast_start": {
                                    "pairing_token": {
                                        "StringValue": "token"
                                    }
                                }
                            },
                            "analytics": {
                                "purpose": "video"
                            }
                        }
                    }
                }
            }
        }
    }
    "type": "uniproxy_action"
}
)");

inline constexpr auto IOT_USER_INFO = TStringBuf(R"(
    {
        "devices": [
            {
                "quasar_info": {
                    "device_id": "device_id_1"
                },
                "group_ids": [
                    "group_1",
                    "group_2"
                ],
                "room_id": "room_1"
            },
            {
                "quasar_info": {
                    "device_id": "device_id_2"
                },
                "group_ids": [
                    "group_1"
                ],
                "room_id": "room_1"
            },
            {
                "quasar_info": {
                    "device_id": "device_id_3"
                },
                "group_ids": [
                    "group_2"
                ],
                "room_id": "room_2"
            },
            {
                "quasar_info": {
                    "device_id": "device_id_4"
                },
                "group_ids": [
                    "group_2",
                    "group_3"
                ],
                "room_id": "room_2"
            },
            {
                "quasar_info": {
                    "device_id": "device_id_5"
                },
                "group_ids": [
                    "group_3"
                ],
                "room_id": "room_1"
            }
        ]
    }
)");

inline constexpr auto ADD_CARD_DIRECTIVE = TStringBuf(R"(
    {
        "name": "add_card",
        "sub_name": "TestAnalyticsType",
        "type": "client_action",
        "payload": {
            "carousel_id": "id1",
            "card_id": "id2",
            "card_show_time_sec": 10,
            "title": "title",
            "image_url": "image_url",
            "type": "type",
            "div2_card": {
                "hide_borders": true,
                "body": {
                }
            },
            "div2_templates": {
            },
            "action_space_id": "1",
            "teaser_config": {
                "teaser_type": "type",
                "teaser_id": "id"
            }
        }
    }
)");

inline constexpr auto SET_UPPER_SHUTTER_DIRECTIVE = TStringBuf(R"(
    {
        "name": "set_upper_shutter",
        "sub_name": "TestAnalyticsType",
        "type": "client_action",
        "payload": {
            "div2_card": {
                "hide_borders": true,
                "body": {
                }
            },
        }
    }
)");

inline constexpr auto SET_MAIN_SCREEN_DIRECTIVE = TStringBuf(R"(
    {
      "name": "set_main_screen",
      "sub_name": "TestAnalyticsType",
      "type": "client_action",
      "payload": {
        "tabs": [
          {
            "id": "tab-1",
            "title": "Мой экран",
            "blocks": [
              {
                "id": "block-1",
                "title": "Послушать",
                "horizontal_mediagallery_block": {
                  "height": 244,
                  "cards": [
                    {
                        "id": "card-1",
                        "width": 244,
                        "card": {
                            "body": {
                                "TestKey": "dialog-action://?directives=%5B%7B%22name%22%3A%22start_image_recognizer%22%2C%22payload%22%3A%7B%7D%2C%22sub_name%22%3A%22TestInnerAnalyticsType%22%2C%22type%22%3A%22client_action%22%7D%5D"
                            }
                        }
                    },
                    {
                        "card": {
                            "body": {
                                "TestKey": "dialog-action://?directives=%5B%7B%22ignore_answer%22%3Afalse%2C%22name%22%3A%22%40%40mm_semantic_frame%22%2C%22payload%22%3A%7B%22analytics%22%3A%7B%22origin%22%3A%22Undefined%22%2C%22origin_info%22%3A%22%22%2C%22product_scenario%22%3A%22%22%2C%22purpose%22%3A%22%22%7D%2C%22params%22%3A%7B%22disable_output_speech%22%3Atrue%2C%22disable_should_listen%22%3Afalse%7D%2C%22typed_semantic_frame%22%3A%7B%22open_smart_device_external_app_frame%22%3A%7B%7D%7D%7D%2C%22type%22%3A%22server_action%22%7D%5D"
                            }
                        }
                    }
                  ]
                }
              }
            ]
          }
        ]
      }
    }
)");

inline constexpr auto SHOW_VIEW_DIRECTIVE = TStringBuf(R"(
    {
        "name": "show_view",
        "sub_name": "TestAnalyticsType",
        "type": "client_action",
        "payload": {
            "div2_card": {
                "hide_borders": true,
                "body": {}
            },
            "layer": {
                "content": {}
            },
            "inactivity_timeout": "Long",
            "do_not_show_close_button": false,
            "action_space_id": "1",
            "keep_stashed_if_possible": true,
            "layer_name": ""
        }
    }
)");    

inline constexpr auto PATCH_VIEW_DIRECTIVE = TStringBuf(R"(
    {
        "name": "patch_view",
        "sub_name": "TestAnalyticsType",
        "type": "client_action",
        "payload": {
            "div2_patch": {
                "body": {}
            },
            "apply_to": {
                "card_name": "A",
                "card_id": "1"
            }
        }
    }
)");

inline constexpr auto STASH_VIEW_DIRECTIVE = TStringBuf(R"(
    {
        "name": "stash_view",
        "sub_name": "TestAnalyticsType",
        "type": "client_action",
        "payload": {
            "card_search_criteria": {
                "card_name": "A",
                "card_id": "1"
            }
        }
    }
)");

inline constexpr auto UNSTASH_VIEW_DIRECTIVE = TStringBuf(R"(
    {
        "name": "unstash_view",
        "sub_name": "TestAnalyticsType",
        "type": "client_action",
        "payload": {
            "card_search_criteria": {
                "card_name": "A",
                "card_id": "1"
            }
        }
    }
)");

inline constexpr auto DIVUI_SHOW_VIEW_DIRECTIVE = TStringBuf(R"(
    {
        "name": "divui_show_view",
        "sub_name": "TestAnalyticsType",
        "type": "client_action",
        "payload": {
            "div2_card": {
                "hide_borders": true,
                "body": {
                }
            },
            "layer": {
                "content": {
                }
            },
            "inactivity_timeout": "Long",
            "action_space_id": "1",
            "stash_interaction": "ShowUnstashed"
        }
    }
)");

inline constexpr auto DIVUI_PATCH_VIEW_DIRECTIVE = TStringBuf(R"(
    {
        "name": "divui_patch_view",
        "sub_name": "TestAnalyticsType",
        "type": "client_action",
        "payload": {
            "div2_patch": {
                "body": {}
            },
            "apply_to": {
                "card_name": "A",
                "card_id": "1"
            }
        }
    }
)");

inline constexpr auto DIVUI_STASH_VIEW_DIRECTIVE = TStringBuf(R"(
    {
        "name": "divui_stash_view",
        "sub_name": "TestAnalyticsType",
        "type": "client_action",
        "payload": {
            "card_search_criteria": {
                "card_name": "A",
                "card_id": "1"
            }
        }
    }
)");

inline constexpr auto DIVUI_UNSTASH_VIEW_DIRECTIVE = TStringBuf(R"(
    {
        "name": "divui_unstash_view",
        "sub_name": "TestAnalyticsType",
        "type": "client_action",
        "payload": {
            "card_search_criteria": {
                "card_name": "A",
                "card_id": "1"
            }
        }
    }
)");


inline constexpr auto ROTATE_CARDS_DIRECTIVE = TStringBuf(R"(
    {
        "name": "rotate_cards",
        "sub_name": "TestAnalyticsType",
        "type": "client_action",
        "payload": {
            "carousel_id": "TestUid",
            "carousel_show_time_sec": 100,
        }
    }
)");

inline constexpr auto START_MULTIROOM_DIRECTIVE_WITH_ROOM_ID = TStringBuf(R"(
{
    "name": "start_multiroom",
    "payload": {
        "room_id": "room_1"
    },
    "type": "client_action",
    "room_device_ids": [
        "device_id_1",
        "device_id_2",
        "device_id_5"
    ]
}
)");

inline constexpr auto START_MULTIROOM_DIRECTIVE_WITH_LOCATION_INFO = TStringBuf(R"(
{
    "name": "start_multiroom",
    "payload": {
        "room_id": "group_3",
        "room_device_ids": [
            "device_id_1",
            "device_id_3",
            "device_id_4",
            "device_id_5"
        ]
    },
    "type": "client_action"
}
)");

inline constexpr auto START_MULTIROOM_DIRECTIVE_WITH_LOCATION_INFO_2 = TStringBuf(R"(
{
    "name": "start_multiroom",
    "sub_name": "TestAnalyticsType",
    "payload": {
        "room_id": "room_2",
        "room_device_ids": [
            "device_id_1",
            "device_id_3",
            "device_id_4"
        ]
    },
    "type": "client_action"
}
)");

inline constexpr auto START_MULTIROOM_DIRECTIVE_WITH_LOCATION_INFO_3 = TStringBuf(R"(
{
    "name": "start_multiroom",
    "payload": {
        "room_id": "__all__",
        "room_device_ids": [
            "device_id_1",
            "device_id_2",
            "device_id_3",
            "device_id_4",
            "device_id_5"
        ],
        "multiroom_token": "TestMultiroomToken",
    },
    "type": "client_action"
}
)");

inline constexpr auto SET_SMART_TV_CATEGORIES = TStringBuf(R"(
{
    "name": "set_smarttv_categories",
    "sub_name": "TestAnalyticsType",
    "type": "client_action",
    "payload": {
        "categories": [
          {
            "category_id": "id1",
            "title": "Category title 1",
            "icon": "http://mds/icon-1.png",
            "rank": 1
          }
        ]
      }
}
)");

inline constexpr auto TV_OPEN_SEARCH_SCREEN_DIRECTIVE = TStringBuf(R"(
{
    "name": "tv_open_search_screen",
    "sub_name": "TestAnalyticsType",
    "type": "client_action",
    "payload": {
        "search_query": "фильмы про спорт"
    }
}
)");

inline constexpr auto TV_OPEN_DETAILS_SCREEN_DIRECTIVE = TStringBuf(R"(
{
    "name": "tv_open_details_screen",
    "sub_name": "TestAnalyticsType",
    "type": "client_action",
    "payload": {
        "content_type": "MOVIE",
        "vh_uuid": "4a5bf03fd28452d1abe53f3801ff5e99",
        "search_query": "экипаж фильм 2016",
        "data": {
            "name": "Экипаж",
            "description": "Талантливый молодой лётчик Алексей Гущин не признаёт авторитетов, предпочитая поступать в соответствии с личным кодексом чести. За невыполнение абсурдного приказа его выгоняют из военной авиации, и только чудом он получает шанс летать на гражданских самолётах. Гущин начинает свою лётную жизнь сначала. Его наставник — командир воздушного судна — суровый и принципиальный Леонид Зинченко. Его коллега — второй пилот, неприступная красавица Александра. Отношения складываются непросто. Но на грани жизни и смерти, когда земля уходит из-под ног, вокруг — огонь и пепел, и только в небе есть спасение, Гущин показывает всё, на что он способен. Только вместе экипаж сможет совершить подвиг и спасти сотни жизней.",
            "hint_description": "2016, драма, триллер",
            "thumbnail": {
                "base_url": "https://avatars.mds.yandex.net/get-kinopoisk-image/1946459/bd25426a-073e-45e8-8c74-97d71c05ae64/",
                "sizes": ["orig"]
            },
            "poster": {
                "base_url": "http://avatars.mds.yandex.net/get-kino-vod-films-gallery/33804/2a000001528e2f061f0c6f28f5580ddb3ee4/",
                "sizes": ["360x540", "orig"]
            },
            "min_age": 6,
        }
    }
}
)");

inline constexpr auto TV_OPEN_SERIES_SCREEN_DIRECTIVE = TStringBuf(R"(
{
    "name": "tv_open_series_screen",
    "sub_name": "TestAnalyticsType",
    "type": "client_action",
    "payload": {
        "vh_uuid": "4faef5f0c695c590a0727142ee2f0a39"
    }
}
)");

inline constexpr auto TV_OPEN_PERSON_SCREEN_DIRECTIVE = TStringBuf(R"({
    "name": "tv_open_person_screen",
    "sub_name": "TestAnalyticsType",
    "type": "client_action",
    "payload": {
        "kp_id": "1054956",
        "data": {
            "name": "Данила Козловский",
            "subtitle": "Российский актёр",
            "image": {
                "base_url": "http://avatars.mds.yandex.net/get-kino-vod-persons-gallery/33886/2a00000151ca1a244f419b4191c2663771fc/",
                "sizes": ["orig"]
            }
        }
    }
}
)");

inline constexpr auto TV_OPEN_COLLECTION_SCREEN_DIRECTIVE = TStringBuf(R"({
    "name": "tv_open_collection_screen",
    "sub_name": "TestAnalyticsType",
    "type": "client_action",
    "payload": {
        "search_query": "Фильмы фестиваля Кинотавр",
        "entref": "0oEg9sc3QtNGI5ODUzM2IuLjAYAgjH4DQ",
        "data": {
            "title": "Фильмы фестиваля Кинотавр"
        }
    }
}
)");

inline constexpr auto SHOW_BUTTONS_DIRECTIVE = TStringBuf(R"({
    "name": "show_buttons",
    "payload": {
        "screen_id": "TestScreenId",
        "buttons": [{
            "directives": [{
                "name": "type",
                "sub_name": "",
                "payload": {
                    "text": "TestTypeText"
                },
                "type": "client_action"
            }, {
                "name": "external_source_action",
                "ignore_answer": true,
                "is_led_silent": false,
                "payload": {
                    "@request_id": "TestRequestId",
                    "@scenario_name": "TestScenarioName",
                    "utm_source": "Yandex_Alisa"
                },
                "type": "server_action"
            }],
            "title": "TestTitle_0",
            "text": "TestText_0",
            "theme": {
                "image_url": "TestImageUrl_0"
            },
            "type": "themed_action"
        }, {
            "directives": [{
                "name": "type",
                "sub_name": "",
                "payload": {
                    "text": "TestTypeText"
                },
                "type": "client_action"
            }, {
                "name": "external_source_action",
                "ignore_answer": true,
                "is_led_silent": false,
                "payload": {
                    "@request_id": "TestRequestId",
                    "@scenario_name": "TestScenarioName",
                    "utm_source": "Yandex_Alisa"
                },
                "type": "server_action"
            }],
            "title": "TestTitle_1",
            "text": "TestText_1",
            "theme": {
                "image_url": "TestImageUrl_1"
            },
            "type": "themed_action"
        }]
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto REQUEST_PERMISSIONS_DIRECTIVE = TStringBuf(R"({
    "name": "request_permissions",
    "sub_name": "TestAnalyticsType",
    "type": "client_action",
    "payload": {
        "permissions": [
            "location",
            "read_contacts"
        ],
        "on_success": [
            {
                "name": "type_silent",
                "type": "client_action",
                "sub_name": "TestAnalyticsType",
                "payload": {
                    "text": "Success!!!"
                }
            }
        ],
        "on_fail": [
            {
                "name": "type_silent",
                "type": "client_action",
                "sub_name": "TestAnalyticsType",
                "payload": {
                    "text": "Fail!!!"
                }
            }
        ]
    }
}
)");

inline constexpr auto REQUEST_PERMISSIONS_DIRECTIVE_WITHOUT_SUBDIRECTIVES = TStringBuf(R"({
    "name": "request_permissions",
    "sub_name": "TestAnalyticsType",
    "type": "client_action",
    "payload": {
        "permissions": [
            "call_phone"
        ]
    }
}
)");

inline constexpr auto UPDATE_SPACE_ACTIONS_DIRECTIVE = TStringBuf(R"({
    "name": "update_space_actions",
    "type": "client_action",
    "sub_name": "update_space_actions",
    "payload": {
        "action_space_id": {
            "frame_name": {
                "analytics": {
                    "origin": "Undefined",
                    "origin_info": "",
                    "product_scenario": "",
                    "purpose": ""
                },
                "typed_semantic_frame": {
                    "search_semantic_frame": {
                        "query": {
                            "string_value": "how are you?"
                        }
                    }
                }
            }
        }
    }
})");

inline constexpr auto ADD_CONDITIONAL_ACTIONS_DIRECTIVE = TStringBuf(R"({
    "name": "add_conditional_actions",
    "sub_name": "add_conditional_actions",
    "payload": {
        "action_1": {
            "effect_frame_request_data": {
                "typed_semantic_frame": {
                    "search_semantic_frame": {
                        "query": {
                            "string_value": "ok"
                        }
                    }
                },
                "origin": null,
                "analytics": null,
                "params": null,
                "request_params": null
            },
            "conditional_semantic_frame": {
                "search_semantic_frame": {
                    "query": {
                        "string_value": "how are you?"
                    }
                }
            }
        },
        "action_2": {
            "effect_frame_request_data": {
                "typed_semantic_frame": {
                    "search_semantic_frame": {
                        "query": {
                            "string_value": "net"
                        }
                    }
                },
                "origin": null,
                "analytics": null,
                "params": null,
                "request_params": null
            },
            "conditional_semantic_frame": {
                "player_pause_semantic_frame": {}
            }
        }
    },
    "type": "client_action"
})");

inline constexpr auto ADD_EXTERNAL_ENTITIES_DESCRIPTION_DIRECTIVE = TStringBuf(R"({
    "name": "add_external_entities_description",
    "sub_name": "add_external_entities_description",
    "payload": {
        "external_entities_description": [
            {
                "name": "entity_name",
                "items": [
                    {
                        "value": {
                            "string_value": "item_value"
                        },
                        "phrases": [
                            {
                                "phrase": "hi"
                            }
                        ]
                    }
                ]
            }
        ]
    },
    "type": "client_action"
})");

inline constexpr auto SEND_ANDROID_APP_INTENT_DIRECTIVE = TStringBuf(R"({
    "name": "send_android_app_intent",
    "sub_name": "TestAnalyticsType",
    "type": "client_action",
    "payload": {
        "action": "android.intent.action.VIEW",
        "uri": "vnd.youtube:dQw4w9WgXcQ",
        "category": "",
        "type": "",
        "start_type": "StartActivity",
        "component": {
            "pkg": "com.yandex.test",
            "cls": "com.yandex.test.Test",
        },
        "flags": {
            "FLAG_ACTIVITY_NEW_TASK": true
        },
        "analytics": {
            "app_launch": {
                "package_name": "com.yandex.test",
                "visible_name": "Test"
            }
        }
    }
})");

inline constexpr auto SEND_ANDROID_APP_INTENT_DIRECTIVE_WITHOUT_URI = TStringBuf(R"({
    "name": "send_android_app_intent",
    "sub_name": "TestAnalyticsType",
    "type": "client_action",
    "payload": {
        "action": "android.intent.action.VIEW",
        "category": "",
        "type": "",
        "start_type": "StartActivity",
        "component": {
            "pkg": "com.yandex.test",
            "cls": "com.yandex.test.Test",
        },
        "flags": {
            "FLAG_ACTIVITY_NEW_TASK": true
        },
        "analytics": {
            "app_launch": {
                "package_name": "com.yandex.test",
                "visible_name": "Test"
            }
        }
    }
})");

inline constexpr auto REQUEST_DRAW_SCLED_ANIMATIONS_DIRECTIVE_WITHOUT_STOP_POLICY = TStringBuf(R"({
    "name": "draw_scled_animations",
    "payload": {
        "animations": [
            {
              "name": "animation_1",
              "base64_encoded_value": "15 25 0",
              "compression_type": "None"
            }
        ],
        "animation_stop_policy" : "Unknown",
        "speaking_animation_policy" : "PlaySpeakingUnknown"
    },
    "type": "client_action",
    "sub_name": "draw_scled_animations"
}
)");

inline constexpr auto REQUEST_DRAW_SCLED_ANIMATIONS_DIRECTIVE_WITH_STOP_POLICY = TStringBuf(R"({
    "name": "draw_scled_animations",
    "payload": {
        "animations": [
            {
              "name": "animation_1",
              "base64_encoded_value": "15 25 0 155 \n15 5 14\n",
              "compression_type": "None"
            },
            {
              "name": "animation_2",
              "base64_encoded_value": "11 0 155 \n..... 2 5 14\n",
              "compression_type": "None"
            }
        ],
        "animation_stop_policy" : "PlayOnce",
        "speaking_animation_policy" : "ShowClockImmediately"
    },
    "type": "client_action",
    "sub_name": "draw_scled_animations"
}
)");

inline constexpr auto STOP_MULTIROOM_DIRECTIVE = TStringBuf(R"(
{
    "name": "stop_multiroom",
    "payload": {
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType",
    "multiroom_session_id": "12345"
}
)");

inline constexpr auto FILL_CLOUD_UI_DIRECTIVE = TStringBuf(R"(
{
    "name": "fill_cloud_ui",
    "payload": {
        "text": "TestText"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto WEB_OS_LAUNCH_APP_DIRECTIVE = TStringBuf(R"(
{
    "name": "web_os_launch_app_directive",
    "payload": {
        "app_id": "some.app.id",
        "params_json": "{}"
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto WEB_OS_SHOW_GALLERY_DIRECTIVE = TStringBuf(R"(
{
    "name": "web_os_show_gallery_directive",
    "payload": {
        "items_json": ["{}","{}"]
    },
    "type": "client_action",
    "sub_name": "TestAnalyticsType"
}
)");

inline constexpr auto AUDIO_MULTIROOM_ATTACH_DIRECTIVE = TStringBuf(R"(
{
    "name": "audio_multiroom_attach",
    "payload": {
        "multiroom_token": "TestMultiroomToken"
    },
    "type": "client_action"
}
)");

} // namespace NAlice::NMegamind
