#pragma once

#include <alice/library/experiments/flags.h>

#include <util/generic/strbuf.h>

namespace NBASS {

using NAlice::NExperiments::EXP_ETHER;
using NAlice::NExperiments::EXP_FIND_POI_GALLERY;
using NAlice::NExperiments::EXP_TV_RESTREAMED_CHANNELS_LIST;
using NAlice::NExperiments::EXP_TV_SHOW_RESTREAMED_CHANNELS_IN_GALLERY;

inline constexpr TStringBuf EXPERIMENTAL_FLAG_ALARMS_KEEP_OBSOLETE = "alarm_keep_obsolete";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_ALICE_PODCAST = "alice_podcast";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_BIO_LIMIT_ENROLLED_USERS = "quasar_biometry_limit_users";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_CHILDREN_SHOW_CONFIG_ID_PREFIX = "hw_alice_children_show_config_id=";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_CLOSE_EXTERNAL_SKILL_ON_DEACTIVATE = "close_external_skill_on_deactivate";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DISABLE_CHANGE_TRACK = "disable_change_track";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DISABLE_MULTIROOM = "disable_multiroom";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DISABLE_MUSIC_ATTRACTIVE_CARD = "disable_music_attractive_card";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DISABLE_PARALLEL_SEARCH = "disable_parallel_search";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DISABLE_RELATED_ON_DIRECT_GALLERY = "disable_related_on_direct_gallery";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DISABLE_SEARCH_POI = "disable_search_poi_2";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DISABLE_TAXI_NEW = "disable_taxi_new";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DISABLE_VIDEO_HOW_LONG = "disable_video_how_long";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DJ_SERVICE_FOR_GAMES_ONBOARDING = "dj_service_for_games_onboarding";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DJ_SERVICE_FOR_GREETINGS_NOCARDS = "dj_service_for_greetings_nocards";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DJ_SERVICE_FOR_ONBOARDING_NAVI = "dj_service_for_onboarding_navi";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DJ_SERVICE_FOR_ONBOARDING_NOCARDS = "dj_service_for_onboarding_nocards";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DJ_SERVICE_FOR_ONBOARDING_SMARTSPEAKER_MAIN = "dj_service_for_onboarding_smartspeaker_main";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DJ_SERVICE_FOR_ONBOARDING_YAMUSIC = "dj_service_for_onboarding_yamusic";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_ACCOUNT_LINKING_ON_DESKTOP_BROWSER = "enable_account_linking_on_desktop_browser";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_CHATS_DISCOVERY = "external_skills_discovery_chats";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DISABLE_MEMENTO_REMINDERS = "disable_memento_reminders";
inline constexpr TStringBuf ENABLE_OPEN_LINK_AND_CLOUD_UI = "enable_open_link_and_cloud_ui";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_FACTOID_CHILD_ANSWER = "enable_factoid_child_answer";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_MESSAGE_BUS = "enable_message_bus";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_NER_FOR_SKILLS = "enable_ner_for_skills";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_SEARCH_SWITCH_TO_TRANSLATE = "search_switch_to_translate";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_SERP_GALLERY = "enable_serp_gallery";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_SKILLS_DISCOVERY = "external_skills_discovery";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_SKILLS_SSL_CHECK = "external_skills_ssl_check";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_SKILLS_WEB_DISCOVERY = "external_skills_web_discovery";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_ENABLE_SKILLS_WEB_DISCOVERY_USE_RELEVANCE_BOUNDARY = "external_skills_web_discovery_use_relevance_boundary";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_FAIRYTALE_RADIO = "fairytale_radio";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_FILM_GALLERY = "film_gallery";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_FIND_POI_GALLERY_OPEN_SHOW_ROUTE = "find_poi_gallery_open_show_route";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_FIND_POI_ONE = "find_poi_one";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_GAMES_ONBOARDING_MULTICOLUMN_CARD = "games_onboarding_multicolumn_card";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_MORDOVIA_SUPPORT_CHANNELS = "mordovia_support_channels";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_MORNING_SHOW_CONFIG_ID_PREFIX = "hw_alice_show_config_id=";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_MUSIC_BARE_SEARCH_TEXT = "music_bare_search_text";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_MUSIC_CHECK_PLUS_PROMO = "music_check_plus_promo";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_MUSIC_DONT_SHOW_FIRST_TRACK = "music_dont_show_first_track";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_MUSIC_FORCE_SHOW_FIRST_TRACK = "music_force_show_first_track";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_MUSIC_FULL_SCREEN_PLAYER = "music_full_screen_player";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_MUSIC_LITE_SEARCH_TEXT = "music_lite_search_text";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_MUSIC_RICH_TRACKS = "music_rich_tracks";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_MUSIC_SEND_PLUS_BUY_LINK = "music_plus_by_link";
inline constexpr TStringBuf DISABLE_EXPERIMENTAL_FLAG_MUSIC_SEARCH_APP_ADDITIONAL_TRACKS = "disable_music_search_app_additional_tracks";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_MUSIC_USE_UTTERANCE_FOR_SEARCH_QUERY = "music_use_utterance_for_search_query";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_MUSIC_VIDEO_SETUP = "music_video_setup";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_MY_LOCATION = "my_location_on_map";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_NO_DATASYNC_UNIPROXY = "no_datasync_uniproxy";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_NO_GREETINGS = "no_greetings";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_ONBOARDING_MULTICOLUMN_CARD = "onboarding_multicolumn_card";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_REMINDERS = "reminders";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_REMINDERS_CHECK_PUSH_PERMISSION = "reminders_check_push_permission";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_REMINDERS_NO_ASK_CHECK_PUSH_PERMISSION = "reminders_no_ask_check_push_permission";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_SEARCH_INTERNET_FALLBACK_SUGGEST_IMAGE = "themed_search_internet_fallback_suggest";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_SEARCH_NO_MUSIC_FALLBACK = "search_no_music_fallback";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_SEARCH_SETUP = "search_setup";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_SHOW_TV_CHANNELS_GALLERY_IS_ACTIVE = "mm_enable_protocol_scenario=ShowTvChannelsGallery";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DISABLE_SHOW_VIDEO_SETTINGS = "disable_show_video_settings";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_SKILLREC_SUGGEST_ELLIPSIS = "skillrec_suggest_ellipsis";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_SKILL_DISCOVERY_SORT_BY_SCORE = "external_skills_discovery_sort_by_score";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_DISABLE_SKIP_VIDEO_FRAGMENT = "disable_skip_video_fragment";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_SLEEP_TIMERS = "sleep_timers";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_SWITCH_TV_INPUT = "switch_tv_input";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TAXI_CARD_CORP = "taxi_card_corp";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TAXI_CLEAR_DEVICE_GEOPOINTS = "taxi_clear_device_geopoints";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TAXI_CLEAR_HISTORY = "taxi_clear_history";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TAXI_DO_NOT_ADD_OPEN_IN_APP_SUGGEST = "do_not_add_open_in_app_suggest";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TAXI_DO_NOT_CHECK_PREVIOUS_ORDER = "taxi_do_not_check_previous_order";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TAXI_DO_NOT_MAKE_ORDER = "taxi_do_not_make_order";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TAXI_NEW_DESKTOP = "enable_taxi_new_desktop";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TAXI_TESTING_COMMENTS = "taxi_service_comments";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TAXI_USE_TEST_BLACKBOX = "taxi_use_test_blackbox";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TAXI_ZONEINFO = "taxi_use_zoneinfo";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_THEREMINVOX = "thereminvox";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TV_DO_NOT_FILTER_EPISODES_BY_SERVICE = "tv_do_not_filter_episodes_by_service";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TV_PLAY_RESTREAMED_CHANNELS = "tv_play_restreamed_channels";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TV_SPECIAL_PROJECT_GALLERY = "tv_special_project_gallery";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TV_SUBSCRIPTION_CHANNELS_DISABLED = "tv_subscription_channels_disabled";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TV_VOD_TRANSLATION = "tv_vod_translation";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_TV_WITHOUT_CHANNEL_STATUS_CHECK = "tv_without_channel_status_check";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_VIDEO_ONBOARDING_RANDOMIZE = "video_onboarding_randomize";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_VIDEO_ONBOARDING_SEND_PUSH_PREFIX = "video_onboarding_send_push_";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_YAMUSIC_AUDIOBRANDING = "yamusic_audiobranding_score=";
inline constexpr TStringBuf EXPERIMENTAL_FLAG_STATION_PROMO = "station_promo_score=";
inline constexpr TStringBuf EXPERIMANTAL_FLAG_ALARM_SEMANTIC_FRAME = "alarm_semantic_frame";
inline constexpr TStringBuf EXPERIMANTAL_FLAG_ENABLE_SOURCE_CROSS_DC_PREFIX = "enable_cross_dc=";


} // namespace NBASS
