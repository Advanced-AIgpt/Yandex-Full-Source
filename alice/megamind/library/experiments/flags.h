#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice {

inline constexpr TStringBuf EXP_DISABLE_ASR_HYPOTHESES_RERANKING = "mm_disable_asr_hypotheses_reranking";

inline constexpr TStringBuf EXP_PREFIX_FAIL_ON_SCENARIO_VERSION_MISMATCH = "mm_fail_on_scenario_version_mismatch=";
inline constexpr TStringBuf EXP_DONT_CALL_SCENARIO_COMMIT = "mm_dont_call_scenario_commit";
inline constexpr TStringBuf EXP_RAISE_ERROR_ON_FAILED_SCENARIOS = "mm_raise_error_on_failed_scenarios";
inline constexpr TStringBuf EXP_MOVE_TUNNELLER_RESPONSES_FROM_SCENARIOS = "mm_move_tunneller_responses_from_scenarios";
inline constexpr TStringBuf EXP_DISABLE_APPLY = "mm_disable_apply";

inline constexpr TStringBuf EXP_PREFIX_PRE_CONFIDENT_SCENARIO_THRESHOLD_FOR_SPECIFIC_CLIENT = "mm_pre_confident_scenario_threshold__";
inline constexpr TStringBuf EXP_DONT_DEFER_APPLY = "mm_dont_defer_apply";
inline constexpr TStringBuf EXP_DISABLE_EFFECTFUL_VINS = "mm_disable_effectful_vins";
inline constexpr TStringBuf EXP_ENABLE_ASYNC_SCENARIO_POLL = "deprecated_mm_enable_async_scenario_poll";
inline constexpr TStringBuf EXP_ENABLE_EARLY_CONTINUE = "deprecated_mm_enable_early_continue";
inline constexpr TStringBuf EXP_ENABLE_RANKING_FEATURES_LOGGING = "mm_enable_ranking_features_logging";
inline constexpr TStringBuf EXP_ENTITY_RESULT_FOR_VINS_DISABLE = "entity_result_for_vins_disable";
inline constexpr TStringBuf EXP_HR_RANKING_FEATURES = "mm_hr_ranking_features";
// Leaves only one scenario enabled regardless of other flags
inline constexpr TStringBuf EXP_PREFIX_MM_SCENARIO = "mm_scenario=";
// Leaves only Vins and scenarios that depends on its classsification
// For development purposes
inline constexpr TStringBuf EXP_FORCE_VINS_SCENARIOS = "mm_force_vins_scenarios";
// Leaves only one scenario enabled regardless of other flags and forces scenario request regardless of preclassifier.
// Requests with this flag are ONLY allowed for development purposes
// TODO(MEGAMIND-1758): remove this flag
inline constexpr TStringBuf EXP_PREFIX_MM_FORCE_SCENARIO = "mm_force_scenario=";
inline constexpr TStringBuf EXP_PREFIX_MM_FORMULA = "mm_formula=";
inline constexpr TStringBuf EXP_PREFIX_MM_FORMULA_FOR_SPECIFIC_CLIENT = "mm_formula__";
inline constexpr TStringBuf EXP_PREFIX_MM_PRECLASSIFIER_THRESHOLDS = "mm_preclassifier_thresholds=";
inline constexpr TStringBuf EXP_PREFIX_SIDE_SPEECH_THRESHOLD = "mm_side_speech_threshold=";
inline constexpr TStringBuf EXP_WIZARD_RESULT_FOR_VINS_DISABLE = "wizard_result_for_vins_disable";
inline constexpr TStringBuf EXP_DISABLE_BEGEMOT_ANAPHORA_RESOLVER_IN_VINS = "mm_disable_begemot_anaphora_resolver_in_vins";
inline constexpr TStringBuf EXP_DISABLE_KV_SAAS = "mm_disable_kv_saas";
inline constexpr TStringBuf EXP_ENABLE_QUERY_TOKEN_STATS = "mm_enable_query_token_stats";
inline constexpr TStringBuf EXP_DONT_USE_PARSED_FRAMES_RULE = "mm_dont_use_parsed_frames_rule";
inline constexpr TStringBuf EXP_DEBUG_RESPONSE_MODIFIERS = "debug_response_modifiers";
inline constexpr TStringBuf EXP_DISABLE_MM_INTENT_FIXLIST = "mm_disable_intent_fixlist";
inline constexpr TStringBuf EXP_DISABLE_TAGGER = "mm_disable_tagger";
inline constexpr TStringBuf EXP_DEBUG_SHOW_SENSITIVE_DATA = "mm_black_sheep_wall";
inline constexpr TStringBuf EXP_DISABLE_MULTIPLE_SESSIONS = "mm_single_session";
inline constexpr TStringBuf EXP_ENABLE_SESSION_RESET = "mm_enable_session_reset";
inline constexpr TStringBuf EXP_DISABLE_PRECLASSIFIER_HINTS = "mm_disable_preclassifier_hints";
inline constexpr TStringBuf EXP_DISABLE_PRECLASSIFIER_HINT_PREFIX = "mm_disable_preclassifier_hint=";
inline constexpr TStringBuf EXP_DISABLE_PRECLASSIFIER_CONFIDENT_FRAMES = "mm_disable_preclassifier_confident_frames";
inline constexpr TStringBuf EXP_POSTCLASSIFIER_GC_FORCE_INTENTS_PREFIX = "mm_postclassifier_gc_force_intents=";
inline constexpr TStringBuf EXP_DISABLE_PLAYER_OWNER_PRIORITY = "mm_disable_player_owner_priority";
inline constexpr TStringBuf EXP_DISABLE_PLAYER_FEATURES = "mm_disable_player_features";
inline constexpr TStringBuf EXP_ADD_PRECLASSIFIER_CONFIDENT_FRAME_PREFIX = "mm_add_preclassifier_confident_frame_";
inline constexpr TStringBuf EXP_ENABLE_VOICE_MISSPELL = "mm_enable_voice_misspell";
inline constexpr TStringBuf EXP_SET_SCENARIO_DATASOURCE_PREFIX = "mm_set_datasource_required=";
inline constexpr TStringBuf EXP_ENABLE_MEMENTO_SURFACE_DATA = "mm_enable_memento_surface_data";
inline constexpr TStringBuf EXP_DISABLE_SIDE_SPEECH_CLASSIFIER = "mm_disable_side_speech_classifier";
inline constexpr TStringBuf EXP_DISABLE_TRAINED_PRECLASSIFIER = "mm_disable_trained_preclassifier";
inline constexpr TStringBuf EXP_ENABLE_PARTIAL_PRECLASSIFIER = "mm_enable_partial_preclassifier";
inline constexpr TStringBuf EXP_PARTIAL_PRECLASSIFIER_THRESHOLD_PREFIX = "mm_partial_preclassifier_threshold=";
// Polyglot flags
inline constexpr TStringBuf EXP_DISABLE_REQUEST_TRANSLATION = "mm_disable_request_translation";
inline constexpr TStringBuf EXP_REQUEST_TRANSLATION_PREFIX = "mm_request_translation=";
inline constexpr TStringBuf EXP_DISABLE_POLYGLOT_BEGEMOT = "mm_disable_polyglot_begemot";
inline constexpr TStringBuf EXP_DISABLE_RESPONSE_TRANSLATION = "mm_disable_response_translation";
inline constexpr TStringBuf EXP_RESPONSE_TRANSLATION_PREFIX = "mm_response_translation=";
inline constexpr TStringBuf EXP_POLYGLOT_VOICE_PREFIX = "mm_polyglot_voice_prefix=";
inline constexpr TStringBuf EXP_FORCE_TRANSLATED_LANGUAGE_FOR_ALL_SCENARIOS = "mm_force_translated_language_for_all_scenarios";
inline constexpr TStringBuf EXP_FORCE_POLYGLOT_LANGUAGE_FOR_SCENARIO_PREFIX = "mm_force_polyglot_language_for_scenario=";
inline constexpr TStringBuf EXP_FORCE_TRANSLATED_LANGUAGE_FOR_SCENARIO_PREFIX = "mm_force_translated_language_for_scenario=";

