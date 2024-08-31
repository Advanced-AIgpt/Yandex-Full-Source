#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/music/create_search_request.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <initializer_list>

namespace NAlice::NHollywood::NMusic {

constexpr TStringBuf MUSIC_REQUEST_ITEM = "hw_music_http_request";
constexpr TStringBuf MUSIC_RESPONSE_ITEM = "hw_music_http_response";
constexpr TStringBuf MUSIC_CACHED_RESPONSE_BODY_ITEM = "hw_music_cached_http_response_body";
constexpr TStringBuf MUSIC_RESPONSE_ITEM_HLS = "hw_music_http_response_hls";
constexpr TStringBuf MUSIC_REQUEST_ITEM_MP3_GET_ALICE = "hw_music_http_request_mp3_get_alice";
constexpr TStringBuf MUSIC_RESPONSE_ITEM_MP3_GET_ALICE = "hw_music_http_response_mp3_get_alice";
constexpr TStringBuf MUSIC_RADIO_REQUEST_ITEM = "hw_music_radio_http_request";
constexpr TStringBuf MUSIC_RADIO_RESPONSE_ITEM = "hw_music_radio_http_response";
constexpr TStringBuf MUSIC_GENERATIVE_REQUEST_ITEM = "hw_music_generative_http_request";
constexpr TStringBuf MUSIC_GENERATIVE_RESPONSE_ITEM = "hw_music_generative_http_response";
constexpr TStringBuf MUSIC_DISLIKE_REQUEST_ITEM = "hw_music_dislike_http_request";
constexpr TStringBuf MUSIC_DISLIKE_RESPONSE_ITEM = "hw_music_dislike_http_response";
constexpr TStringBuf MUSIC_REMOVE_DISLIKE_REQUEST_ITEM = "hw_music_http_request_remove_dislike";
constexpr TStringBuf MUSIC_REMOVE_DISLIKE_RESPONSE_ITEM = "hw_music_http_response_remove_dislike";
constexpr TStringBuf MUSIC_TRACK_FULL_INFO_REQUEST_ITEM = "hw_music_track_full_info_http_request";
constexpr TStringBuf MUSIC_TRACK_FULL_INFO_RESPONSE_ITEM = "hw_music_track_full_info_http_response";
constexpr TStringBuf MUSIC_TRACK_SEARCH_REQUEST_ITEM = "hw_music_track_search_http_request";
constexpr TStringBuf MUSIC_TRACK_SEARCH_RESPONSE_ITEM = "hw_music_track_search_http_response";
constexpr TStringBuf MUSIC_PLAYLIST_SEARCH_REQUEST_ITEM = "hw_music_playlist_search_http_request";
constexpr TStringBuf MUSIC_PLAYLIST_SEARCH_RESPONSE_ITEM = "hw_music_playlist_search_http_response";
constexpr TStringBuf MUSIC_SPECIAL_PLAYLIST_REQUEST_ITEM = "hw_music_special_playlist_http_request";
constexpr TStringBuf MUSIC_SPECIAL_PLAYLIST_RESPONSE_ITEM = "hw_music_special_playlist_http_response";
constexpr TStringBuf MUSIC_NOVELTY_ALBUM_SEARCH_REQUEST_ITEM = "hw_music_novelty_album_search_http_request";
constexpr TStringBuf MUSIC_NOVELTY_ALBUM_SEARCH_RESPONSE_ITEM = "hw_music_novelty_album_search_http_response";
constexpr TStringBuf MUSIC_PLAYS_REQUEST_ITEM = "hw_music_http_request_plays";
constexpr TStringBuf MUSIC_PLAYS_RESPONSE_ITEM = "hw_music_http_response_plays";
constexpr TStringBuf MUSIC_SAVE_PROGRESS_REQUEST_ITEM = "hw_music_http_request_save_progress";
constexpr TStringBuf MUSIC_SAVE_PROGRESS_RESPONSE_ITEM = "hw_music_http_response_save_progress";
constexpr TStringBuf MUSIC_LIKE_REQUEST_ITEM = "hw_music_http_request_like";
constexpr TStringBuf MUSIC_LIKE_RESPONSE_ITEM = "hw_music_http_response_like";
constexpr TStringBuf MUSIC_REMOVE_LIKE_REQUEST_ITEM = "hw_music_http_request_remove_like";
constexpr TStringBuf MUSIC_REMOVE_LIKE_RESPONSE_ITEM = "hw_music_http_response_remove_like";
constexpr TStringBuf MUSIC_GENERATIVE_FEEDBACK_REQUEST_ITEM = "hw_music_generative_feedback_http_request";
constexpr TStringBuf MUSIC_GENERATIVE_FEEDBACK_RESPONSE_ITEM = "hw_music_generative_feedback_http_response";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_LIKE_DISLIKE_REQUEST_ITEM = "hw_music_http_request_radio_feedback_like_dislike";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_LIKE_DISLIKE_RESPONSE_ITEM = "hw_music_http_response_radio_feedback_like_dislike";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_REQUEST_ITEM = "hw_music_http_request_radio_feedback";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_RESPONSE_ITEM = "hw_music_http_response_radio_feedback";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_SKIP_REQUEST_ITEM = "hw_music_http_request_radio_feedback_skip";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_SKIP_RESPONSE_ITEM = "hw_music_http_response_radio_feedback_skip";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_TRACK_STARTED_REQUEST_ITEM = "hw_music_http_request_radio_track_started_feedback";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_TRACK_STARTED_RESPONSE_ITEM = "hw_music_http_response_radio_track_started_feedback";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_TRACK_FINISHED_REQUEST_ITEM = "hw_music_http_request_radio_track_finished_feedback";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_TRACK_FINISHED_RESPONSE_ITEM = "hw_music_http_response_radio_track_finished_feedback";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_RADIO_STARTED_REQUEST_ITEM = "hw_music_http_request_radio_feedback_radio_started";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_RADIO_STARTED_RESPONSE_ITEM = "hw_music_http_response_radio_feedback_radio_started";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_DISLIKE_REQUEST_ITEM = "hw_music_http_request_radio_feedback_dislike";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_DISLIKE_RESPONSE_ITEM = "hw_music_http_response_radio_feedback_dislike";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_LIKE_REQUEST_ITEM = "hw_music_http_request_radio_like_feedback";
constexpr TStringBuf MUSIC_RADIO_FEEDBACK_LIKE_RESPONSE_ITEM = "hw_music_http_response_radio_like_feedback";
constexpr TStringBuf MUSIC_ALBUM_REQUEST_ITEM = "hw_music_album_http_request";
constexpr TStringBuf MUSIC_ARTIST_REQUEST_ITEM = "hw_music_artist_http_request";
constexpr TStringBuf MUSIC_ALBUM_RESPONSE_ITEM = "hw_music_album_http_response";
constexpr TStringBuf MUSIC_ARTIST_RESPONSE_ITEM = "hw_music_artist_http_response";
constexpr TStringBuf MUSIC_BILLING_REQUEST_ITEM = "hw_music_billing_http_request";
constexpr TStringBuf MUSIC_BILLING_RESPONSE_ITEM = "hw_music_billing_http_response";
constexpr TStringBuf MUSIC_SHOTS_HTTP_REQUEST_ITEM = "hw_music_shots_http_request";
constexpr TStringBuf MUSIC_SHOTS_HTTP_RESPONSE_ITEM = "hw_music_shots_http_response";
constexpr TStringBuf MUSIC_SHOTS_FEEDBACK_REQUEST_ITEM = "hw_music_shots_feedback_http_request";
constexpr TStringBuf MUSIC_SHOTS_FEEDBACK_RESPONSE_ITEM = "hw_music_shots_feedback_http_response";
constexpr TStringBuf MUSIC_FIND_TRACK_IDX_REQUEST_ITEM = "hw_music_find_track_idx_http_request";
constexpr TStringBuf MUSIC_FIND_TRACK_IDX_RESPONSE_ITEM = "hw_music_find_track_idx_http_response";
constexpr TStringBuf MUSIC_ARTIST_BRIEF_INFO_REQUEST_ITEM = "hw_music_artist_brief_info_http_request";
constexpr TStringBuf MUSIC_ARTIST_BRIEF_INFO_RESPONSE_ITEM = "hw_music_artist_brief_info_http_response";
constexpr TStringBuf MUSIC_ARTIST_TRACKS_REQUEST_ITEM = "hw_music_artist_tracks_http_request";
constexpr TStringBuf MUSIC_ARTIST_TRACKS_RESPONSE_ITEM = "hw_music_artist_tracks_http_response";
constexpr TStringBuf MUSIC_GENRE_OVERVIEW_REQUEST_ITEM = "hw_music_genre_overview_http_request";
constexpr TStringBuf MUSIC_GENRE_OVERVIEW_RESPONSE_ITEM = "hw_music_genre_overview_http_response";
constexpr TStringBuf MUSIC_AVATAR_COLORS_REQUEST_ITEM = "hw_music_avatar_colors_http_request";
constexpr TStringBuf MUSIC_AVATAR_COLORS_RESPONSE_ITEM = "hw_music_avatar_colors_http_response";
constexpr TStringBuf MUSIC_SINGLE_TRACK_REQUEST_ITEM = "hw_music_single_track_http_request";
constexpr TStringBuf MUSIC_SINGLE_TRACK_RESPONSE_ITEM = "hw_music_single_track_http_response";
constexpr TStringBuf MUSIC_PLAYLIST_INFO_REQUEST_ITEM = "hw_music_playlist_info_http_request";
constexpr TStringBuf MUSIC_PLAYLIST_INFO_RESPONSE_ITEM = "hw_music_playlist_info_http_response";
constexpr TStringBuf MUSIC_PREDEFINED_PLAYLIST_INFO_REQUEST_ITEM = "hw_music_predefined_playlist_info_http_request";
constexpr TStringBuf MUSIC_PREDEFINED_PLAYLIST_INFO_RESPONSE_ITEM = "hw_music_predefined_playlist_info_http_response";
constexpr TStringBuf MUSIC_NEIGHBORING_TRACKS_REQUEST_ITEM = "hw_music_neighboring_tracks_http_request";
constexpr TStringBuf MUSIC_NEIGHBORING_TRACKS_RESPONSE_ITEM = "hw_music_neighboring_tracks_http_response";
constexpr TStringBuf MUSIC_CONTENT_RESPONSE_ITEM = "hw_music_content_http_response";
constexpr TStringBuf MUSIC_LIKES_TRACKS_REQUEST_ITEM = "hw_music_likes_tracks_http_request";
constexpr TStringBuf MUSIC_LIKES_TRACKS_RESPONSE_ITEM = "hw_music_likes_tracks_http_response";
constexpr TStringBuf MUSIC_DISLIKES_TRACKS_REQUEST_ITEM = "hw_music_dislikes_tracks_http_request";
constexpr TStringBuf MUSIC_DISLIKES_TRACKS_RESPONSE_ITEM = "hw_music_dislikes_tracks_http_response";
constexpr TStringBuf MUSIC_FM_RADIO_RANKED_LIST_REQUEST_ITEM = "hw_music_fm_radio_ranked_list_http_request";
constexpr TStringBuf MUSIC_FM_RADIO_RANKED_LIST_RESPONSE_ITEM = "hw_music_fm_radio_ranked_list_http_response";

// FIXME(vitvlkv): rtlog token items should be different for all requests
constexpr TStringBuf MUSIC_REQUEST_RTLOG_TOKEN_ITEM = "hw_music_request_rtlog_token";

// order is important!
// hls item should only be used if url_proxy node was skipped
constexpr inline std::initializer_list<TStringBuf> MUSIC_RESPONSE_ITEMS = {
    MUSIC_RESPONSE_ITEM,
    MUSIC_RESPONSE_ITEM_HLS,
    MUSIC_RESPONSE_ITEM_MP3_GET_ALICE,
};

void AddMusicProxyRequest(TScenarioHandleContext& ctx, const NAppHostHttp::THttpRequest& request,
                          const TStringBuf itemName = MUSIC_REQUEST_ITEM);

using TMaybeRawHttpResponse = TMaybe<std::pair<TStringBuf, TString>>;

TMaybeRawHttpResponse GetFirstOfRawHttpResponses(const TScenarioHandleContext& ctx,
                                                 const std::initializer_list<TStringBuf>& itemNames);

} // namespace NAlice::NHollywood::NMusic
