#pragma once

#include <util/generic/strbuf.h>

namespace NAlice::NMegamind {

/// Item with blackbox data.
inline constexpr TStringBuf AH_ITEM_BLACKBOX = "mm_blackbox";
/// Item where http response must be stored to output it via http_adapter.
inline constexpr TStringBuf AH_ITEM_HTTP_RESPONSE = "http_response";
/// Item where a parsed speechkit request (protobuf) is placed.
inline constexpr TStringBuf AH_ITEM_SPEECHKIT_REQUEST = "mm_speechkit_request";
/// Item where some http and other parsed from http data is stored.
inline constexpr TStringBuf AH_ITEM_UNIPROXY_REQUEST = "mm_uniproxy_request";
/// Item which contains the data required for everynode in Megamind
inline constexpr TStringBuf AH_ITEM_REQUIRED_NODE_META = "mm_required_node_meta";
/// Item which is used by response to understand if error is happened in one of the nodes.
inline constexpr TStringBuf AH_ITEM_ERROR = "mm_error";
/// Item to pass WebSearch query string
inline constexpr TStringBuf AH_ITEM_WEB_SEARCH_QUERY = "mm_web_search_query";

// Proactivity items
inline constexpr TStringBuf AH_ITEM_PROACTIVITY_HTTP_REQUEST_NAME = "mm_skill_proactivity_http_request";
inline constexpr TStringBuf AH_ITEM_PROACTIVITY_HTTP_RESPONSE_NAME = "mm_skill_proactivity_http_response";
inline constexpr TStringBuf AH_FLAG_EXPECT_PROACTIVITY_RESPONSE = "flag_expect_proactivity_response";

// WebSearch items
inline constexpr TStringBuf AH_ITEM_WEBSEARCH_HTTP_REQUEST_NAME = "mm_websearch_http_request";
inline constexpr TStringBuf AH_ITEM_WEBSEARCH_HTTP_RESPONSE_NAME = "mm_websearch_http_response";
inline constexpr TStringBuf AH_FLAG_EXPECT_WEBSEARCH_RESPONSE = "flag_expect_websearch_response";

// Begemot response parts
inline constexpr TStringBuf AH_ITEM_BEGEMOT_RESPONSE_REWRITTEN_REQUEST_PART = "mm_begemot_response_rewritten_request_part";

// Parsed session for internal megamind usage
inline constexpr TStringBuf AH_ITEM_SPEECHKIT_SESSION = "mm_speechkit_session";

inline constexpr TStringBuf AH_ITEM_MISSPELL = "mm_misspell";
inline constexpr TStringBuf AH_ITEM_BEGEMOT_RESPONSE_JSON = "begemot_response_json";
inline constexpr TStringBuf AH_ITEM_ENTITY_SEARCH_RESPONSE_JSON = "entity_search_response_json";
inline constexpr TStringBuf AH_ITEM_FACTORSTORAGE_BINARY = "mm_factorstorage_binary";
inline constexpr TStringBuf AH_ITEM_PRECLASSIFY = "mm_preclassify";
inline constexpr TStringBuf AH_ITEM_QUALITYSTORAGE = "mm_qualitystorage";
inline constexpr TStringBuf AH_ITEM_QUALITYSTORAGE_POSTCLASSIFY = "mm_qualitystorage_postclassify";
inline constexpr TStringBuf AH_ITEM_ANALYTICS_POSTCLASSIFY = "mm_analytics_postclassify";

inline constexpr TStringBuf AH_ITEM_SCENARIO = "mm_scenario";
inline constexpr TStringBuf AH_ITEM_LAUNCHED_SCENARIOS = "launched_scenarios";
inline constexpr TStringBuf AH_ITEM_WINNER_SCENARIO = "mm_winner_scenario";
inline constexpr TStringBuf AH_ITEM_SCENARIOS_RESPONSE_MONITORING = "mm_scenarios_response_monitoring";
inline constexpr TStringBuf AH_ITEM_SCENARIO_ERRORS = "mm_scenario_errors";
inline constexpr TStringBuf AH_ITEM_CONTINUE_RESPONSE_POSTCLASSIFY = "mm_continue_response_postclassify";

inline constexpr TStringBuf AH_ITEM_ERROR_POSTCLASSIFY = "mm_error_postclassify";

inline constexpr TStringBuf SCENARIO_HTTP_ITEM_SUFFIX = "_run_http_proxy_response";
inline constexpr TStringBuf SCENARIO_ITEM_PREFIX = "scenario_";
inline constexpr TStringBuf SCENARIO_PURE_ITEM_SUFFIX = "_run_pure_response";
/// Content field name in json for item HTTP_REQUEST/HTTP_RESPONSE.
inline constexpr TStringBuf AH_ITEM_HTTP_CONTENT_JSFIELD = "content";
/// Status code field name in json for item HTTP_REQUEST/HTTP_RESPONSE.
inline constexpr TStringBuf AH_ITEM_HTTP_STATUS_CODE_JSFIELD = "status_code";
/// Headers field name in json for item HTTP_REQUEST/HTTP_RESPONSE.
inline constexpr TStringBuf AH_ITEM_HTTP_HEADERS_CODE_JSFIELD = "headers";

inline constexpr TStringBuf AH_ITEM_SKR_CLIENT_INFO = "mm_skr_client_info";
inline constexpr TStringBuf AH_ITEM_SKR_EVENT = "mm_skr_event";
inline constexpr TStringBuf AH_ITEM_SKR_PERS_INTENTS = "mm_skr_pers_intents";
inline constexpr TStringBuf AH_ITEM_SKR_USERINFO = "mm_skr_userinfo";

inline constexpr TStringBuf AH_STAGE_TIMESTAMP = "mm_stage_timestamp";

/// Internal apphost item.
inline constexpr TStringBuf AH_ITEM_APPHOST_PARAMS = "app_host_params";

inline constexpr TStringBuf AH_ITEM_MM_REQUEST_DATA = "mm_request_data";
inline constexpr TStringBuf AH_ITEM_MM_RUN_STATE_ANALYTICS_WALKER_PREPARE = "mm_run_state_analytics_walker_prepare";

inline constexpr TStringBuf AH_ITEM_LAUNCHED_COMBINATORS = "launched_combinators";
inline constexpr TStringBuf AH_ITEM_WINNER_COMBINATOR = "mm_winner_combinator";

inline constexpr TStringBuf AH_ITEM_MODIFIER_REQUEST = "mm_modifier_request";
inline constexpr TStringBuf AH_ITEM_MODIFIER_RESPONSE = "mm_modifier_response";

inline constexpr TStringBuf AH_ITEM_ANALYTICS_LOG_CONTEXT = "mm_analytics_log_context";
inline constexpr TStringBuf AH_ITEM_LOGGER_OPTIONS = "logger_options";

inline constexpr TStringBuf AH_ITEM_FULL_MEMENTO_DATA = "full_memento_data";

inline constexpr TStringBuf AH_ITEM_FAKE_ITEM = "fake_item";

inline constexpr TStringBuf AH_ITEM_IS_ALICE_WORLDWIDE_FLAG = "is_alice_worldwide";

} // namespace NAlice::NMegamind