// Flag for forcing LANG_RUS pre / post classifiers instead of native ones
inline constexpr TStringBuf EXP_FORCE_RUS_CLASSIFIERS = "mm_force_rus_classifiers";

inline constexpr TStringBuf EXP_ALLOW_LANG_EN = "mm_allow_lang_en";
inline constexpr TStringBuf EXP_ALLOW_LANG_AR = "mm_allow_lang_ar";

inline constexpr TStringBuf EXP_ENABLE_STACK_ENGINE_MEMENTO_BACKUP = "mm_enable_memento_stack_engine_backup";
inline constexpr TStringBuf EXP_DISABLE_STACK_ENGINE_RECOVERY_CALLBACK = "mm_disable_stack_engine_recovery_callback";

// WebSearch related flags.
inline constexpr TStringBuf EXP_DISABLE_WEBSEARCH_ADS_FOR_MEGAMIND = "websearch_disable_ads_for_megamind";
inline constexpr TStringBuf EXP_DISABLE_WEBSEARCH_REQUEST = "websearch_disable";
inline constexpr TStringBuf EXP_DISABLE_WEBSEARCH_HARDCODED_LR = "websearch_disable_hardcoded_lr";
inline constexpr TStringBuf EXP_WEBSEARCH_PASS_RTLOG_TOKEN = "websearch_pass_rtlog_token";
// LOG_INFO websearch request proto in AppHost.
inline constexpr TStringBuf EXP_DUMP_WEBSEARCH_REQUEST = "websearch_request_dump";

