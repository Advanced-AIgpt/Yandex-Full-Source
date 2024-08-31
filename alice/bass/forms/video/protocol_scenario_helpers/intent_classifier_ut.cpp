#include "intent_classifier.h"

#include <alice/library/json/json.h>
#include <alice/library/video_common/defs.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NScenarios;
using namespace NAlice::NVideoCommon;

namespace {

void TestCase(TStringBuf base, TStringBuf input, const TMap<int, TStringBuf>& dataMap, TStringBuf target) {
    TScenarioBaseRequest requestBase;
    JsonToProto(JsonFromString(base), requestBase);

    TInput requestInput;
    JsonToProto(JsonFromString(input), requestInput);

    TScenarioRunRequest request;
    *request.MutableBaseRequest() = requestBase;
    *request.MutableInput() = requestInput;
    for (const auto& data : dataMap) {
        TDataSource dataSource;
        JsonToProto(JsonFromString(data.second), dataSource);
        request.MutableDataSources()->insert(google::protobuf::MapPair<int, NAlice::NScenarios::TDataSource>(data.first, dataSource));
    }

    NAlice::TClientFeatures clientFeatures(TClientInfoProto{}, {});
    const auto choosen = NVideoProtocol::ChooseIntent(request, clientFeatures);
    Cerr << "Choosen intent: " << choosen << "; Target intent: " << target << Endl;
    UNIT_ASSERT_EQUAL(choosen, target);
}

void TestCase(TStringBuf base, TStringBuf input, TStringBuf target) {
    static const TMap<int, TStringBuf> EMPTY_DATA_MAP;
    TestCase(base, input, EMPTY_DATA_MAP, target);
}

constexpr TStringBuf BASE_MAIN_SCREEN = TStringBuf(R"(
{
    "device_state": {
        "video": {
            "current_screen": "main"
        }
    }
}
)");

constexpr TStringBuf BASE_DESCRIPTION_SCREEN = TStringBuf(R"(
{
    "device_state": {
        "video": {
            "current_screen": "description"
        }
    }
}
)");

constexpr TStringBuf BASE_WEBVIEW_VIDEO_ENTITY_SCREEN = TStringBuf(R"(
{
    "device_state": {
        "video": {
            "current_screen": "mordovia_webview",
            "view_state": {
                "currentScreen": "videoEntity"
            }
        }
    }
}
)");

constexpr TStringBuf BASE_WEBVIEW_VIDEO_ENTITY_DESCRIPTION_SCREEN = TStringBuf(R"(
{
    "device_state": {
        "video": {
            "current_screen": "mordovia_webview",
            "view_state": {
                "currentScreen": "videoEntity/Description"
            }
        }
    }
}
)");

constexpr TStringBuf BASE_WEBVIEW_VIDEO_ENTITY_CAROUSEL_SCREEN = TStringBuf(R"(
{
    "device_state": {
        "video": {
            "current_screen": "mordovia_webview",
            "view_state": {
                "currentScreen": "videoEntity/Carousel",
                "sections":[{
                    "items": [
                        {
                            "number": 1,
                            "active": true,
                            "title": "название видео"
                        },
                        {
                            "number": 2,
                            "active": true,
                            "title": "имя фильма"
                        }
                    ],
                }]

            }
        }
    }
}
)");


constexpr TStringBuf BASE_PAYMENT_SCREEN = TStringBuf(R"(
{
    "device_state": {
        "video": {
            "current_screen": "payment"
        }
    }
}
)");

constexpr TStringBuf BASE_VIDEO_PLAYER_SCREEN = TStringBuf(R"(
{
    "device_state": {
        "video": {
            "current_screen": "video_player"
        }
    }
}
)");

constexpr TStringBuf BASE_GALLERY_SCREEN = TStringBuf(R"(
{
    "device_state": {
        "video": {
            "current_screen": "gallery",
            "screen_state": {
                "items": [
                    {"name": "имя канала"},
                    {"name": "имя фильма"}
                ]
            }
        }
    },
    "experiments": {
        "mm_disable_begemot_item_selector": {
            "1": "1"
        }
    }
}
)");

constexpr TStringBuf BASE_WEBVIEW_VIDEOSEARCH_SCREEN = TStringBuf(R"(
{
    "device_state" : {
        "video": {
            "current_screen": "mordovia_webview",
            "view_state" : {
                "currentScreen" : "videoSearch",
                "sections":[{
                    "items": [
                        {
                            "number": 1,
                            "active": true,
                            "title": "название видео"
                        },
                        {
                            "number": 2,
                            "active": true,
                            "title": "имя фильма"
                        }
                    ],
                }]
            }
        }
    }
}
)");

constexpr TStringBuf BASE_WEBVIEW_FILMSSEARCH_SCREEN = TStringBuf(R"(
{
    "device_state" : {
        "video": {
            "current_screen": "mordovia_webview",
            "view_state" : {
                "currentScreen" : "filmsSearch",
                "sections":[{
                    "items": [
                        {
                            "number": 1,
                            "active": true,
                            "title": "название видео"
                        },
                        {
                            "number": 2,
                            "active": true,
                            "title": "имя фильма"
                        }
                    ],
                }]
            }
        }
    }
}
)");

constexpr TStringBuf BASE_TV_GALLERY_SCREEN = TStringBuf(R"(
{
    "device_state": {
        "video": {
            "current_screen": "tv_gallery",
            "screen_state": {
                "items": [
                    {"name": "имя канала"},
                    {"name": "имя фильма"}
                ]
            }
        }
    },
    "experiments": {
        "mm_disable_begemot_item_selector": {
            "1": "1"
        }
    }
}
)");

constexpr TStringBuf BASE_WEBVIEW_CHANNELS_SCREEN = TStringBuf(R"(
{
    "device_state": {
        "video": {
            "current_screen": "mordovia_webview",
            "view_state": {
                "currentScreen": "channels",
                "sections":[{
                    "items": [
                        {
                            "number": 1,
                            "active": true,
                            "title": "имя канала — имя передачи",
                            "name": "имя канала",
                            "tv_episode_name": "имя передачи"
                        },
                        {
                            "number": 2,
                            "active": false,
                            "title": "Первый канал — Доброе утро",
                            "name": "Первый канал",
                            "tv_episode_name": "Доброе утро"
                        },
                        {
                            "number": 3,
                            "active": false,
                            "title": "Волгоград 1 — Город-сказка",
                            "name": "Волгоград 1",
                            "tv_episode_name": "Город-сказка"
                        }
                    ],
                }]
            }
        }
    },
    "experiments": {
        "mm_disable_begemot_item_selector": {
            "1": "1"
        }
    }
}
)");

constexpr TStringBuf BASE_WEBVIEW_VIDEO_ENTITY_RELATED_CAROUSEL_SCREEN = TStringBuf(R"(
{
    "device_state": {
        "video": {
            "current_screen": "mordovia_webview",
            "view_state": {
                "currentScreen": "videoEntity/RelatedCarousel",
                "sections":[{
                    "items": [
                        {
                            "number": 1,
                            "active": true,
                            "title": "название видео"
                        },
                        {
                            "number": 2,
                            "active": true,
                            "title": "имя фильма"
                        }
                    ],
                }]
            }
        }
    }
}
)");

constexpr TStringBuf BASE_SEASON_GALLERY_SCREEN = TStringBuf(R"(
{
    "device_state": {
        "video": {
            "current_screen": "season_gallery",
            "screen_state": {
                "items": [
                    {"name": "имя канала"},
                    {"name": "имя фильма"}
                ]
            }
        }
    },
    "experiments": {
        "mm_disable_begemot_item_selector": {
            "1": "1"
        }
    }
}
)");

constexpr TStringBuf BASE_WEBVIEW_VIDEO_ENTITY_SEASONS_SCREEN = TStringBuf(R"(
{
    "device_state": {
        "video": {
            "current_screen": "mordovia_webview",
            "view_state": {
                "currentScreen": "videoEntity/Seasons",
                "sections":[{
                    "items": [
                        {
                            "number": 1,
                            "active": true,
                            "title": "название видео"
                        },
                        {
                            "number": 2,
                            "active": true,
                            "title": "имя фильма"
                        }
                    ],
                }]
            }
        }
    },
    "experiments": {
        "mm_disable_begemot_item_selector": {
            "1": "1"
        }
    }
}
)");

constexpr TStringBuf INPUT_VIDEO_PLAY = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.video_play"
        }
    ]
}
)");

