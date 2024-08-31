#pragma once

#include <util/generic/strbuf.h>

namespace NAlice::NExperiments {

inline constexpr TStringBuf TUNNELLER_PROFILE = "tunneller_profile";
inline constexpr TStringBuf TUNNELLER_PROFILE_VIDEO = "tunneller_profile_video";
inline constexpr TStringBuf ANALYTICS_INFO = "analytics_info";
inline constexpr TStringBuf TUNNELLER_ANALYTICS_INFO = "tunneller_analytics_info";
inline constexpr TStringBuf PROACTIVITY_LOG_STORAGE = "proactivity_log_storage";

// Serp Summarization flags.
inline constexpr TStringBuf DISABLE_SERP_SUMMARIZATION = "disable_serp_summarization";
inline constexpr TStringBuf SERP_SUMMARIZATION_CM2_PREFIX = "serp_summarization_cm2_";
inline constexpr TStringBuf SERP_SUMMARIZATION_MAX_ADS_PREFIX = "serp_summarization_max_ads_";
inline constexpr TStringBuf SERP_SUMMARIZATION_QUERY_MX_PREFIX = "serp_summarization_query_mx_";
inline constexpr TStringBuf SERP_SUMMARIZATION_BE_BRAVE = "serp_summarization_be_brave";
inline constexpr TStringBuf ENABLE_ALL_PLATFORMS_HOLLYWOOD_SUMMARIZATION = "enable_all_platforms_hollywood_summarization";
inline constexpr TStringBuf ENABLE_BASS_SEARCHAPP_SUMMARIZATION = "enable_bass_searchapp_summarization";
inline constexpr TStringBuf SUMMARIZATION_TRIGGER_FACTSNIP = "summarization_trigger_factsnip";
inline constexpr TStringBuf SUMMARIZATION_SYNC_REQUEST = "summarization_sync_request";
inline constexpr TStringBuf SERP_SUMMARIZATION_CGI = "serp_summarization_";
inline constexpr TStringBuf SERP_SUMMARIZATION_SRCRWR = "serp_summarization_srcrwr_";

// Related facts promo
inline constexpr TStringBuf DISABLE_RELATED_FACTS_PROMO = "disable_related_facts_promo";
inline constexpr TStringBuf RELATED_FACTS_PROMO_PROBA_PREFIX = "related_facts_promo_proba_";
inline constexpr TStringBuf DISABLE_RELATED_FACTS_SUGGESTS = "disable_related_facts_suggests";
inline constexpr TStringBuf RELATED_FACTS_DISABLE_SHUFFLE = "related_facts_disable_shuffle";
inline constexpr TStringBuf RELATED_FACTS_ENABLE_FILTER = "related_facts_enable_filter";
inline constexpr TStringBuf RELATED_FACTS_DISABLE_NOTHING_ON_DECLINE = "related_facts_disable_nothing_on_decline";
inline constexpr TStringBuf RELATED_FACTS_DONT_USE_DISCOVERY = "related_facts_dont_use_discovery";

inline constexpr TStringBuf VOICE_FACTS_SOURCE_AFTER_TEXT = "voice_facts_source_after_text";

inline constexpr TStringBuf DISABLE_FACT_LISTS = "disable_fact_lists";

// Push notifications promo
inline constexpr TStringBuf PUSH_PATH_SITE = "push_path_site";
inline constexpr TStringBuf HANDOFF_PROMO_PROBA_PREFIX = "handoff_promo_proba_";
inline constexpr TStringBuf HANDOFF_PROMO_LONG = "handoff_promo_long";
inline constexpr TStringBuf DISABLE_PUSH_ANSWER_PP_REMINDER = "disable_handoff_pp_reminder";
inline constexpr TStringBuf HANDOFF_SILENTLY = "handoff_silently";
inline constexpr TStringBuf DISABLE_HANDOFF_ON_NO_ANSWER = "disable_handoff_on_no_answer";
inline constexpr TStringBuf HANDOFF_LISTEN_AFTER_PUSH = "handoff_listen_after_push";
inline constexpr TStringBuf HANDOFF_FILTER = "handoff_filter";

// ObjectAsFact experiments
inline constexpr TStringBuf OBJECT_AS_FACT_LONG_TTS = "object_as_fact_long_tts";
inline constexpr TStringBuf FORCE_SEARCH_GOODWIN = "force_search_goodwin";

// WebSearch related flags.
inline constexpr TStringBuf WEBSEARCH_DISABLE_DIRECT_GALLERY = "disable_direct_gallery";
inline constexpr TStringBuf WEBSEARCH_DISABLE_CONFIRM_DIRECT_HIT = "disable_direct_confirm_hit";
inline constexpr TStringBuf WEBSEARCH_DISABLE_EVERYTHING_BUT_PLATINA = "websearch_disable_everything_but_platina";
inline constexpr TStringBuf WEBSEARCH_DISABLE_REPORT_CACHE = "websearch_disable_report_cache";
inline constexpr TStringBuf WEBSEARCH_REPORT_CACHE_FLAGS_PREFIX = "websearch_report_cache_flags=";
inline constexpr TStringBuf WEBSEARCH_ENABLE_ADS_FOR_NONSEARCH = "websearch_enable_ads_for_nonsearch";
inline constexpr TStringBuf WEBSEARCH_ENABLE_DIRECT_GALLERY = "enable_direct_gallery";
inline constexpr TStringBuf WEBSEARCH_ADD_INIT_HEADER = "add_init_header_to_search_requests";
/// Reverse flag to turn on requesting image sources in report_alice websearch graph (MEGAMIND-785).
inline constexpr TStringBuf WEBSEARCH_ENABLE_IMAGE_SOURCES = "websearch_enable_image_sources";

// Search scenario
inline constexpr TStringBuf DISABLE_BASS_FACTS = "disable_bass_facts";
inline constexpr TStringBuf DISABLE_BASS_FACTS_ON_ONEWORD = "disable_bass_facts_on_oneword";
inline constexpr TStringBuf GC_INSTEAD_FACTS_ON_ONEWORD = "gc_instead_of_oneword_facts";
inline constexpr TStringBuf ENABLE_SERP_REDIRECT = "enable_serp_redirect";
inline constexpr TStringBuf DISABLE_READ_FACTOID_SOURCE = "disable_read_factoid_source";
inline constexpr TStringBuf FLAG_FACT_SNIP_ADDITIONAL_DATA = "fact_snip_additional_data";

// Music flags
inline constexpr TStringBuf DISABLE_EXPERIMENTAL_FLAG_MUSIC_SEARCH_APP_NEW_CARDS = "disable_music_search_app_new_cards";
inline constexpr TStringBuf EXP_MUSIC_EXP_FLAG_PREFIX = "music_exp__"; // for forwarding exps to music backend
inline constexpr TStringBuf EXP_MUSIC_LOG_CATALOG_RESPONSE = "music_log_catalog_response";
inline constexpr TStringBuf EXP_HOLLYWOOD_MUSIC_SERVER_ACTION = "hollywood_music_server_action";
inline constexpr TStringBuf EXP_HOLLYWOOD_NO_MUSIC_PLAY_LESS = "hollywood_no_music_play_less";
inline constexpr TStringBuf EXP_HOLLYWOOD_MUSIC_PLAY_ANAPHORA = "hollywood_music_play_anaphora";
inline constexpr TStringBuf EXP_HW_MUSIC_ENABLE_AMBIENT_SOUND = "hw_music_enable_ambient_sound";
inline constexpr TStringBuf EXP_HW_MUSIC_KEEP_OBJECT_NAME_IN_NLG_FOR_ARA = "hw_music_keep_object_name_in_nlg_for_ara";
inline constexpr TStringBuf EXP_HW_MUSIC_TRACK_CACHE = "hw_music_track_cache";
inline constexpr TStringBuf EXP_HW_MUSIC_COMPLEX_LIKE_DISLIKE = "hw_music_complex_like"; // for 'i like smth' requests
inline constexpr TStringBuf EXP_HW_MUSIC_DISABLE_TANDEM_FOLLOWER_BAN = "hw_music_disable_tandem_follower_ban";
inline constexpr TStringBuf EXP_HW_MUSIC_FAIRY_TALES_ENABLE_ONDEMAND = "hw_music_fairy_tales_enable_ondemand";
inline constexpr TStringBuf EXP_HW_MUSIC_MULTIROOM_REDIRECT = "hw_music_multiroom_redirect"; // server redirect
inline constexpr TStringBuf EXP_HW_MUSIC_MULTIROOM_CLIENT_REDIRECT = "hw_music_multiroom_client_redirect";
inline constexpr TStringBuf EXP_HW_MUSIC_ONBOARDING = "hw_music_onboarding";
inline constexpr TStringBuf EXP_HW_MUSIC_ONBOARDING_DISCOVERY_RADIO = "hw_music_onboarding_discovery_radio";
inline constexpr TStringBuf EXP_HW_MUSIC_ONBOARDING_GENRE_RADIO = "hw_music_onboarding_genre_radio";
inline constexpr TStringBuf EXP_HW_MUSIC_ONBOARDING_MASTER_TRACKS_COUNT = "hw_music_onboarding_master_tracks_count="; // 0 is inf
inline constexpr TStringBuf EXP_HW_MUSIC_ONBOARDING_NO_ARTIST_FROM_TRACK = "hw_music_onboarding_no_artist_from_track";
inline constexpr TStringBuf EXP_HW_MUSIC_ONBOARDING_TRACKS_COUNT = "hw_music_onboarding_tracks_count="; // 0 is inf
inline constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT = "hw_music_thin_client";
inline constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_GENERATIVE = "hw_music_thin_client_generative";
inline constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_GENERATIVE_FORCE_RELOAD_ON_STREAM_PLAY = "music_thin_client_generative_force_reload_on_stream_play";
inline constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_FAIRY_TALE_PLAYLISTS = "hw_music_thin_client_fairy_tale_playlists";
inline constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_FAIRY_TALE_ONDEMAND = "hw_music_thin_client_fairy_tale_ondemand";
inline constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_FM_RADIO = "hw_music_thin_client_fm_radio";
inline constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_USE_SAVE_PROGRESS = "hw_music_thin_client_use_save_progress";
inline constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_FORCE_ICHWILL_ONYOURWAVE = "hw_music_thin_client_force_ichwill_onyourwave";
inline constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_DISABLE_ICHWILL_MYWAVE = "hw_music_thin_client_disable_ichwill_mywave";
inline constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_ALARM = "hw_music_thin_client_alarm";
inline constexpr TStringBuf EXP_HW_MUSIC_HUMAN_READABLE_ALL_ARTISTS = "hw_music_human_readable_all_artists";
inline constexpr TStringBuf EXP_HW_MUSIC_SHOW_VIEW_NEIGHBORING_TRACKS_COUNT = "hw_music_show_view_neighboring_tracks_count=";
inline constexpr TStringBuf EXP_MUSIC_EXTRA_PROMO_PERIOD = "music_extra_promo_period";
inline constexpr TStringBuf EXP_MUSIC_THIN_CLIENT_RADIO_SLOW_NEXT_TRACK = "music_radio_slow_next_track";
inline constexpr TStringBuf EXP_MUSIC_LITE_HINT = "music_lite_hint";
inline constexpr TStringBuf EXP_MUSIC_DISABLE_REASK = "music_disable_reask";
inline constexpr TStringBuf EXP_MUSIC_INTERACTIVE_FAIRYTALES = "interactive_fairytales";
inline constexpr TStringBuf EXP_FAIRY_TALES_AGE_SELECTOR_FORCE_ALWAYS_SEND_PUSH = "fairy_tales_age_selector_force_always_send_push";
inline constexpr TStringBuf EXP_FAIRY_TALES_DISABLE_SHUFFLE = "fairy_tales_disable_shuffle";
inline constexpr TStringBuf EXP_FAIRY_TALES_BEDTIME_TALES = "fairy_tales_bedtime_tales";
inline constexpr TStringBuf EXP_FAIRY_TALES_BEDTIME_TALES_FORCE_ONBOARDING = "fairy_tales_bedtime_tales_force_onboarding";
inline constexpr TStringBuf EXP_FAIRY_TALES_LINEAR_ALBUMS = "fairy_tales_linear_albums";
inline constexpr TStringBuf EXP_REDIRECT_TO_THIN_PLAYER = "redirect_to_thin_player";
inline constexpr TStringBuf EXP_HW_MUSIC_ENABLE_CROSS_DC = "hw_music_enable_cross_dc";

// News flags
inline constexpr TStringBuf EXP_NEWS_DISABLE_RUBRIC_API = "news_disable_rubric_api";

// Direct in Wizard experiment
inline constexpr TStringBuf ENABLE_DIRECT_IN_WIZARD_SCENARIO = "enable_direct_in_wizard";

// Recipe scenario promo in factoids
inline constexpr TStringBuf FACTOID_RECIPE_PREROLL = "factoid_recipe_preroll";

// Translate scenario
inline constexpr TStringBuf EXP_DISABLE_REVERSE_TRANSLATION = "disable_reverse_translation";

inline constexpr TStringBuf DUMP_SESSIONS_TO_LOGS = "dump_sessions_to_logs";

// Moved
inline constexpr TStringBuf EXP_ENABLE_CONTINUE_IN_HW_MUSIC = "enable_continue_in_hw_music";
inline constexpr TStringBuf EXP_ETHER = "ether";
inline constexpr TStringBuf EXP_FIND_POI_GALLERY = "find_poi_gallery";
inline constexpr TStringBuf EXP_FORBIDDEN_INTENTS = "forbidden_intents";
inline constexpr TStringBuf EXP_TV_RESTREAMED_CHANNELS_LIST = "tv_restreamed_channels_list";
inline constexpr TStringBuf EXP_TV_SHOW_RESTREAMED_CHANNELS_IN_GALLERY = "tv_show_restreamed_channels_in_gallery";

// Overrides default gif version for scenarios with LED
inline constexpr TStringBuf EXP_GIF_VERSION = "gif_version=";

inline constexpr TStringBuf EXP_DISABLE_MULTIPLE_CHANGE_FORMS = "bass_disable_multiple_change_forms";

inline constexpr TStringBuf EXPERIMENTAL_FLAG_PLAY_CHANNEL_BY_NAME = "enable_hollywood_scenario_play_channel_by_name";

inline constexpr TStringBuf EXP_ENABLE_OUTGOING_DEVICE_CALLS = "enable_outgoing_device_calls";
inline constexpr TStringBuf EXP_ENABLE_OUTGOING_OPERATOR_CALLS = "enable_outgoing_operator_calls";
inline constexpr TStringBuf EXP_ENABLE_OUTGOING_DEVICE_TO_DEVICE_CALLS = "enable_outgoing_device_to_device_calls";
inline constexpr TStringBuf EXP_HW_DISABLE_DEVICE_CALL_SHORTCUT = "hw_disable_device_call_shortcut";
inline constexpr TStringBuf EXP_HW_PHONE_CALLS_STUB_RESPONSE = "hw_phone_calls_stub_response";
inline constexpr TStringBuf EXP_HW_PHONE_CALLS_MAX_DISPLAY_CONTACTS_PREFIX = "hw_phone_calls_max_display_contacts=";
inline constexpr TStringBuf EXP_HW_ENABLE_PHONE_BOOK_ANALYTICS = "hw_enable_phone_book_analytics";

inline constexpr TStringBuf EXP_DISABLE_ADS_IN_SEARCH = "disable_ads_in_bass_search";
inline constexpr TStringBuf EXP_SEARCH_REQUEST_SRCRWR = "bass_search_request_srcrwr";
inline constexpr TStringBuf EXP_ENABLE_ICOOKIE_WEBSEARCH = "enable_websearch_bass_icookie";

// For new cloud ui at searchapp-like devices
inline constexpr TStringBuf ONBOARDING_USE_CLOUD_UI = "onboarding_use_cloud_ui";

inline constexpr TStringBuf EXPERIMENTAL_FLAG_TEST_MUSIC_SKIP_PLUS_PROMO_CHECK = "test_music_skip_plus_promo_check";

} // namespace NAlice::NExperiments
