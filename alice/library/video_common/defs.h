#pragma once

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NAlice::NVideoCommon {

inline constexpr ui64 SKIP_FRAGMENT_MAX_TIME_DELAY = 1;
inline constexpr ui64 SKIP_LAST_FRAGMENT_MAX_ERROR = 5;

inline constexpr TStringBuf SLOT_BOOL_TYPE = "bool";
inline constexpr TStringBuf SLOT_BOOL_FALSE = "false";
inline constexpr TStringBuf SLOT_BOOL_TRUE = "true";

inline constexpr TStringBuf SLOT_ABOUT = "about";
inline constexpr TStringBuf SLOT_ACTION = "action";
inline constexpr TStringBuf SLOT_BROWSER_VIDEO_GALLERY = "browser_video_gallery";
inline constexpr TStringBuf SLOT_CONTENT_TYPE = "content_type";
inline constexpr TStringBuf SLOT_CONTENT_TYPE_TEXT = "content_type_text";
inline constexpr TStringBuf SLOT_COUNTRY = "country";
inline constexpr TStringBuf SLOT_COUNTRY_TEXT = "country_text";
inline constexpr TStringBuf SLOT_EPISODE = "episode";
inline constexpr TStringBuf SLOT_FORBID_AUTOSELECT = "forbid_autoselect";
inline constexpr TStringBuf SLOT_FREE = "free";
inline constexpr TStringBuf SLOT_GENRE = "film_genre";
inline constexpr TStringBuf SLOT_GENRE_TEXT = "film_genre_text";
inline constexpr TStringBuf SLOT_NEW = "new";
inline constexpr TStringBuf SLOT_PROVIDER_OVERRIDE = "content_provider_override";
inline constexpr TStringBuf SLOT_PROVIDER = "content_provider";
inline constexpr TStringBuf SLOT_PROVIDER_TEXT = "content_provider_text";
inline constexpr TStringBuf SLOT_RELEASE_DATE = "release_date";
inline constexpr TStringBuf SLOT_RELEASE_DATE_TEXT = "release_date_text";
inline constexpr TStringBuf SLOT_SCREEN_NAME = "screen_name";
inline constexpr TStringBuf SLOT_SCREEN = "screen";
inline constexpr TStringBuf SLOT_SEARCH_TEXT = "search_text";
inline constexpr TStringBuf SLOT_SEARCH_TEXT_TEXT = "search_text_text";
inline constexpr TStringBuf SLOT_SEASON = "season";
inline constexpr TStringBuf SLOT_TOP = "top";
inline constexpr TStringBuf SLOT_VIDEO_TEXT = "video_text";
inline constexpr TStringBuf SLOT_VIDEO_INDEX = "video_index";
inline constexpr TStringBuf SLOT_VIDEO_ITEM = "video_item";
inline constexpr TStringBuf SLOT_VIDEO_NUMBER = "video_number";
inline constexpr TStringBuf SLOT_VIDEO_RESULT = "video_result";
inline constexpr TStringBuf SLOT_VIDEO_TYPE_RAW = "video_type_raw";
inline constexpr TStringBuf SLOT_VIDEO_TEXT_RAW = "video_text_raw";
inline constexpr TStringBuf SLOT_AUDIO_LANGUAGE = "audio_language";
inline constexpr TStringBuf SLOT_SUBTITLE_LANGUAGE = "subtitle_language";
inline constexpr TStringBuf SLOT_SILENT_RESPONSE = "silent_response";
inline constexpr TStringBuf SLOT_TAB_NAME = "tab_name";
inline constexpr TStringBuf SLOT_TEXT_SUFFIX = "_text";
inline constexpr TStringBuf SLOT_RAW_SUFFIX = "_raw";


inline constexpr TStringBuf SLOT_ACTION_TYPE = "video_action";
inline constexpr TStringBuf SLOT_BROWSER_VIDEO_GALLERY_TYPE = "browser_video_gallery";
inline constexpr TStringBuf SLOT_CONTENT_TYPE_TYPE = "video_content_type";
inline constexpr TStringBuf SLOT_COUNTRY_TYPE = "geo_adjective";
inline constexpr TStringBuf SLOT_EPISODE_TYPE = "video_episode";
inline constexpr TStringBuf SLOT_FORBID_AUTOSELECT_TYPE = "forbid_autoselect"; // TODO
inline constexpr TStringBuf SLOT_FREE_TYPE = "video_free";
inline constexpr TStringBuf SLOT_GENRE_TYPE = "video_film_genre";
inline constexpr TStringBuf SLOT_NEW_TYPE = "video_new";
inline constexpr TStringBuf SLOT_PROVIDER_OVERRIDE_TYPE = "video_provider_override";
inline constexpr TStringBuf SLOT_PROVIDER_TYPE = "video_provider";
inline constexpr TStringBuf SLOT_PROVIDER_STRING_TYPE = "string";
inline constexpr TStringBuf SLOT_RELEASE_DATE_TYPE = "year_adjective";
inline constexpr TStringBuf SLOT_SCREEN_NAME_TYPE = "quasar_video_screen";
inline constexpr TStringBuf SLOT_SCREEN_TYPE = "quasar_video_screen";
inline constexpr TStringBuf SLOT_SEASON_TYPE = "video_season";
inline constexpr TStringBuf SLOT_TOP_TYPE = "video_top";
inline constexpr TStringBuf SLOT_VIDEO_INDEX_TYPE = "video_index"; //TODO
inline constexpr TStringBuf SLOT_VIDEO_RESULT_TYPE = "video_result";
inline constexpr TStringBuf SLOT_SELECTION_ACTION_TYPE = "video_selection_action";
inline constexpr TStringBuf SLOT_VIDEO_LANGUAGE_TYPE = "video_language";
inline constexpr TStringBuf SLOT_VIDEO_GALLERY_TYPE = "device.video_gallery";

inline constexpr TStringBuf SLOT_CUSTOM_SCREEN_TYPE = "custom.quasar_video_screen";
inline constexpr TStringBuf SLOT_CUSTOM_SELECTION_ACTION_TYPE = "custom.video_selection_action";
inline constexpr TStringBuf SLOT_CUSTOM_SEASON_TYPE = "custom.video_season";
inline constexpr TStringBuf SLOT_CUSTOM_EPISODE_TYPE = "custom.video_episode";

inline constexpr TStringBuf SLOT_CALCULATE_VIDEO_FACTORS_ON_BASS = "calculate_video_factors_on_bass";
inline constexpr TStringBuf SLOT_CALCULATE_VIDEO_FACTORS_ON_BASS_TYPE = SLOT_BOOL_TYPE;
inline constexpr TStringBuf SLOT_CALCULATE_VIDEO_FACTORS_ON_BASS_VALUE = SLOT_BOOL_TRUE;


inline constexpr TStringBuf WIZARD_QUASAR_VIDEO_PLAY_NEXT_VIDEO = "personal_assistant.scenarios.player.next_track";
inline constexpr TStringBuf WIZARD_QUASAR_VIDEO_PLAY_PREV_VIDEO = "personal_assistant.scenarios.player.previous_track";

inline constexpr TStringBuf SEARCH_VIDEO = "personal_assistant.scenarios.video_play";
inline constexpr TStringBuf SEARCH_VIDEO_ENTITY = "personal_assistant.scenarios.video_play_entity";
inline constexpr TStringBuf SEARCH_VIDEO_FREE = "personal_assistant.scenarios.video_play_free";
inline constexpr TStringBuf SEARCH_VIDEO_PAID = "personal_assistant.scenarios.video_play_paid";
inline constexpr TStringBuf QUASAR_VIDEO_PLAY_NEXT_VIDEO = "personal_assistant.scenarios.player_next_track";
inline constexpr TStringBuf QUASAR_VIDEO_PLAY_PREV_VIDEO = "personal_assistant.scenarios.player_previous_track";
inline constexpr TStringBuf SEARCH_VIDEO_TEXT = "personal_assistant.scenarios.video_play_text";
inline constexpr TStringBuf SEARCH_VIDEO_RAW = "alice.quasar.video_play_text";
inline constexpr TStringBuf VIDEO_RECOMMENDATION = "personal_assistant.scenarios.video_recommendation";
inline constexpr TStringBuf QUASAR_SELECT_VIDEO_FROM_GALLERY = "personal_assistant.scenarios.quasar.select_video_from_gallery";
inline constexpr TStringBuf QUASAR_SELECT_VIDEO_FROM_GALLERY_CALLBACK = "personal_assistant.scenarios.quasar.select_video_from_gallery__callback";
inline constexpr TStringBuf QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_REMOTE_CONTROL = "personal_assistant.scenarios.quasar.select_video_from_gallery_by_remote_control";
inline constexpr TStringBuf QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT = "personal_assistant.scenarios.quasar.select_video_from_gallery_by_text";
inline constexpr TStringBuf QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_NUMBER = "personal_assistant.scenarios.select_video_by_number";
inline constexpr TStringBuf QUASAR_SELECT_CHANNEL_FROM_GALLERY_BY_TEXT = "personal_assistant.scenarios.quasar.select_channel_from_gallery_by_text";
inline constexpr TStringBuf QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT_TEXT = "personal_assistant.scenarios.quasar.select_video_from_gallery_by_text_text";
inline constexpr TStringBuf QUASAR_SELECT_CHANNEL_FROM_GALLERY_BY_TEXT_TEXT = "personal_assistant.scenarios.quasar.select_channel_from_gallery_by_text_text";
inline constexpr TStringBuf QUASAR_GOTO_VIDEO_SCREEN = "personal_assistant.scenarios.quasar.goto_video_screen";
inline constexpr TStringBuf QUASAR_PAYMENT_CONFIRMED = "personal_assistant.scenarios.quasar.payment_confirmed";
inline constexpr TStringBuf QUASAR_PAYMENT_CONFIRMED_CALLBACK = "personal_assistant.scenarios.quasar.payment_confirmed__callback";
inline constexpr TStringBuf QUASAR_AUTHORIZE_PROVIDER_CONFIRMED = "personal_assistant.scenarios.quasar.authorize_video_provider";
inline constexpr TStringBuf QUASAR_OPEN_CURRENT_VIDEO = "personal_assistant.scenarios.quasar.open_current_video";
inline constexpr TStringBuf QUASAR_OPEN_CURRENT_VIDEO_CALLBACK = "personal_assistant.scenarios.quasar.open_current_video__callback";
inline constexpr TStringBuf VIDEO_GALLERY_SELECT = "alice.tv.gallery_video_select";
inline constexpr TStringBuf VIDEO_COMMAND_CHANGE_TRACK = "personal_assistant.scenarios.video_command.change_track";
inline constexpr TStringBuf VIDEO_COMMAND_CHANGE_TRACK_HARDCODED = "personal_assistant.scenarios.video_command.change_track_hardcoded";
inline constexpr TStringBuf QUASAR_OPEN_CURRENT_TRAILER = "alice.video.open_current_trailer";
inline constexpr TStringBuf QUASAR_VIDEO_PLAYER_FINISHED = "alice.quasar.video_player.finished";
inline constexpr TStringBuf VIDEO_COMMAND_SHOW_VIDEO_SETTINGS = "personal_assistant.scenarios.video_command.show_video_settings";
inline constexpr TStringBuf VIDEO_COMMAND_SKIP_VIDEO_FRAGMENT = "personal_assistant.scenarios.video_command.skip_video_fragment";
inline constexpr TStringBuf VIDEO_COMMAND_VIDEO_HOW_LONG = "personal_assistant.scenarios.video_command.video_how_long";
inline constexpr TStringBuf VIDEO_GET_GALLERIES = "alice.video.get_galleries";
inline constexpr TStringBuf VIDEO_GET_GALLERY = "alice.video.get_gallery";
inline constexpr TStringBuf TV_GET_SEARCH_RESULT = "alice.tv.get_search_result";
inline constexpr TStringBuf NO_INTENT = "no_intent";

inline constexpr TStringBuf FLAG_ENABLE_YOUTUBE_USER_TOKEN = "enable_youtube_user_token";
inline constexpr TStringBuf FLAG_VIDEO_BILLING_IN_APPLY_ONLY = "video_billing_in_apply_only";
inline constexpr TStringBuf FLAG_VIDEO_DEBUG_INFO = "video_debug_info";
inline constexpr TStringBuf FLAG_VIDEO_DISABLE_CONTENT_BANS = "video_disable_content_bans";
inline constexpr TStringBuf FLAG_VIDEO_DISABLE_ENTITY_SEARCH = "video_disable_entity_search";
inline constexpr TStringBuf FLAG_VIDEO_DISABLE_FALLBACK_ON_EMPTY_PROVIDER_RESULT = "video_disable_fallback_on_empty_provider_result";
inline constexpr TStringBuf FLAG_VIDEO_DISABLE_FALLBACK_ON_NO_GOOD_PROVIDER_RESULT = "video_disable_fallback_on_no_good_provider_result";
inline constexpr TStringBuf FLAG_VIDEO_DISABLE_FORCE_UNFILTERED = "video_disable_force_unfiltered";
inline constexpr TStringBuf FLAG_VIDEO_DISABLE_GALLERY_MINIMIZING = "video_disable_gallery_minimizing";
inline constexpr TStringBuf FLAG_VIDEO_DISABLE_TVSHOW_AVAILABILITY_INFO = "video_disable_tvshow_availability_info";
inline constexpr TStringBuf FLAG_VIDEO_DONT_USE_CONTENT_DB = "video_dont_use_content_db";
inline constexpr TStringBuf FLAG_VIDEO_DONT_USE_CONTENT_DB_MULTIREQUEST = "video_dont_use_content_db_multirequest";
inline constexpr TStringBuf FLAG_VIDEO_ENABLE_ALL_HOSTS = "video_unban_all_players";
inline constexpr TStringBuf FLAG_VIDEO_ENABLE_GALLERY_CUT = "video_enable_gallery_cut";
inline constexpr TStringBuf FLAG_VIDEO_ENABLE_RECOMMENDATIONS_AND_GENRES = "video_enable_recommendations_and_genres";
inline constexpr TStringBuf FLAG_VIDEO_ENABLE_SHOWING_VIDEOS_COMING_SOON = "video_enable_showing_videos_coming_soon";
inline constexpr TStringBuf FLAG_VIDEO_ENABLE_TELEMETRY = "video_enable_telemetry";
inline constexpr TStringBuf FLAG_VIDEO_ENABLE_VH_SEARCH = "video_enable_vh_search";
inline constexpr TStringBuf FLAG_VIDEO_ENABLE_YOUTUBE_CONTENT_INFO = "video_enable_youtube_content_info";
inline constexpr TStringBuf FLAG_VIDEO_ENABLED_FINISHED_BACKWARD = "enabled_video_finished_backward";
inline constexpr TStringBuf FLAG_VIDEO_FORBID_INTERNET_PROVIDERS_FOR_CHILDREN = "forbid_internet_providers_for_children";
inline constexpr TStringBuf FLAG_VIDEO_DISABLE_YAVIDEO_RECOMMENDATIONS = "video_disable_yavideo_recommendations";
inline constexpr TStringBuf FLAG_VIDEO_DONT_USE_CONTENT_TYPE_FOR_PROVIDER_SEARCH = "video_dont_use_content_type_for_provider_search";
inline constexpr TStringBuf FLAG_VIDEO_NOT_ALWAYS_RECOMMEND_KP = "video_not_always_recommend_kp";
inline constexpr TStringBuf FLAG_VIDEO_DONT_USE_TEXT_ONLY_SLOTS_IN_SEARCH_REQUESTS = "video_dont_use_text_only_slots_in_search_requests";
inline constexpr TStringBuf FLAG_VIDEO_USE_RAW_SEARCH_TEXT_FRAME = "video_use_raw_search_text_frame";
inline constexpr TStringBuf FLAG_USE_NEW_SERVICE_FOR_MODULE = "use_new_service_for_module";

inline constexpr TStringBuf FLAG_VIDEO_FORCE_PROVIDER_AMEDIATEKA = "video_force_provider_amediateka";
inline constexpr TStringBuf FLAG_VIDEO_FORCE_PROVIDER_IVI = "video_force_provider_ivi";
inline constexpr TStringBuf FLAG_VIDEO_FORCE_PROVIDER_KINOPOISK = "video_force_provider_kinopoisk";
inline constexpr TStringBuf FLAG_VIDEO_FORCE_PROVIDER_YAVIDEO = "video_force_provider_yavideo";
inline constexpr TStringBuf FLAG_VIDEO_FORCE_PROVIDER_YOUTUBE = "video_force_provider_youtube";

inline constexpr TStringBuf FLAG_VIDEO_KINOPOISK_RECOMMENDATIONS_FIXED = "video_kinopoisk_recommendations_fixed";
inline constexpr TStringBuf FLAG_VIDEO_USE_NATIVE_YOUTUBE_API = "video_use_native_youtube_api";
inline constexpr TStringBuf FLAG_VIDEO_REARR_ADD_DEVICE_ZERO = "video_rearr_add_device_zero";
inline constexpr TStringBuf FLAG_VIDEO_UNBAN_AMEDIATEKA = "video_unban_amediateka";
inline constexpr TStringBuf FLAG_VIDEO_UNBAN_IVI = "video_unban_ivi";
inline constexpr TStringBuf FLAG_VIDEO_DISABLE_KNOSS_BALANCER = "video_disable_knoss_balancer";
inline constexpr TStringBuf FLAG_VIDEO_YABRO_URL_BY_SEARCH = "video_yabro_url_by_search";
inline constexpr TStringBuf FLAG_VIDEO_YOUTUBE_CONTENT_INFO_FROM_YAVIDEO = "video_youtube_content_info_from_yavideo";
inline constexpr TStringBuf FLAG_VIDEO_DISABLE_DOC2DOC = "video_disable_doc2doc";
inline constexpr TStringBuf FLAG_VIDEO_ENABLE_WEB_REQUEST = "video_enable_web_request";
inline constexpr TStringBuf FLAG_VIDEO_ENABLE_KINOPOISK_WEBSEARCH = "video_enable_kinopoisk_websearch";
inline constexpr TStringBuf FLAG_VIDEO_ADD_MAIN_OBJECT_FROM_YAVIDEO = "video_add_main_object_from_yavideo";
inline constexpr TStringBuf FLAG_VIDEO_USE_TYPE_IN_CONTENT_DB = "video_use_type_in_content_db";
inline constexpr TStringBuf FLAG_VIDEO_DO_NOT_USE_CONTENT_DB_SEASON_AND_EPISODE = "video_do_not_use_content_db_season_and_episode";
inline constexpr TStringBuf FLAG_VIDEO_DISABLE_VINS_CONTINUE_FOR_VIDEO_SCREENS = "video_disable_vins_continue_for_video_screens";
inline constexpr TStringBuf FLAG_VIDEO_SETUP_TV_ACTIONS = "video_setup_tv_actions";

inline constexpr TStringBuf FLAG_VIDEO_USE_OLD_BILLING = "video_use_old_billing";
inline constexpr TStringBuf FLAG_VIDEO_USE_OLD_BUY_PUSH_SCREEN = "video_use_old_buy_push_screen";

inline constexpr TStringBuf FLAG_VIDEO_CHECK_TOP_ORGANIC_RESULTS_FOR_KP = "checkTopOrganicResultsForKp";
inline constexpr TStringBuf FLAG_VIDEO_CHECK_SCALED_TOP_ORGANIC_RESULTS_FOR_KP = "checkScaledTopOrganicResultsForKp";
inline constexpr TStringBuf FLAG_VIDEO_CHECK_REDUCED_TOP_ORGANIC_RESULTS_FOR_KP = "checkReducedTopOrganicResultsForKp";
inline constexpr TStringBuf FLAG_VIDEO_CHECK_CHAINED_TOP_ORGANIC_RESULTS_FOR_KP = "checkChainedTopOrganicResultsForKp";

inline constexpr TStringBuf FLAG_ANALYTICS_VIDEO_WEB_RESPONSES = "analytics.video.add_web_responses";

inline constexpr TStringBuf FLAG_PREFIX_ITEM_SELECTOR_THRESHOLD = "item_selector_threshold=";

inline constexpr TStringBuf FLAG_VIDEO_DISABLE_IRREL_IF_DEVICE_STATE_MALFORMED = "video_disable_irrel_if_device_state_malformed";

// WebView experiment flags
// disable-flags
inline constexpr TStringBuf FLAG_DISABLE_FILMS_WEBVIEW_SEARCHSCREEN = "video_disable_films_webview_searchscreen";
inline constexpr TStringBuf FLAG_DISABLE_VIDEO_WEBVIEW_SEARCHSCREEN = "video_disable_webview_searchscreen";
inline constexpr TStringBuf FLAG_DISABLE_VIDEO_WEBVIEW_USE_ONTOIDS = "video_disable_webview_use_ontoids";
inline constexpr TStringBuf FLAG_DISABLE_VIDEO_WEBVIEW_VIDEO_ENTITY = "video_disable_webview_video_entity";
inline constexpr TStringBuf FLAG_DISABLE_VIDEO_WEBVIEW_VIDEO_ENTITY_SEASONS = "video_disable_webview_video_entity_seasons";
inline constexpr TStringBuf FLAG_DISABLE_MORDOVIA_TABS = "video_disable_mordovia_tabs";
inline constexpr TStringBuf FLAG_DISABLE_VIDEO_MORDOVIA_SPA = "video_disable_mordovia_spa";
inline constexpr TStringBuf FLAG_DISABLE_VIDEO_MORDOVIA_COMMAND_NAVIGATION = "video_disable_mordovia_command_navigation";
inline constexpr TStringBuf FLAG_DISABLE_MORDOVIA_GO_HOME = "video_disable_mordovia_go_home";

// enable-flags
inline constexpr TStringBuf FLAG_VIDEO_WEBVIEW_DO_NOT_ADD_UUIDS = "video_webview_do_not_add_uuids";
inline constexpr TStringBuf FLAG_TV_CHANNELS_WEBVIEW = "tv_channels_webview";
inline constexpr TStringBuf FLAG_VIDEO_ENABLE_PROMO_NY = "video_enable_promo_ny";
inline constexpr TStringBuf FLAG_VIDEO_REMOVE_UNPUBLISHED_TRAILERS_5XX = "video_remove_unpublished_trailers_5xx";
inline constexpr TStringBuf FLAG_VIDEO_TRAILER_START_FROM_THE_BEGINNING = "video_trailer_start_from_the_beginning";

// enable-flags for 3-rd party prices
inline constexpr TStringBuf FLAG_VIDEO_FILM_OFFERS_PRICES_MIN = "video_film_offers_prices_min";
inline constexpr TStringBuf FLAG_VIDEO_FILM_OFFERS_PRICES_ALL = "video_film_offers_prices_all";

// push extra reqid in video search call + more logging
inline constexpr TStringBuf FLAG_VIDEO_SEARCH_DEBUG_GIZMOS = "video_search_debug_gizmos";

// Videoscenario in HW
inline constexpr TStringBuf FLAG_VIDEO_USE_BASS_ONLY = "video_use_bass_only";
inline constexpr TStringBuf FLAG_VIDEO_USE_PURE_PLUGS = "video_use_pure_plugs";
inline constexpr TStringBuf FLAG_VIDEO_DUMP_VIDEOSEARCH_RESULT = "video_dump_search";
inline constexpr TStringBuf FLAG_VIDEO_CENTAUR_VIDEOSEARCH_RESULT = "centaur_video_search";

inline constexpr TStringBuf FLAG_VIDEO_HALFPIRATE_FROM_BASEINFO = "video_half_pirate_from_base_info";

inline constexpr TStringBuf FLAG_GALLERY_VIDEO_SELECT = "gallery_video_select";

// Web search in videoscenario
inline constexpr TStringBuf FLAG_VIDEO_USE_WEB_SEARCH_ES = "video_use_web_search";
inline constexpr TStringBuf FLAG_VIDEO_USE_WEB_SEARCH_ALL = "video_use_web_search_all";
inline constexpr TStringBuf FLAG_VIDEO_DUMP_WEB_SEARCH = "video_dump_web_search";

// other defs
inline constexpr TStringBuf PROVIDER_AMEDIATEKA = "amediateka";
inline constexpr TStringBuf PROVIDER_IVI = "ivi";
inline constexpr TStringBuf PROVIDER_KINOPOISK = "kinopoisk";
inline constexpr TStringBuf PROVIDER_OKKO = "okko";
inline constexpr TStringBuf PROVIDER_STRM = "strm";
inline constexpr TStringBuf PROVIDER_YAVIDEO = "yavideo";
inline constexpr TStringBuf PROVIDER_YAVIDEO_PROXY = "yavideo_proxy";
inline constexpr TStringBuf PROVIDER_YOUTUBE = "youtube";

inline constexpr TStringBuf ATTENTION_ALL_RESULTS_FILTERED = "all_results_filtered";
inline constexpr TStringBuf ATTENTION_AUTOPLAY = "video_autoplay";
inline constexpr TStringBuf ATTENTION_AUTOSELECT = "video_autoselect";
inline constexpr TStringBuf ATTENTION_DETAILED_DESCRIPTION = "video_detailed_description";
inline constexpr TStringBuf ATTENTION_DISABLED_PROVIDER = "video_disabled_provider";
inline constexpr TStringBuf ATTENTION_EMPTY_SEARCH_GALLERY = "empty_search_gallery";
inline constexpr TStringBuf ATTENTION_NON_AUTHORIZED_USER = "video_non_authorized_user";
inline constexpr TStringBuf ATTENTION_NO_GOOD_RESULT = "video_no_good_result";
inline constexpr TStringBuf ATTENTION_NO_SUCH_EPISODE = "no_such_episode";
inline constexpr TStringBuf ATTENTION_NO_SUCH_SEASON = "no_such_season";
inline constexpr TStringBuf ATTENTION_PAID_CONTENT = "video_cannot_autoplay_because_its_paid";
inline constexpr TStringBuf ATTENTION_RECOMMENDATION_CANNOT_BE_NARROWED = "recommendation_cannot_be_narrowed";
inline constexpr TStringBuf ATTENTION_SHOW_TV_GALLERY = "show_tv_gallery";
inline constexpr TStringBuf ATTENTION_SHOW_PROMO_WEBVIEW = "show_promo_webview";

inline constexpr TStringBuf VIDEO_FACTORS_BLOCK_TYPE = "video_factors";
inline constexpr TStringBuf VIDEO_FACTORS_DESCRIPTION_SIMILARITY = "description_similarity";
inline constexpr TStringBuf VIDEO_FACTORS_NAME_SIMILARITY = "name_similarity";

inline constexpr TStringBuf COMMAND_TTS_PLAY_PLACEHOLDER = "tts_play_placeholder";
inline constexpr TStringBuf COMMAND_OPEN_URI = "open_uri";
inline constexpr TStringBuf COMMAND_SHOW_DESCRIPTION = "show_description";
inline constexpr TStringBuf COMMAND_SHOW_GALLERY = "show_gallery";
inline constexpr TStringBuf COMMAND_SHOW_PAY_PUSH_SCREEN = "show_pay_push_screen";
inline constexpr TStringBuf COMMAND_SHOW_SEASON_GALLERY = "show_season_gallery";
inline constexpr TStringBuf COMMAND_SHOW_VIDEO_SETTINGS = "show_video_settings";
inline constexpr TStringBuf COMMAND_VIDEO_PLAY = "video_play";
inline constexpr TStringBuf COMMAND_MORDOVIA_SHOW = "mordovia_show";
inline constexpr TStringBuf COMMAND_MORDOVIA_COMMAND = "mordovia_command";
inline constexpr TStringBuf COMMAND_PLAYER_REWIND = "player_rewind";
inline constexpr TStringBuf COMMAND_CHANGE_AUDIO = "change_audio";
inline constexpr TStringBuf COMMAND_CHANGE_SUBTITLES = "change_subtitles";
inline constexpr TStringBuf COMMAND_GO_BACKWARD = "go_backward";
inline constexpr TStringBuf COMMAND_TV_OPEN_SEARCH_SCREEN = "tv_open_search_screen";
inline constexpr TStringBuf COMMAND_TV_OPEN_DETAILS_SCREEN = "tv_open_details_screen";
inline constexpr TStringBuf COMMAND_TV_OPEN_PERSON_SCREEN = "tv_open_person_screen";
inline constexpr TStringBuf COMMAND_TV_OPEN_COLLECTION_SCREEN = "tv_open_collection_screen";
inline constexpr TStringBuf COMMAND_TV_OPEN_SERIES_SCREEN = "tv_open_series_screen";
inline constexpr TStringBuf COMMAND_WEB_OS_LAUNCH_APP = "web_os_launch_app";
inline constexpr TStringBuf COMMAND_WEB_OS_SHOW_GALLERY = "web_os_show_gallery";

// server directive commands
inline constexpr TStringBuf COMMAND_SEND_PUSH = "send_push";
inline constexpr TStringBuf COMMAND_PERSONAL_CARDS = "personal_cards";
inline constexpr TStringBuf COMMAND_PUSH_MESSAGE = "push_message";

inline constexpr TStringBuf DIRECTIVE_DEBUG_INFO = "debug_info";

inline constexpr TStringBuf DISABLE_PERSONAL_TV_CHANNEL = "disable_personal_tv_channel";

inline constexpr TStringBuf QUASAR_FROM_ID = "ya-station";
inline constexpr TStringBuf QUASAR_SERVICE = "ya-station";
inline constexpr TStringBuf TV_FROM_ID = "tvandroid";
inline constexpr TStringBuf TV_SERVICE = "ya-tv-android";
inline constexpr TStringBuf YAMODULE_FROM_ID = "module2";
inline constexpr TStringBuf YAMODULE_SERVICE = "ya-module";

inline constexpr TStringBuf VIDEO_SOURCE_CAROUSEL = "video_source_carousel";
inline constexpr TStringBuf VIDEO_SOURCE_ENTITY_SEARCH = "video_source_entity_search";
inline constexpr TStringBuf VIDEO_SOURCE_KINOPOISK_RECOMMENDATIONS = "video_source_kp_recommendations";
inline constexpr TStringBuf VIDEO_SOURCE_WEB = "video_source_web_search";
inline constexpr TStringBuf VIDEO_SOURCE_YAVIDEO_TOUCH = "video_source_yavide_touch";
inline constexpr TStringBuf VIDEO_SOURCE_YAVIDEO = "video_source_yavideo";

inline constexpr TStringBuf VIDEO_ENTLIST_POSTFIX = "enum";

inline constexpr TStringBuf FLAG_VIDEO_DISREGARD_UAAS = "disregard_uaas";
inline constexpr TStringBuf FLAG_VIDEO_FIX_TESTIDS = "video_testids";

inline constexpr TStringBuf FLAG_VIDEO_DISABLE_OAUTH = "video_disable_oauth";

enum class EProviderOverrideType {
    PaidOnly /* "paid_only" */,
    FreeOnly /* "free_only" */,
    Entity   /* "entity" */
};

enum class EScreenId {
    Gallery /* "gallery" */,
    SeasonGallery /* "season_gallery" */,
    TvGallery /* "tv_gallery" */,
    Description /* "description" */,
    Payment /* "payment" */,
    RadioPlayer /* "radio_player" */,
    VideoPlayer /* "video_player" */,
    MusicPlayer /* "music_player" */,
    Bluetooth /* "bluetooth" */,
    WebViewVideoEntity /* "videoEntity" */,
    WebviewVideoEntityWithCarousel /* "videoEntity/Carousel" */,
    WebviewVideoEntityDescription /* "videoEntity/Description" */,
    WebviewVideoEntityRelated /* "videoEntity/RelatedCarousel" */,
    WebviewVideoEntitySeasons /* "videoEntity/Seasons" */,
    WebViewVideoSearchGallery /* "videoSearch" */,
    WebViewFilmsSearchGallery /* "filmsSearch" */,
    WebViewChannels /* "channels" */,
    MordoviaMain /* "mordovia_webview" */,
    TvExpandedCollection /* "tv_expanded_collection" */,
    SearchResults /* "search_results" */,
    ContentDetails /* "content_details" */,
    Main /* "main" */,
    TvMain /* "tv_main" */
};

enum class EScreenName {
    TopScreen /* "top_screen" */,
    NewScreen /* "new_screen" */,
    RecommendationScreen /* "recommendations_screen" */,
};

enum class ESelectionAction {
    Play /* "play" */,
    Description /* "description" */,
    ListSeasons /* "list_seasons" */,
    ListEpisodes /* "list_episodes" */,
};

enum class EVideoAction {
    Play /* "play" */,
    Recommend /* "recommend" */,
    Find /* "find" */,
    Continue /* "continue" */,
    ListSeasons /* "list_seasons" */,
    ListEpisodes /* "list_episodes" */,
    Description /* "description" */
};

enum class EContentType {
    Null = 0 /* "" */,
    Movie = (1ULL << 0) /* "movie" */,                   // aka "film"
    TvShow = (1ULL << 1) /* "tv_show" */,                // aka "series"
    Video = (1ULL << 3) /* "video" */,                   // videos, clips, cartoons from youtube etc
    MusicVideo = (1ULL << 4) /* "music_video" */,        // concert, music clip, etc
    Cartoon = (1ULL << 5) /* "cartoon" */,               // cartoon
    TvStream = (1ULL << 6) /* "tv_stream" */,            // tv stream
    TvShowEpisode = (1ULL << 7) /* "tv_show_episode" */, // series episode
};

enum class EVideoGenre {
    Action /* "action" */,
    Adventure /* "adventure" */,
    Adult /* "adult" */,
    Anime /* "anime" */,
    Arthouse /* "arthouse" */,
    Biopic /* "biopic" */,
    ByComics /* "by_comics" */,
    Childrens /* "childrens" */,
    Comedy /* "comedy" */,
    Concert /* "concert" */,
    Crime /* "crime" */,
    Detective /* "detective" */,
    Disaster /* "disaster" */,
    Documentary /* "documentary" */,
    Drama /* "drama" */,
    Epic /* "epic" */,
    Erotica /* "erotica" */,
    Family /* "family" */,
    Fantasy /* "fantasy" */,
    Historical /* "historical" */,
    Horror /* "horror" */,
    Melodramas /* "melodramas" */,
    Musical /* "musical" */,
    Noir /* "noir" */,
    Porno /* "porno" */,
    Romantic /* "romantic" */,
    ScienceVideo /* "science_video" */,
    ScienceFiction /* "science_fiction" */,
    Show /* "show" */,
    SportVideo /* "sport_video" */,
    Supernatural /* "supernatural" */,
    Thriller /* "thriller" */,
    War /* "war" */,
    Westerns /* "westerns" */,
    Zombie /* "zombie" */,
};

} // namespace NAlice::NVideoCommon
