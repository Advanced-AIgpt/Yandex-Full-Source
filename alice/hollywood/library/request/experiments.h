#pragma once

#include <google/protobuf/struct.pb.h>

#include <util/generic/maybe.h>

namespace NAlice::NHollywood {

// Use legacy BASS rules to create utterance and asr_utterance.
// Currently, only Market uses asr_utterance (which is a hack),
// we don't want other scenarios to do that, but there is an option
// just in case.
constexpr TStringBuf EXP_HW_ENABLE_BASS_ADAPTER_LEGACY_UTTERANCE = "hw_enable_bass_adapter_legacy_utterance";

// Custom url suffix for search entity http proxy
constexpr TStringBuf EXP_HW_SEARCH_ENTITY_REQUEST = "hw_search_entity_request=";

constexpr TStringBuf EXP_HOLLYWOOD_SOURCES_LOGGING = "hollywood_sources_logging";

// if set, writes debug info in the NLG subsystem to the rtlog
constexpr TStringBuf EXP_HW_LOG_NLG = "hw_log_nlg";

// if present, clears user ratings history (VideoRater scenario)
constexpr TStringBuf EXP_HW_VIDEO_RATER_CLEAR_HISTORY = "hw_video_rater_clear_history";

// if present, resets morning show pushes in datasync
constexpr TStringBuf EXP_HW_MORNING_SHOW_RESET_PUSHES = "hw_morning_show_reset_pushes";

// if present, forces morning show push
constexpr TStringBuf EXP_HW_MORNING_SHOW_FORCE_PUSH = "hw_morning_show_force_push";

constexpr TStringBuf EXP_HW_MORNING_SHOW_MUSIC_BETWEEN_TOPICS = "hw_morning_show_music_between_topics";
constexpr TStringBuf EXP_HW_MORNING_SHOW_SKILLS_AFTER_TOPICS = "hw_morning_show_skills_after_topics";
constexpr TStringBuf EXP_HW_MORNING_SHOW_DISABLE_SKILL_HISTORY = "hw_morning_show_disable_skill_history";

// if present, evening show's playlist is replaced by playlist of the day
constexpr TStringBuf EXP_HW_EVENING_SHOW_FORCE_PLAYLIST_OF_THE_DAY = "hw_evening_show_force_playlist_of_the_day";

// if present, evening show is allowed by "good evening" phrase
constexpr TStringBuf EXP_HW_ENABLE_EVENING_SHOW_GOOD_EVENING = "hw_enable_evening_show_good_evening";

// if present, good night show is allowed by "good night" phrase
constexpr TStringBuf EXP_HW_ENABLE_GOOD_NIGHT_SHOW_GOOD_NIGHT = "hw_enable_good_night_show_good_night";

// if set, determines how often a user will be shown news or topics suggests in alice show (once in "flag value" days)
constexpr TStringBuf EXP_HW_ALICE_SHOW_INTERACTIVITY_COOLDOWN_PREFIX = "hw_alice_show_interactivity_cooldown=";

// if set, ceases to show news or topics suggests in alice show once a user rejects suggests more than "flag value" times
constexpr TStringBuf EXP_HW_ALICE_SHOW_INTERACTIVITY_REJECTION_LIMIT_PREFIX = "hw_alice_show_interactivity_rejection_limit=";

// if set, picks suggests only from news or topics selected by user in search app
constexpr TStringBuf EXP_HW_ALICE_SHOW_INTERACTIVITY_SELECTED_ONLY_PREFIX = "hw_alice_show_interactivity_selected_only";

// if set, the specified emotion is forced in the alice show
constexpr TStringBuf EXP_HW_ALICE_SHOW_FORCED_EMOTION_PREFIX = "hw_alice_show_forced_emotion=";

// if set, scenario data is cleared on show begin as if the user starts the show for the first time
constexpr TStringBuf EXP_HW_ALICE_SHOW_NEW_USER = "hw_alice_show_new_user";

// if set, alice show does not use rubrics in news
constexpr TStringBuf EXP_ALICE_SHOW_NEWS_NO_RUBRICS = "hw_alice_show_news_no_rubrics";

// if set, news scenario will be disabled
constexpr TStringBuf EXP_DISABLE_NEWS = "hw_disable_news";

// if present, enables navi route confirmation regardless of device state
constexpr TStringBuf EXP_NAVIGATOR_ALICE_CONFIRMATION = "navigator_alice_confirmation";

// if present, fully disables server biometry in music scenario
constexpr TStringBuf EXP_HW_MUSIC_DISABLE_SERVER_BIOMETRY = "hw_music_disable_server_biometry";

// if set, overrides default page size
constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_PAGE_SIZE_PREFIX = "hw_music_thin_client_page_size=";
constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_FIND_TRACK_IDX_PAGE_SIZE_PREFIX = "hw_music_thin_client_find_track_idx_page_size=";

// if set, radio queue is cleared after corresponding number of tracks is finished
constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_RADIO_FRESH_QUEUE_SIZE_PREFIX = "hw_music_thin_client_radio_fresh_queue_size=";

// if present, enable playlist handling in thin music
constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_PLAYLIST = "hw_music_thin_client_playlist";
// if present, use /after-track handle to get shots for all context, not only in origin playlist aka playlist with Alice
constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_SHOTS_FOR_ALL = "hw_music_thin_client_shots_for_all";
// use encrypted hls instead of raw mp3
constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_USE_HLS = "hw_music_thin_client_use_hls";

// if set, repeated skips above the threshold cause a proposal
constexpr TStringBuf EXP_HW_MUSIC_REPEATED_SKIP_THRESHOLD_PREFIX = "hw_music_repeated_skip_threshold=";
// how often a repeated skip proposal can be said (in seconds)
constexpr TStringBuf EXP_HW_MUSIC_REPEATED_SKIP_TIME_DELTA_PREFIX = "hw_music_repeated_skip_time_delta=";
// if set, repeated skip proposal sets ExpectsRequest flag
constexpr TStringBuf EXP_HW_MUSIC_REPEATED_SKIP_EXPECTS_REQUEST = "hw_music_repeated_skip_expects_request";

// if set, repeated skips in onboarding above the threshold cause a proposal
constexpr TStringBuf EXP_HW_MUSIC_ONBOARDING_REPEATED_SKIP_THRESHOLD_PREFIX = "hw_music_onboarding_repeated_skip_threshold=";
// how often a repeated skip proposal can be said in onboarding (in seconds)
constexpr TStringBuf EXP_HW_MUSIC_ONBOARDING_REPEATED_SKIP_TIME_DELTA_PREFIX = "hw_music_onboarding_repeated_skip_time_delta=";
// if set, repeated skip proposal in onboarding sets ExpectsRequest flag
constexpr TStringBuf EXP_HW_MUSIC_ONBOARDING_REPEATED_SKIP_EXPECTS_REQUEST = "hw_music_onboarding_repeated_skip_expects_request";
// if set, user will receive pushes that remind them to like/dislike tracks in tracks game
constexpr TStringBuf EXP_HW_MUSIC_ONBOARDING_TRACKS_REASK = "hw_music_onboarding_tracks_reask";
// if set, determines the number of reminders user will receive in tracks game (per track, default 2)
constexpr TStringBuf EXP_HW_MUSIC_ONBOARDING_TRACKS_REASK_COUNT_PREFIX = "hw_music_onboarding_tracks_reask_count=";
// if set, determines the time before user receives reminder in tracks game (default 90 seconds)
constexpr TStringBuf EXP_HW_MUSIC_ONBOARDING_TRACKS_REASK_DELAY_SECONDS_PREFIX = "hw_music_onboarding_tracks_reask_delay_seconds=";

// enable music track announcement
constexpr TStringBuf EXP_HW_MUSIC_ANNOUNCE = "hw_music_announce";
// enable music track album announcement
constexpr TStringBuf EXP_HW_MUSIC_ANNOUNCE_ALBUM = "hw_music_announce_album";
// enable music track shot announcement
constexpr TStringBuf EX_HW_MUSIC_ANNOUNCE_SHOT = "hw_music_announce_shot";

constexpr TStringBuf EXP_HW_MUSIC_CHANGE_TRACK_NUMBER = "hw_music_change_track_number";
constexpr TStringBuf EXP_HW_MUSIC_CHANGE_TRACK_VERSION = "hw_music_change_track_version";
// enable intent for sending the song text
constexpr TStringBuf EXP_HW_MUSIC_SEND_SONG_TEXT = "hw_music_send_song_text";
// enable intent for playing songs by this artist
constexpr TStringBuf EXP_HW_MUSIC_SONGS_BY_THIS_ARTIST = "hw_music_songs_by_this_artist";
// enable intent for asking what album is this song from
constexpr TStringBuf EXP_HW_MUSIC_WHAT_ALBUM_IS_THIS_SONG_FROM = "hw_music_what_album_is_this_song_from";
// enable intent for asking what is this song about
constexpr TStringBuf EXP_HW_MUSIC_WHAT_IS_THIS_SONG_ABOUT = "hw_music_what_is_this_song_about";
// enable intent for asking what year is this song
constexpr TStringBuf EXP_HW_MUSIC_WHAT_YEAR_IS_THIS_SONG = "hw_music_what_year_is_this_song";

// moved
constexpr TStringBuf EXP_HW_ENABLE_SHUFFLE_IN_HW_MUSIC = "enable_shuffle_in_hw_music";
constexpr TStringBuf EXP_HW_AUDIO_PAUSE_ON_COMMON_PAUSE = "hw_audio_pause_on_common_pause";
constexpr TStringBuf EXP_HW_DISABLE_PAUSE_WITHOUT_PLAYER = "hw_disable_pause_without_player";

// if present, skips ASR hypotheses check in reask scenario
inline constexpr TStringBuf EXP_REASK_SKIP_ASR_HYPO = "reask_skip_asr_hypo";

// Overrides default error margin in meters when locating user address
constexpr TStringBuf EXP_HW_FOOD_USER_ADDRESS_ERROR_MARGIN_PREFIX = "hw_food_user_address_error_margin=";

// if present, HollywoodMusic scenario will think FmRadio should activate on this attempt
constexpr TStringBuf EXP_MUSIC_PLAY_FM_RADIO_ON_ATTEMPT = "music_play_fm_radio_on_attempt=";

// if present, HollywoodMusic scenario will think that we are on this attempt
constexpr TStringBuf EXP_SET_MUSIC_FM_RADIO_ACTIVATION_ATTEMPT_TO = "test_set_music_fm_radio_activation_attempt_to=";

// if present, HollywoodMusic will enable middle-quality 192 audio bitrate (if the device supports it)
constexpr TStringBuf AUDIO_BITRATE_192_EXP = "audio_bitrate192";

// if present, HollywoodMusic will send multiroom directives at thin client
constexpr TStringBuf EXP_HW_MUSIC_THIN_CLIENT_MULTIROOM = "hw_music_thin_client_multiroom";

// if present, Search will be able to send directive for opening cloud_ui screen (in "open_uri" directive)
constexpr TStringBuf EXP_SEARCH_USE_CLOUD_UI = "search_use_cloud_ui";

// if present, enables the find_remote intent of LinkARemote scenario
constexpr TStringBuf EXP_HW_FIND_REMOTE = "hw_find_remote";

constexpr TStringBuf EXP_HW_MUSIC_ENABLE_PREFETCH_GET_NEXT_CORRECTION = "hw_music_enable_prefetch_get_next_correction";

// if present, HollywoodMusic will send show_view directive (if the device supports it)
constexpr TStringBuf EXP_HW_MUSIC_SHOW_VIEW = "hw_music_show_view";

// if present, HollywoodMusic will cache likes tracks request
constexpr TStringBuf EXP_HW_MUSIC_CACHE_LIKES_TRACKS_REQUEST = "hw_music_cache_likes_tracks_request";

// if present, enables using client biometry (guest data sources) in voiceprint scenario
constexpr TStringBuf EXP_HW_VOICEPRINT_ENABLE_BIO_CAPABILITY = "hw_voiceprint_enable_bio_capability";

// if present, enables fallback to server biometry in case of error in processing client biometry
constexpr TStringBuf EXP_HW_VOICEPRINT_ENABLE_FALLBACK_TO_SERVER_BIOMETRY = "hw_voiceprint_enable_fallback_to_server_biometry";

// if present, enables multiaccount product features in voiceprint scenario
constexpr TStringBuf EXP_HW_VOICEPRINT_ENABLE_MULTIACCOUNT = "hw_voiceprint_enable_multiaccount";

// if present, enables remove case in voiceprint scenario
constexpr TStringBuf EXP_HW_VOICEPRINT_ENABLE_REMOVE = "hw_voiceprint_enable_remove";

// use new enrollment directives for client
constexpr TStringBuf EXP_HW_VOICEPRINT_ENROLLMENT_DIRECTIVES = "hw_enrollment_directives";

// allways use bass for enrollment finish phase
constexpr TStringBuf EXP_HW_VOICEPRINT_ENROLLMENT_FINISH_OVER_BASS = "hw_enrollment_finish_over_bass";

// if present, voiceprint scenario uses ApplyFor machanism for update_datasync directives
constexpr TStringBuf EXP_HW_VOICEPRINT_UPDATE_GUEST_DATASYNC = "hw_voiceprint_update_guest_datasync";

// if present, HollywoodMusic will process 'music_thin_client_next' callback in run+apply. Otherwise in run+continue.
constexpr TStringBuf EXP_HW_MUSIC_GET_NEXTS_USE_APPLY = "hw_music_get_nexts_use_apply";

// use "formatFlags" parameter for "/download-info" music backend requests
constexpr TStringBuf EXP_HW_MUSIC_USE_DOWNLOAD_INFO_FORMAT_FLAGS = "hw_music_use_download_info_format_flags";

// RouteManager flags
constexpr TStringBuf EXP_HW_ROUTE_MANAGER_HANDLE_STATE = "hw_route_manager_handle_state";

// if present, add contexts of all rendered nlgs to anaylytics info
constexpr TStringBuf EXP_DUMP_NLG_RENDER_CONTEXT = "dump_nlg_render_context";

constexpr TStringBuf EXP_CONJUGATOR_MODIFIER_ENABLE_SCENARIO_PREFIX = "hw_conjugator_modifier_enable_scenario=";
constexpr TStringBuf EXP_CONJUGATOR_MODIFIER_DUMP_DETAILED_ANALYTICS_INFO = "hw_conjugator_modifier_dump_detailed_analytics_info";

// Do not render nlg in vins, render it in Hollywood
constexpr TStringBuf EXP_HW_RENDER_VINS_NLG = "hw_render_vins_nlg";

// if set, onboarding greetings scenario is enabled
constexpr TStringBuf EXP_HW_ONBOARDING_ENABLE_GREETINGS = "hw_onboarding_enable_greetings";
// if set, greetings images are omitted in onboarding scenario response
constexpr TStringBuf EXP_HW_ONBOARDING_DISABLE_GREETINGS_IMAGES = "hw_onboarding_disable_greetings_images";
// if set, onboarding what_can_you_do scenario is disabled
constexpr TStringBuf EXP_HW_ONBOARDING_DISABLE_WHAT_CAN_YOU_DO = "hw_onboarding_disable_what_can_you_do";
// if set, add DoNothing action with stop grammar
constexpr TStringBuf EXP_HW_WHAT_CAN_YOU_DO_DONT_STOP_ON_DECLINE = "hw_what_can_you_do_dont_stop_on_decline";
// If set, disable main screen phrases and launch screen depending
constexpr TStringBuf EXP_HW_WHAT_CAN_YOU_DO_SWITCH_PHRASES = "hw_what_can_you_do_switch_phrases";
// If set, query skills-rec backend in what-can-you-do scene
constexpr TStringBuf EXP_HW_WHAT_CAN_YOU_DO_USE_SKILLREC = "hw_what_can_you_do_use_skillrec";

// If set, resets the IsOnboarded memento flag in News scenario
constexpr TStringBuf EXP_HW_NEWS_RESET_IS_ONBOARDED = "hw_news_reset_is_onboarded";

// if set, updated get_greetings handler is used
constexpr TStringBuf EXP_HW_ONBOARDING_GREETINGS_USE_UPDATED_BACKEND = "hw_onboarding_greetings_use_updated_backend";

// If set, order scenario will name or don't name unique items. Put true to name items, or false otherwise
constexpr TStringBuf EXP_HW_ORDER_CALLITEMS = "hw_order_call_items";
// If set, run test in order scenario, need number of test as param
constexpr TStringBuf EXP_HW_ORDER_NUMBER_TEST = "hw_order_number_test";
// If set, put given status to first order
constexpr TStringBuf EXP_HW_ORDER_STATUS_TEST = "hw_order_status_test";

// Enable device to nanny device call
constexpr TStringBuf EXP_ENABLE_CALL_TO_NANNY_ENTRY = "hw_enable_call_to_nanny_entry";
// Force to autoaccept incoming call in nanny mode without payload
constexpr TStringBuf EXP_FORCE_NANNY_MODE_ON_INCOMING_CALLS = "hw_force_nanny_mode_on_incoming_calls";

// DIALOG-8360
constexpr TStringBuf EXP_HW_ENABLE_NOTIFICATIONS_DROP_ALL = "hw_enable_notifications_drop_all";

using TExpFlags = THashMap<TString, TMaybe<TString>>;

TExpFlags ExpFlagsFromProto(const google::protobuf::Struct& experiments);

bool IsExpFlagTrue(const TExpFlags& expFlags, const TStringBuf expFlagKey);

} // namespace NAlice::NHollywood
