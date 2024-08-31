#pragma once

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <alice/memento/proto/api.pb.h>
#include <alice/protos/data/scenario/centaur/main_screen.pb.h>
#include <google/protobuf/any.pb.h>


namespace NAlice::NHollywood::NCombinators {

using TWidgetResponses = THashMap<TString, THashMap<TString, NData::TCentaurWidgetCardData>>;

namespace NMemento = ru::yandex::alice::memento::proto;

inline const TString RUN_HANDLE_NAME = "centaur/centaur_main_screen/run";
inline const TString CONTINUE_HANDLE_NAME = "centaur/centaur_main_screen/continue";
inline const TString FINALIZE_HANDLE_NAME = "centaur/centaur_main_screen/finalize";

inline const TString CENTAUR_COMBINATOR_RUN_HANDLE_NAME = "centaur/combinator/run";
inline const TString CENTAUR_COMBINATOR_CONTINUE_HANDLE_NAME = "centaur/combinator/continue";
inline const TString CENTAUR_COMBINATOR_FINALIZE_HANDLE_NAME = "centaur/combinator/finalize";

const TString CENTAUR_MAIN_SCREEN_COMBINATOR_PSN = "CentaurMainScreen";

const TString MUSIC_SCENARIO_NAME = "HollywoodMusic";
const TVector<TString> REQUIRED_SCENARIOS = {
    MUSIC_SCENARIO_NAME
};

const TString WEATHER_SCENARIO_NAME = "Weather";
const TString PHOTO_FRAME_SCENARIO_NAME = "PhotoFrame";
const TString NEWS_SCENARIO_NAME = "News";
const TString AFISHA_SCENARIO_NAME = "Afisha";
const TString DIALOGOVO_SCENARIO_NAME = "Dialogovo";

const TString DEFAULT_CHROME_LAYER_RENDER_KEY = "chrome.layer.type.default";
const TString CAROUSEL_ID_SLOT_NAME = "carousel_id";
const TString IS_SCHEDULED_UPDATE_SLOT_NAME = "is_scheduled_update";
const TString SKILLS_TEASER_EXP_FLAG_NAME = "collect_teasers_skills";
const TString TEASER_SETTINGS_EXP_FLAG_NAME = "teaser_settings";
const int CAROUSEL_SHOW_TIME_SEC = 300;
const TVector<TString> TEASER_SEQUENCE = {
    PHOTO_FRAME_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    AFISHA_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    NEWS_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    AFISHA_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    NEWS_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    AFISHA_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    NEWS_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    AFISHA_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    NEWS_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    AFISHA_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    NEWS_SCENARIO_NAME,
    WEATHER_SCENARIO_NAME
};
const TVector<TString> TEASER_SEQUENCE_WITH_SKILLS = {
    PHOTO_FRAME_SCENARIO_NAME,
    AFISHA_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    DIALOGOVO_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    NEWS_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    AFISHA_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    DIALOGOVO_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    NEWS_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    AFISHA_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    DIALOGOVO_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    NEWS_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    AFISHA_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    DIALOGOVO_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    NEWS_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    AFISHA_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    DIALOGOVO_SCENARIO_NAME,
    PHOTO_FRAME_SCENARIO_NAME,
    NEWS_SCENARIO_NAME,
    WEATHER_SCENARIO_NAME
};
const TString CAROUSEL_SERVER_UPDATE_EXP_FLAG_NAME = "centaur_carousel_server_updates";
const TString CENTAUR_COLLECT_MAIN_SCREEN_FRAME = "alice.centaur.collect_main_screen";
const TString CENTAUR_ADD_WIDGET_FROM_GALLERY_FRAME = "alice.centaur.add_widget_from_gallery";
const TString COLLECT_CARDS_FRAME_NAME = "alice.centaur.collect_cards";
const TString SET_TEASER_CONFIGURATION_FRAME_NAME = "alice.centaur.set_teaser_configuration";
const TString COLLECT_TEASERS_PREVIEW_FRAME_NAME = "alice.centaur.collect_teasers_preview";
const TString CENTAUR_TEASERS_COMBINATOR_PSN = "CentaurTeasersCombinator";
const TString SCENARIOS_FOR_TEASERS_DATA_SLOT = "scenarios_for_teasers_slot";

struct TMusicScenarioData {
    TString Id;
    TString ImgUrl;
    TString Type;
    TString Title;
    TMaybe<TString> Modified = Nothing();
    TMaybe<uint> LikesCount = Nothing();
    TVector<TString> Genres = {};
    TMaybe<TString> ReleaseDate = Nothing();