constexpr TStringBuf INPUT_GOTO_VIDEO_SCREEN = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.quasar.goto_video_screen",
            "slots": [
                {
                    "name": "screen"
                }
            ]
        }
    ]
}
)");

constexpr TStringBuf INPUT_GOTO_VIDEO_SCREEN_EMPTY = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.quasar.goto_video_screen"
        }
    ]
}
)");

constexpr TStringBuf INPUT_PAYMENT_CONFIRMED = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.quasar.payment_confirmed"
        }
    ]
}
)");

constexpr TStringBuf INPUT_OPEN_CURRENT_VIDEO = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.quasar.open_current_video"
        }
    ]
}
)");

constexpr TStringBuf INPUT_OPEN_CURRENT_TRAILER = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "alice.video.open_current_trailer"
        }
    ]
}
)");

constexpr TStringBuf INPUT_AUTHORIZE_PROVIDER = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.quasar.authorize_video_provider"
        }
    ]
}
)");

constexpr TStringBuf INPUT_EMPTY = TStringBuf(R"(
{
    "semantic_frames": []
}
)");

constexpr TStringBuf INPUT_SELECT_VIDEO = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.quasar.select_video_from_gallery_by_text",
            "slots": [
                {
                    "name": "video_text",
                    "type": "string",
                    "value": "имя фильма"
                }
            ]
        }
    ]
}
)");

