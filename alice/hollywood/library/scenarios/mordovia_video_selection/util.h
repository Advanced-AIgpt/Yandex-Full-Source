#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/library/video_common/hollywood_helpers/util.h>

namespace NAlice::NHollywood::NMordovia {

constexpr TStringBuf VIDEO_SELECTION_FRAME = "alice.mordovia_video_selection";
constexpr TStringBuf VIDEO_SELECTION_BY_REMOTE_CONTROL_FRAME = "personal_assistant.scenarios.quasar.select_video_from_gallery_by_remote_control";
constexpr TStringBuf OPEN_MORDOVIA_FRAME = "alice.mordovia_open_mordovia";
constexpr TStringBuf QUASAR_OPEN_HOME_SCREEN_FRAME = "quasar.mordovia.home_screen";
constexpr TStringBuf GO_HOME_FRAME = "personal_assistant.scenarios.quasar.go_home";
constexpr TStringBuf SWITCH_MORDOVIA_TAB_FRAME = "alice.switch_mordovia_tab";

constexpr TStringBuf BEGEMOT_VIDEO_GALLERY = "video_gallery";
constexpr TStringBuf FRONTEND_VH_PLAYER_REQUEST_ITEM = "hw_frontend_vh_player_request";
constexpr TStringBuf FRONTEND_VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM = "hw_frontend_vh_player_request_rtlog_token";
constexpr TStringBuf FRONTEND_VH_PLAYER_RESPONSE_ITEM = "hw_frontend_vh_player_response";

constexpr TStringBuf OTT_STREAMS_META_REQUEST_ITEM = "hw_ott_streams_meta_request";
constexpr TStringBuf OTT_STREAMS_META_REQUEST_RTLOG_TOKEN_ITEM = "hw_ott_streams_meta_request_rtlog_token";
constexpr TStringBuf OTT_STREAMS_META_RESPONSE_ITEM = "hw_ott_streams_meta_response";

constexpr TStringBuf SELECT_VIDEO_CALLBACK_DEPRECATED = "alice.mordovia_video_selection__callback";
constexpr TStringBuf SELECT_VIDEO_CALLBACK = "personal_assistant.scenarios.quasar.select_video_from_gallery__callback";

constexpr TStringBuf CLEAR_QUEUE_DIRECTIVE_NAME = "clear_queue";
constexpr TStringBuf MORDOVIA_SHOW_DIRECTIVE_NAME = "mordovia_show";
constexpr TStringBuf MORDOVIA_COMMAND_DIRECTIVE_NAME = "mordovia_command";

constexpr TStringBuf ANALYTICS_GO_HOME_INTENT_NAME = "go_home";
constexpr TStringBuf ANALYTICS_SELECT_VIDEO_INTENT_NAME = "personal_assistant.scenarios.ether.quasar.video_select";
constexpr TStringBuf ANALYTICS_SELECT_TAB_INTENT_NAME = "select_mordovia_tab";
constexpr TStringBuf ANALYTICS_PRODUCT_SCENARIO_NAME = "video_commands";
constexpr TStringBuf MORDOVIA_WEBVIEW_PATTERN = "video/quasar";

TStringBuf GetCurrentViewKey(const TScenarioRunRequestWrapper& request);

TString PrepareWebviewUrlForPromo(const TStringBuf originalPromoUrl, const TScenarioRunRequestWrapper request);
bool IsSilentResponse(const TScenarioRunRequestWrapper& request);

void AddWebViewCommand(const TScenarioRunRequestWrapper& request, TResponseBodyBuilder& bodyBuilder,
                       TStringBuf host, TStringBuf path, TStringBuf splashDiv, TStringBuf oldViewKey, bool isMainPage = false);

void AddWebViewCommand(const TScenarioRunRequestWrapper& request, TResponseBodyBuilder& bodyBuilder,
                       TStringBuf url, TStringBuf splashDiv, TStringBuf oldViewKey, bool isMainPage = false);

} // namespace NAlice::NHollywood::NMordovia
