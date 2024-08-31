#pragma once

namespace NAlice::NHollywood::NMusic {

inline const TString SCENARIO_META_CONTENT_ID = "content_id";
inline const TString SCENARIO_META_QUEUE_ITEM = "queue_item";
inline const TString SCENARIO_META_OWNER = "owner";
constexpr TStringBuf SCENARIO_META_MUSIC = "music";

constexpr TStringBuf MUSIC_THIN_CLIENT_NEXT_CALLBACK = "music_thin_client_next";
constexpr TStringBuf MUSIC_THIN_CLIENT_ON_STARTED_CALLBACK = "music_thin_client_on_started";
constexpr TStringBuf MUSIC_THIN_CLIENT_ON_STOPPED_CALLBACK = "music_thin_client_on_stopped";
constexpr TStringBuf MUSIC_THIN_CLIENT_ON_FINISHED_CALLBACK = "music_thin_client_on_finished";
constexpr TStringBuf MUSIC_THIN_CLIENT_ON_FAILED = "music_thin_client_on_failed";
constexpr TStringBuf MUSIC_THIN_CLIENT_RECOVERY_CALLBACK = "music_thin_client_recovery";

constexpr TStringBuf MUSIC_ONBOARDING_ON_PLAY_DECLINE_CALLBACK = "music_onboarding_on_play_decline";
constexpr TStringBuf MUSIC_ONBOARDING_ON_TRACKS_GAME_DECLINE_CALLBACK = "music_onboarding_on_tracks_game_decline";
constexpr TStringBuf MUSIC_ONBOARDING_ON_DONT_KNOW_CALLBACK = "music_onboarding_on_dont_know";
constexpr TStringBuf MUSIC_ONBOARDING_ON_SILENCE_CALLBACK = "music_onboarding_on_silence";
constexpr TStringBuf MUSIC_ONBOARDING_ON_REPEATED_SKIP_DECLINE_CALLBACK = "music_onboarding_on_repeated_skip_decline";

constexpr TStringBuf PAYMENT_REQUIRED_ERROR_CODE = "payment-required";  // user has no Ya.Plus
constexpr TStringBuf PROMO_AVAILABLE_ERROR_CODE = "promo_available";  // user has no Ya.Plus, but has promo

const double EPS = 1e-6;

} // NAlice::NHollywood::NMusic