inline constexpr TStringBuf EXP_PREFIX_WEBSEARCH = "websearch_cgi_";

inline constexpr TStringBuf EXP_ENABLE_WIZARD = "mm_wizard";

// Begemot experiments
inline constexpr TStringBuf EXP_ENABLE_BEGEMOT_CONTACTS_LOGS = "mm_enable_begemot_contacts_logs";
inline constexpr TStringBuf EXP_ENABLE_BEGEMOT_RULES_PREFIX = "mm_enable_begemot_rules=";

// Music experiments
inline constexpr TStringBuf EXP_MUSIC_PLAY_CONFIDENCE_THRESHOLD_PREFIX = "mm_music_play_confidence_threshold=";
inline constexpr TStringBuf EXP_MUSIC_PLAY_DISABLE_CONFIDENCE_THRESHOLD = "mm_music_play_disable_confidence_threshold";
inline constexpr TStringBuf EXP_MUSIC_PLAY_DISABLE_UNCONDITIONAL_SWAP_TRICK = "mm_music_play_disable_unconditional_swap_trick";
inline constexpr TStringBuf EXP_MUSIC_PLAY_ENABLE_CONFIDENCE_THRESHOLD_SPEAKERS_SEARCH_APPS = "mm_music_play_enable_confidence_threshold_speakers_search_apps";
inline constexpr TStringBuf EXP_MUSIC_FAIRY_TALE_PREFER_HW_MUSIC_OVER_VINS = "mm_music_fairy_tale_prefer_music_over_vins";
inline constexpr TStringBuf EXP_MUSIC_FAIRY_TALE_PREFER_HW_MUSIC_OVER_VINS_ON_SMART_SPEAKERS = "mm_music_fairy_tale_prefer_music_over_vins_on_smart_speakers";
inline constexpr TStringBuf EXP_FM_RADIO_PREFER_HW_MUSIC_OVER_VINS_ON_SMART_SPEAKERS = "mm_fm_radio_prefer_hw_music_over_vins_on_smart_speakers";

// Video experiments
inline constexpr TStringBuf EXP_DISABLE_BEGEMOT_ITEM_SELECTOR = "mm_disable_begemot_item_selector";
inline constexpr TStringBuf EXP_VIDEO_ADD_FAST_CONTINUE_PRECLASSIFIER_HINT = "mm_video_add_fast_continue_preclassifier_hint";
inline constexpr TStringBuf EXP_VIDEO_FORCE_ITEM_SELECTION_INSTEAD_OF_TV_CHANNELS = "mm_video_force_item_selection_instead_of_tv_channels";

// Proactivity experiments
inline constexpr TStringBuf EXP_PROACTIVITY_DEBUG_TEXT_RESPONSE = "mm_proactivity_debug_text_response";
inline constexpr TStringBuf EXP_PROACTIVITY_DISABLE_MEMENTO = "mm_proactivity_disable_memento";
inline constexpr TStringBuf EXP_PROACTIVITY_ENABLE_EMOTIONAL_TTS = "mm_proactivity_enable_emotional_tts";
inline constexpr TStringBuf EXP_PROACTIVITY_ENABLE_NOTIFICATION_SOUND = "mm_proactivity_enable_notification_sound";
inline constexpr TStringBuf EXP_PROACTIVITY_ENABLE_ON_ANY_EVENT = "mm_proactivity_enable_on_any_event";
inline constexpr TStringBuf EXP_PROACTIVITY_REQUEST_DELTA_THRESHOLD_PREFIX = "mm_proactivity_request_delta_threshold=";
inline constexpr TStringBuf EXP_PROACTIVITY_RESET_SESSION_LIKE_VINS = "mm_proactivity_reset_session_like_vins";
inline constexpr TStringBuf EXP_PROACTIVITY_SERVICE_ALL_APPS = "mm_proactivity_service_all_apps";
inline constexpr TStringBuf EXP_PROACTIVITY_SHOWS_HISTORY_LEN = "mm_proactivity_shows_history_len=";
inline constexpr TStringBuf EXP_PROACTIVITY_STORAGE_UPDATE_TIME_DELTA_PREFIX = "mm_proactivity_storage_update_time_delta="; // seconds
inline constexpr TStringBuf EXP_PROACTIVITY_TIME_DELTA_THRESHOLD_PREFIX = "mm_proactivity_time_delta_threshold="; // minutes