constexpr TStringBuf INPUT_SELECT_CHANNEL = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.quasar.select_channel_from_gallery_by_text",
            "slots": [
                {
                    "name": "video_text",
                    "type": "string",
                    "value": "имя канала"
                }
            ]
        }
    ]
}
)");

constexpr TStringBuf INPUT_SELECT_TV_PROGRAM = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.quasar.select_channel_from_gallery_by_text",
            "slots": [
                {
                    "name": "video_text",
                    "type": "string",
                    "value": "Доброе утро"
                }
            ]
        }
    ]
}
)");

constexpr TStringBuf INPUT_SELECT_CHANNEL_1 = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.quasar.select_channel_from_gallery_by_text",
            "slots": [
                {
                    "name": "video_text",
                    "type": "string",
                    "value": "Волгоград 1"
                }
            ]
        }
    ]
}
)");

constexpr TStringBuf INPUT_SHOW_VIDEO_SETTINGS = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.video_command.show_video_settings"
        }
    ]
}
)");

constexpr TStringBuf INPUT_SKIP_VIDEO_FRAGMENT = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.video_command.skip_video_fragment"
        }
    ]
}
)");

constexpr TStringBuf INPUT_CHANGE_TRACK = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.video_command.change_track"
        }
    ]
}
)");

constexpr TStringBuf INPUT_CHANGE_TRACK_HARDCODED = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.video_command.change_track_hardcoded"
        }
    ]
}
)");

constexpr TStringBuf INPUT_VIDEO_HOW_LONG = TStringBuf(R"(
{
    "semantic_frames": [
        {
            "name": "personal_assistant.scenarios.video_command.video_how_long"
        }
    ]
}
)");

constexpr TStringBuf BEGEMOT_ITEM_SELECTION_RESULT = TStringBuf(R"(
{
    "begemot_item_selector_result": {
        "galleries": [{
            "gallery_name": "video_gallery",
            "items": [{
                 "score": 0.9860969048,
                 "is_selected": true,
                 "alias": "2"
            }]
        }]
    }
}
)");

const TMap<int, TStringBuf> ITEM_SELECTION_DATA_MAP = {{11, BEGEMOT_ITEM_SELECTION_RESULT}};

