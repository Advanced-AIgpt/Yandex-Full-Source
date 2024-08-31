#pragma once

#include <alice/hollywood/library/request/request.h>

#include <util/generic/string.h>

namespace NAlice::NHollywood::NTvChannelsEfir {

constexpr TStringBuf VIDEO_QUASAR_REQUEST_ITEM = "hw_video_quasar_request";
constexpr TStringBuf VIDEO_QUASAR_REQUEST_RTLOG_TOKEN_ITEM = "hw_video_quasar_request_rtlog_token";
constexpr TStringBuf VIDEO_QUASAR_RESPONSE_ITEM = "hw_video_quasar_response";

constexpr TStringBuf VIDEO_RESULT_REQUEST_ITEM = "hw_video_result_request";
constexpr TStringBuf VIDEO_RESULT_REQUEST_RTLOG_TOKEN_ITEM = "hw_video_result_request_rtlog_token";
constexpr TStringBuf VIDEO_RESULT_RESPONSE_ITEM = "hw_video_result_response";

constexpr TStringBuf VH_PLAYER_REQUEST_ITEM = "hw_vh_player_request";
constexpr TStringBuf VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM = "hw_vh_player_request_rtlog_token";
constexpr TStringBuf VH_PLAYER_RESPONSE_ITEM = "hw_vh_player_response";

constexpr TStringBuf SHOW_CHANNEL_BY_NAME_FRAME = "alice.tv_channels_efir.show_tv_channel_by_name";
constexpr TStringBuf SHOW_TV_CHANNELS_GALLERY_FRAME = "alice.tv_channels_efir.show_tv_channels_gallery";

constexpr TStringBuf CHANNEL_NAME_SLOT = "channel_name";

constexpr TStringBuf NLG_NAME = "tv_channels_efir";

constexpr TStringBuf ANALYTICS_PRODUCT_SCENARIO_NAME = "video_commands";
constexpr TStringBuf ANALYTICS_INTENT_NAME = "personal_assistant.scenarios.tv_stream";

TString GetChannelName(const TScenarioRunRequestWrapper& request);

}
