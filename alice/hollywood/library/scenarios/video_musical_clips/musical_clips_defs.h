#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

namespace NAlice::NHollywood::NMusicalClips {

constexpr TStringBuf MUSIC_RADIO_FEEDBACK_REQUEST_ITEM = "hw_music_radio_feedback_http_request";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_RESPONSE_ITEM = "hw_music_radio_feedback_http_response";
constexpr TStringBuf MUSIC_RADIO_TRACKS_REQUEST_ITEM = "hw_music_radio_tracks_http_request";
constexpr TStringBuf MUSIC_RADIO_TRACKS_RESPONSE_ITEM = "hw_music_radio_tracks_http_response";
constexpr TStringBuf MUSIC_CLIP_REQUEST_ITEM = "hw_music_clip_http_request";
constexpr TStringBuf MUSIC_CLIP_RESPONSE_ITEM = "hw_music_clip_http_response";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_STARTED_REQUEST_ITEM = "hw_music_radio_feedback_started_http_request";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_STARTED_RESPONSE_ITEM = "hw_music_radio_feedback_started_http_response";
constexpr TStringBuf FRONTEND_VH_PLAYER_REQUEST_ITEM = "hw_frontend_vh_player_request";
constexpr TStringBuf FRONTEND_VH_PLAYER_RESPONSE_ITEM = "hw_frontend_vh_player_response";
constexpr TStringBuf FRONTEND_VH_PLAYER_REQUEST_RTLOG_TOKEN_ITEM = "hw_frontend_vh_player_request_rtlog_token";

constexpr TStringBuf MUSICAL_CLIPS_MTV_STATION = "author:rammstein";
constexpr TStringBuf MUSICAL_CLIPS_MYWAVE_STATION = "user:onyourwave";

constexpr TStringBuf ALICE_SHOW_MUSICAL_CLIPS = "alice.show_musical_clips";
constexpr TStringBuf ALICE_PLAYER_FINISHED = "alice.quasar.video_player.finished";
constexpr TStringBuf ALICE_PLAYER_NEXT_TRACK = "personal_assistant.scenarios.player.next_track";
constexpr TStringBuf ALICE_PLAYER_PREV_TRACK = "personal_assistant.scenarios.player.prev_track";
constexpr TStringBuf ALICE_PLAYER_LIKE = "personal_assistant.scenarios.player.like";
constexpr TStringBuf ALICE_PLAYER_DISLIKE = "personal_assistant.scenarios.player.dislike";
constexpr TStringBuf ALICE_PLAYER_REPLAY = "personal_assistant.scenarios.player.replay";

constexpr TStringBuf ANALYTICS_PRODUCT_SCENARIO_NAME = "video";

} // namespace NAlice::NHollywood::NMusicalClips