    struct TArtistInfo {
        TString Id;
        TString Name;
    };
    TVector<TArtistInfo> Artists = {};
    TString BlockType = "unknown_type";
    TString Action = {};
    // should be TTypedSemanticframe
    TMaybe<google::protobuf::Any> TypedAction = Nothing();
};
struct TMusicScenarioDataBlock {
    TString Type;
    TString Title;
    TVector<TMusicScenarioData> MusicScenarioData = {};
};
struct TMusicData {
    TVector<TMusicScenarioDataBlock> musicDataBlocks = {};
};
struct TVideoScenarioData {
    TString Id;
    TString ImgUrl;
    TString Action;
};
const TVector<TVideoScenarioData> VIDEO_CARDS = {
    {
        "kinopoisk_1",
        "https://avatars.mds.yandex.net/get-ott/2385704/2a000001752c779eee1032f5da3c164d1e91/640x360",
        "dialog://text_command?query=включи сериал новый папа"
    },
    {
        "kinopoisk_2",
        "https://avatars.mds.yandex.net/get-ott/223007/2a0000016ff0f489d6c4d22aec03d3b752c3/640x360",
        "dialog://text_command?query=включи сериал наследники"
    },
    {
        "kinopoisk_3",
        "https://avatars.mds.yandex.net/get-ott/2419418/2a0000017183dbe82bfb88cec0cf8e050ac4/640x360",
        "dialog://text_command?query=включи сериал селиконовая долина"
    },
    {
        "kinopoisk_4",
        "https://avatars.mds.yandex.net/get-ott/2385704/2a0000017015758bcb332eb794772915d1ba/640x360",
        "dialog://text_command?query=включи фильм боль и слава"
    },
    {
        "kinopoisk_5",
        "https://avatars.mds.yandex.net/get-ott/2439731/2a00000172550245e33b2b8dd0019e5d4185/640x360",
        "dialog://text_command?query=включи фильм чужие"
    },
    {
        "kinopoisk_6",
        "https://avatars.mds.yandex.net/get-ott/2385704/2a000001720d27be172b84628276f3c9ecc7/640x360",
        "dialog://text_command?query=включи мультфильм бесподобный мистер фокс"
    },
    {
        "kinopoisk_7",
        "https://avatars.mds.yandex.net/get-ott/2419418/2a00000173c319abf8e1d2f4ef6598b211e3/640x360",
        "dialog://text_command?query=включи фильм двухсотлетний человек"
    }
};

struct TServiceData {
    TString Id;
    TString Text;
    TString ImgUrl;
    TString WebviewUrl;
};
const TVector<TServiceData> SERVICE_CARDS = {
    {
        "youtube",
        "Youtube",
        "https://www.youtube.com/s/desktop/aa517dff/img/favicon_144x144.png",
        "https://www.youtube.com"
    },
    {
        "yandex_zoom",
        "Zoom",
        "https://yastatic.net/s3/dialogs/smart_displays/services/zoom.png",
        "https://yandex.zoom.us/"
    },
    {
        "netflix",
        "Netflix",
        "https://yastatic.net/s3/dialogs/smart_displays/services/netflix.png",
        "https://www.netflix.com/"
    },
    {
        "telegram",
        "Telegram",
        "https://yastatic.net/s3/dialogs/smart_displays/services/telegram.png",
        "https://web.telegram.org/z/"
    },
    {
        "spotify",
        "Spotify",
        "https://yastatic.net/s3/dialogs/smart_displays/services/spotify.png",
        "https://www.spotify.com"
    },
    {
        "tiktok",
        "Tiktok",
        "https://lf16-tiktok-common.ibytedtos.com/obj/tiktok-web-common-sg/mtact/static/pwa/icon_128x128.png",
        "https://www.tiktok.com"
    },
    {
        "yandex_eda",
        "Еда",
        "https://yastatic.net/s3/dialogs/smart_displays/services/eda.png",
        "https://eda.yandex.ru"
    },
    {
        "yandex_lavka",
        "Лавка",
        "https://yastatic.net/s3/dialogs/smart_displays/services/lavka.png",
        "https://lavka.yandex.ru/"
    },
    {
        "yandex_notes",
        "Заметки",
        "https://yastatic.net/s3/dialogs/smart_displays/services/notes.png",
        "https://disk.yandex.ru/notes/"
    },
    {
        "yandex_maps",
        "Карты",
        "https://yastatic.net/s3/front-maps-static/maps-front-maps//static/v17/icons/favicon/apple-touch-icon-180x180.png",
        "https://yandex.ru/maps/"
    },
    {
        "yandex_browser",
        "Браузер",
        "https://cdn.icon-icons.com/icons2/2552/PNG/128/yandex_browser_logo_icon_152939.png",
        "https://yandex.ru/"
    },
    {
        "yandex_translate",
        "Переводчик",
        "https://translate.yandex.ru/icons/favicon.png",
        "https://translate.yandex.ru/"
    },
    {
        "yandex_kinopoisk",
        "Кинопоиск",
        "https://yastatic.net/s3/dialogs/smart_displays/services/kinopoisk.png",
        "https://www.kinopoisk.ru/"
    },
    {
        "yandex_video",
        "Видео",
        "https://yastatic.net/s3/dialogs/smart_displays/services/video.png",
        "https://yandex.ru/video/"
    },
    {
        "yandex_market",
        "Маркет",
        "https://yastatic.net/s3/dialogs/smart_displays/services/market.png",
        "https://market.yandex.ru/"
    },
    {
        "yandex_disc",
        "Диск",
        "https://yastatic.net/s3/dialogs/smart_displays/services/disc.png",
        "https://disc.yandex.ru"
    },
    {
        "yandex_mail",
        "Почта",
        "https://yastatic.net/s3/dialogs/smart_displays/services/mail.png",
        "http://mail.yandex.ru"
    },
    {
        "yandex_teleprogramma",
        "Телепрограмма",
        "https://yastatic.net/s3/dialogs/smart_displays/services/tele.png",
        "https://tv.yandex.ru/"
    },
    {
        "yandex_news",
        "Новости",
        "https://yastatic.net/s3/dialogs/smart_displays/services/news.png",
        "https://yandex.ru/news/"
    },
    {
        "yandex_q",
        "Кью",
        "https://answers-static.s3.yandex.net/assets/favicons/apple-touch-icon-180x180-q.png",
        "https://yandex.ru/q/"
    },
    {
        "yandex_games",
        "Игры",
        "https://yastatic.net/s3/dialogs/smart_displays/services/games.png",
        "https://yandex.ru/games/"
    },
    {
        "yandex_zen",
        "Дзен",
        "https://yastatic.net/s3/dialogs/smart_displays/services/zen.png",
        "https://zen.yandex.ru/"
    },
    {
        "rickroll",
        "Rick Roll",
        "https://yastatic.net/s3/dialogs/smart_displays/services/rickroll.png",
        "https://www.youtube.com/watch?v=xvFZjo5PgG0"
    }
};

const TString MAIN_SCREEN_DIRECTIVE_NAME = "Мой экран";
const TString UPPER_SHUTTER_DIRECTIVE_NAME = "Верхнее меню";
const TString MY_SCREEN_TAB_ID = "tab1";
const TString MY_SCREEN_TAB_TITLE = "Мой экран";

const TString MUSIC_TAB_ID = "tab2";
const TString MUSIC_TAB_TITLE = "Музыка";
const TString MUSIC_DIV_CARD_ID = "music.gallery.tab.block";

const TString SMART_HOME_TAB_ID = "tab3";
const TString SMART_HOME_TAB_TITLE = "Умный Дом";
const TString SMART_HOME_DIV_CARD_ID = "smart.home.webview.tab";
const TString SMART_HOME_WEBVIEW_URL = "https://yandex.ru/iot";

const TString SERVICES_TAB_ID = "tab4";
const TString SERVICES_TAB_TITLE = "Сервисы";
const TString SERVICES_DIV_CARD_ID = "services.webview.tab";

const TString DISCOVERY_TAB_ID = "tab5";
const TString DISCOVERY_TAB_TITLE = "Что я умею";
const TString DISCOVERY_DIV_CARD_ID = "discovery.tab.block";

const TString MUSIC_BLOCK_TITLE = "Послушать";
const TString MUSIC_BLOCK_ID = "music_block1";
const uint MUSIC_BLOCK_WIDTH = 275;
const uint MUSIC_BLOCK_HEIGHT = 255;

const TString VIDEO_BLOCK_TITLE = "Посмотреть";
const TString VIDEO_BLOCK_ID = "kinopoisk_block1";
const uint VIDEO_BLOCK_WIDTH = 538;
const uint VIDEO_BLOCK_HEIGHT = 298;

const uint SERVICES_BLOCK_WIDTH = 214;
const uint SERVICES_BLOCK_HEIGHT = 232;

const TString UPPER_SHUTTER_DIV_CARD_ID = "upper.shutter.card.id";

const TVector<TStringBuf> MUSIC_BLOCK_TYPES_ORDER = {
    "inf_feed_play_contexts", // Вы недавно слушали
    "inf_feed_new_releases", // Новый релизы
    "inf_feed_liked_artists", // Ваши любимые исполнители
    "inf_feed_liked_podcasts", // Ваши любимые подкасты
    "inf_feed_recommended_podcasts", // Подкасты
    "inf_feed_tag_playlists-for_kids", // Для детей
    "inf_feed_user_library_playlists", // Ваши плейлисты
    "inf_feed_tag_playlists", // Популярное
    "inf_feed_popular_artists", // Популярные исполнители
    "inf_feed_auto_playlists" // Собрано для вас
};
const TString MY_SCREEN_TAB_MUSIC_BLOCK_TYPE = "inf_feed_auto_playlists";

const TString IOT_SCENARIO_NAME = "IoTScenarios";
const TString DIVVED_SMART_HOME_TAB_EXP_FLAG_NAME = "divved_smart_home_tab_exp";

const TString MAIN_SCREEN_SERVER_UPDATE_EXP_FLAG_NAME = "centaur_main_screen_server_updates";
const TString MAIN_SCREEN_SERVICES_TAB_ENABLE_EXP_FLAG_NAME = "main_screen_services_tab_enable";

const TString WIDGET_GALLERY_DIV_CARD_ID = "widget_gallery.card.id";

const int REFRESH_MAX_RETRIES = 1000;
const int REFRESH_MAX_RESTART_PERIOD_DAYS = 7;
const int REFRESH_RESTART_PERIOD_BACKOFF = 2;

const int CAROUSEL_REFRESH_PERIOD_MINUTES = 15;
const TString CAROUSEL_UPDATE_ACTION_NAME = "update_carousel";
const TString CENTAUR_TEASERS_PRODUCT_SCENARIO = "CentaurTeasersCombinator";
const TString COLLECT_CAROUSEL_PURPOSE = "collect_carousel_cards";

const int MAIN_SCREEN_REFRESH_PERIOD_MINUTES = 60;
const TString MAIN_SCREEN_UPDATE_ACTION_NAME = "update_main_screen";
const TString CENTAUR_MAIN_SCREEN_PRODUCT_SCENARIO = "CentaurMainScreen";
const TString COLLECT_MAIN_SCREEN_PURPOSE = "collect_main_screen";

const TString DELETE_WIDGET_EXP_FLAG_NAME = "delete_widget";

const TString PATCH_MAIN_SCREEN_EXP_FLAG_NAME = "patch_main_screen";
const TString MAIN_SCREEN_DIV_CARD_NAME = "MainScreen";
const TString MY_SCREEN_DIV_CARD_ID = "myscreen.card.id";

const TString TYPED_ACTION_EXP_FLAG_NAME = "centaur_typed_action";

} // namespace NAlice::NHollywood::NCombinators