Y_UNIT_TEST_SUITE(VideoIntentsClassifier) {
    Y_UNIT_TEST(Main) {
        TestCase(BASE_MAIN_SCREEN, INPUT_GOTO_VIDEO_SCREEN, QUASAR_GOTO_VIDEO_SCREEN);
        TestCase(BASE_MAIN_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_MAIN_SCREEN, INPUT_GOTO_VIDEO_SCREEN_EMPTY, NO_INTENT);
        TestCase(BASE_MAIN_SCREEN, INPUT_EMPTY, NO_INTENT);
    }
    Y_UNIT_TEST(Gallery) {
        // native Gallery sreen
        TestCase(BASE_GALLERY_SCREEN, INPUT_SELECT_VIDEO, QUASAR_SELECT_VIDEO_FROM_GALLERY);
        TestCase(BASE_GALLERY_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_GALLERY_SCREEN, INPUT_EMPTY, NO_INTENT);
    }
    Y_UNIT_TEST(WebViewVideoSearch) {
        // webview videoSearch screen
        TestCase(BASE_WEBVIEW_VIDEOSEARCH_SCREEN, INPUT_SELECT_VIDEO, ITEM_SELECTION_DATA_MAP, QUASAR_SELECT_VIDEO_FROM_GALLERY);
        TestCase(BASE_WEBVIEW_VIDEOSEARCH_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_WEBVIEW_VIDEOSEARCH_SCREEN, INPUT_EMPTY, NO_INTENT);
    }
    Y_UNIT_TEST(WebViewFilmsSearch) {
        // webview filmsSearch screen
        TestCase(BASE_WEBVIEW_FILMSSEARCH_SCREEN, INPUT_SELECT_VIDEO, ITEM_SELECTION_DATA_MAP, QUASAR_SELECT_VIDEO_FROM_GALLERY);
        TestCase(BASE_WEBVIEW_FILMSSEARCH_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_WEBVIEW_FILMSSEARCH_SCREEN, INPUT_EMPTY, NO_INTENT);
    }
    Y_UNIT_TEST(TvGallery) {
        // native channels screen
        TestCase(BASE_TV_GALLERY_SCREEN, INPUT_SELECT_CHANNEL, QUASAR_SELECT_VIDEO_FROM_GALLERY);
        TestCase(BASE_TV_GALLERY_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_TV_GALLERY_SCREEN, INPUT_EMPTY, NO_INTENT);
    }
    Y_UNIT_TEST(WebViewChannels) {
        // webview channels screen
        TestCase(BASE_WEBVIEW_CHANNELS_SCREEN, INPUT_SELECT_CHANNEL, QUASAR_SELECT_VIDEO_FROM_GALLERY);
        TestCase(BASE_WEBVIEW_CHANNELS_SCREEN, INPUT_SELECT_CHANNEL_1, QUASAR_SELECT_VIDEO_FROM_GALLERY);
        TestCase(BASE_WEBVIEW_CHANNELS_SCREEN, INPUT_SELECT_TV_PROGRAM, QUASAR_SELECT_VIDEO_FROM_GALLERY);
        TestCase(BASE_WEBVIEW_CHANNELS_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_WEBVIEW_CHANNELS_SCREEN, INPUT_EMPTY, NO_INTENT);
    }
    Y_UNIT_TEST(SeasonGallery) {
        // native season gallery screen
        TestCase(BASE_SEASON_GALLERY_SCREEN, INPUT_SELECT_VIDEO, QUASAR_SELECT_VIDEO_FROM_GALLERY);
        TestCase(BASE_SEASON_GALLERY_SCREEN, INPUT_OPEN_CURRENT_VIDEO, QUASAR_OPEN_CURRENT_VIDEO);
        TestCase(BASE_SEASON_GALLERY_SCREEN, INPUT_PAYMENT_CONFIRMED, QUASAR_PAYMENT_CONFIRMED);
        TestCase(BASE_SEASON_GALLERY_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_SEASON_GALLERY_SCREEN, INPUT_EMPTY, NO_INTENT);
    }
    Y_UNIT_TEST(WebViewVideoEntitySeasons) {
        // webview season gallery screen
        TestCase(BASE_SEASON_GALLERY_SCREEN, INPUT_SELECT_VIDEO, QUASAR_SELECT_VIDEO_FROM_GALLERY);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_SEASONS_SCREEN, INPUT_OPEN_CURRENT_VIDEO, QUASAR_OPEN_CURRENT_VIDEO);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_SEASONS_SCREEN, INPUT_PAYMENT_CONFIRMED, QUASAR_PAYMENT_CONFIRMED);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_SEASONS_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_SEASONS_SCREEN, INPUT_EMPTY, NO_INTENT);
    }
    Y_UNIT_TEST(Description) {
        // native Description screen
        TestCase(BASE_DESCRIPTION_SCREEN, INPUT_OPEN_CURRENT_VIDEO, QUASAR_OPEN_CURRENT_VIDEO);
        TestCase(BASE_DESCRIPTION_SCREEN, INPUT_PAYMENT_CONFIRMED, QUASAR_PAYMENT_CONFIRMED);
        TestCase(BASE_DESCRIPTION_SCREEN, INPUT_AUTHORIZE_PROVIDER, QUASAR_AUTHORIZE_PROVIDER_CONFIRMED);
        TestCase(BASE_DESCRIPTION_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_DESCRIPTION_SCREEN, INPUT_EMPTY, NO_INTENT);
    }
    Y_UNIT_TEST(WebViewVideoEntity) {
        // webview videoEntity screen
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_SCREEN, INPUT_OPEN_CURRENT_VIDEO, QUASAR_OPEN_CURRENT_VIDEO);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_SCREEN, INPUT_PAYMENT_CONFIRMED, QUASAR_PAYMENT_CONFIRMED);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_SCREEN, INPUT_AUTHORIZE_PROVIDER, QUASAR_AUTHORIZE_PROVIDER_CONFIRMED);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_SCREEN, INPUT_EMPTY, NO_INTENT);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_SCREEN, INPUT_OPEN_CURRENT_TRAILER, QUASAR_OPEN_CURRENT_TRAILER);
    }
    Y_UNIT_TEST(WebViewVideoEntityDescription) {
        // webview videoEntity/Description screen
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_DESCRIPTION_SCREEN, INPUT_OPEN_CURRENT_VIDEO, QUASAR_OPEN_CURRENT_VIDEO);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_DESCRIPTION_SCREEN, INPUT_PAYMENT_CONFIRMED, QUASAR_PAYMENT_CONFIRMED);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_DESCRIPTION_SCREEN, INPUT_AUTHORIZE_PROVIDER, QUASAR_AUTHORIZE_PROVIDER_CONFIRMED);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_DESCRIPTION_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_DESCRIPTION_SCREEN, INPUT_EMPTY, NO_INTENT);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_DESCRIPTION_SCREEN, INPUT_OPEN_CURRENT_TRAILER, QUASAR_OPEN_CURRENT_TRAILER);
    }
    Y_UNIT_TEST(WebViewVideoEntityCarousel) {
        // webview videoEntity/Carousel screen
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_CAROUSEL_SCREEN, INPUT_OPEN_CURRENT_VIDEO, QUASAR_OPEN_CURRENT_VIDEO);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_CAROUSEL_SCREEN, INPUT_PAYMENT_CONFIRMED, QUASAR_PAYMENT_CONFIRMED);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_CAROUSEL_SCREEN, INPUT_AUTHORIZE_PROVIDER, QUASAR_AUTHORIZE_PROVIDER_CONFIRMED);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_CAROUSEL_SCREEN, INPUT_SELECT_VIDEO, ITEM_SELECTION_DATA_MAP, QUASAR_SELECT_VIDEO_FROM_GALLERY);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_CAROUSEL_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_CAROUSEL_SCREEN, INPUT_EMPTY, NO_INTENT);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_CAROUSEL_SCREEN, INPUT_OPEN_CURRENT_TRAILER, QUASAR_OPEN_CURRENT_TRAILER);
    }
    Y_UNIT_TEST(WebViewVideoEntityRelatedCarousel) {
        // webview videoEntity/RelatedCarousel screen
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_RELATED_CAROUSEL_SCREEN, INPUT_SELECT_VIDEO, ITEM_SELECTION_DATA_MAP, QUASAR_SELECT_VIDEO_FROM_GALLERY);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_RELATED_CAROUSEL_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_RELATED_CAROUSEL_SCREEN, INPUT_EMPTY, NO_INTENT);
        TestCase(BASE_WEBVIEW_VIDEO_ENTITY_RELATED_CAROUSEL_SCREEN, INPUT_OPEN_CURRENT_TRAILER, QUASAR_OPEN_CURRENT_TRAILER);
    }
    Y_UNIT_TEST(PaymentAndVideoPlayer) {
        TestCase(BASE_PAYMENT_SCREEN, INPUT_OPEN_CURRENT_VIDEO, QUASAR_OPEN_CURRENT_VIDEO);
        TestCase(BASE_VIDEO_PLAYER_SCREEN, INPUT_OPEN_CURRENT_VIDEO, QUASAR_OPEN_CURRENT_VIDEO);
        TestCase(BASE_PAYMENT_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_VIDEO_PLAYER_SCREEN, INPUT_VIDEO_PLAY, SEARCH_VIDEO);
        TestCase(BASE_PAYMENT_SCREEN, INPUT_EMPTY, NO_INTENT);
        TestCase(BASE_VIDEO_PLAYER_SCREEN, INPUT_EMPTY, NO_INTENT);
        TestCase(BASE_VIDEO_PLAYER_SCREEN, INPUT_SHOW_VIDEO_SETTINGS, VIDEO_COMMAND_SHOW_VIDEO_SETTINGS);
        TestCase(BASE_VIDEO_PLAYER_SCREEN, INPUT_SKIP_VIDEO_FRAGMENT, VIDEO_COMMAND_SKIP_VIDEO_FRAGMENT);
        TestCase(BASE_VIDEO_PLAYER_SCREEN, INPUT_CHANGE_TRACK, VIDEO_COMMAND_CHANGE_TRACK);
        TestCase(BASE_VIDEO_PLAYER_SCREEN, INPUT_CHANGE_TRACK_HARDCODED, VIDEO_COMMAND_CHANGE_TRACK_HARDCODED);
        TestCase(BASE_VIDEO_PLAYER_SCREEN, INPUT_VIDEO_HOW_LONG, VIDEO_COMMAND_VIDEO_HOW_LONG);
    }
}

} // namespace