// GC experiments
inline constexpr TStringBuf EXP_ENABLE_GC_PROACTIVITY = "mm_gc_proactivity";
inline constexpr TStringBuf EXP_ENABLE_GC_MEMORY_LSTM = "mm_gc_lstm_memory";
inline constexpr TStringBuf EXP_DISABLE_GC_PURE_PROTOCOL = "mm_gc_pure_protocol_disable";
inline constexpr TStringBuf EXP_DISABLE_GC_FIXLIST_PROTOCOL = "mm_gc_fixlist_protocol_disable";

inline constexpr TStringBuf EXP_ENABLE_RUS_GET_WEATHER_STUB = "mm_enable_rus_get_weather_stub";

inline constexpr TStringBuf EXP_ENABLE_GC_WIZ_DETECTION = "mm_gc_wiz_detection";

// News experiments
inline constexpr TStringBuf EXP_DECREASE_NEWS_PRIORITY = "mm_decrease_news_priority";
inline constexpr TStringBuf EXP_NEWS_DISABLE_CONFIDENCE_THRESHOLD = "mm_news_disable_confidence_threshold";

// Search experiments
inline constexpr TStringBuf EXP_DISABLE_SEARCH_ACTIVATE_BY_VINS = "mm_disable_search_activate_by_vins";

inline constexpr TStringBuf EXP_FILTER_DIRECT_BY_GRANET = "mm_filter_direct_by_granet";
inline constexpr TStringBuf EXP_USE_SEARCH_QUERY_PREPARE_RULE = "mm_use_search_query_prepare_rule";

inline constexpr TStringBuf EXP_LOG_REQUEST_AS_PROTOBUF = "mm_log_request_as_protobuf";

// Market experiments
inline constexpr TStringBuf EXP_HOW_MUCH_BIN_CLASS = "mm_how_much_bin_class";

// Logging related stuff.
inline constexpr auto EXP_LOG_BEGEMOT_RESPONSE{"enable_log_begemot_response"};

inline const TString EXP_PREFIX_MM_ENABLE_PROTOCOL_SCENARIO = "mm_enable_protocol_scenario=";
inline const TString EXP_ENABLE_PROTO_VINS_SCENARIO = EXP_PREFIX_MM_ENABLE_PROTOCOL_SCENARIO + "Vins";

inline const TString EXP_PREFIX_MM_ENABLE_COMBINATOR = "mm_enable_combinator=";

inline const TString EXP_PREFIX_MM_DISABLE_PROTOCOL_SCENARIO = "mm_disable_protocol_scenario=";
inline const TString EXP_DISABLE_PROTO_VINS_SCENARIO = EXP_PREFIX_MM_DISABLE_PROTOCOL_SCENARIO + "Vins";

inline const TString EXP_PREFIX_SUBSCRIBE_TO_FRAME = "mm_subscribe_to_frame=";

inline constexpr TStringBuf EXP_DISABLE_APPHOST_APPLY_SCEANRIOS = "mm_disable_apphost_apply_scenarios";
inline constexpr TStringBuf EXP_DISABLE_APPHOST_CONTINUE_SCEANRIOS = "mm_disable_apphost_continue_scenarios";

inline constexpr TStringBuf EXP_MM_MODIFIERS_ENABLE = "mm_modifiers_enable";
inline constexpr TStringBuf EXP_MM_MODIFIERS_DISABLE = "mm_modifiers_disable";

inline constexpr TStringBuf EXP_MM_WHISPER_TTL_PREFIX = "mm_whisper_ttl=";

inline constexpr TStringBuf EXP_SHOW_WIZARD_LOGS = "mm_show_wizard_logs";

inline constexpr TStringBuf EXP_DEFAULT_WHISPER_CONFIG_FOR_UNAUTHORIZED_USERS = "mm_default_whisper_config_for_unauthorized_users=";
inline constexpr TStringBuf EXP_USE_COMMON_WHISPER_CONFIG_IN_MOBILE_SURFACES = "mm_use_common_whisper_config_in_mobile_surfaces";

inline constexpr TStringBuf EXP_DO_NOT_FORCE_ACTION_EFFECT_FRAME_WHEN_NO_UTTERANCE_UPDATE = "mm_do_not_force_action_effect_frame_when_no_utterance_update";
inline constexpr TStringBuf EXP_PASS_ALL_PARSED_SEMANTIC_FRAMES = "pass_all_parsed_semantic_frames";

inline constexpr TStringBuf EXP_ENABLE_FULL_RTLOG = "enable_full_rtlog";

} // namespace NAlice
