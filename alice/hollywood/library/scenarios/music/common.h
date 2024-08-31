#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/resources/resources.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>
#include <alice/megamind/protos/property/morning_show_profile.pb.h>
#include <alice/memento/proto/api.pb.h>
#include <alice/memento/proto/user_configs.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/data/scenario/music/infinite_feed.pb.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

using namespace ru::yandex::alice::memento::proto;

namespace NAlice::NHollywood::NMusic {

const THashSet<TString> SAVE_PROGRESS_ALBUM_TYPES = {
    "audiobook",
    "lecture",
    "podcast",
    "poetry",
    "show",
};

constexpr TStringBuf TEMPLATE_FM_RADIO_PLAY = "fm_radio";
constexpr TStringBuf TEMPLATE_MUSIC_ONBOARDING = "music_onboarding";
constexpr TStringBuf TEMPLATE_MUSIC_PLAY = "music_play";
constexpr TStringBuf TEMPLATE_PLAYER_WHAT_IS_PLAYING = "player_what_is_playing";
constexpr TStringBuf TEMPLATE_PLAYER_SEND_SONG_TEXT = "player_send_song_text";
constexpr TStringBuf TEMPLATE_PLAYER_SONGS_BY_THIS_ARTIST = "player_songs_by_this_artist";
constexpr TStringBuf TEMPLATE_PLAYER_WHAT_ALBUM_IS_THIS_SONG_FROM = "player_what_album_is_this_song_from";
constexpr TStringBuf TEMPLATE_PLAYER_WHAT_IS_THIS_SONG_ABOUT = "player_what_is_this_song_about";
constexpr TStringBuf TEMPLATE_PLAYER_WHAT_YEAR_IS_THIS_SONG = "player_what_year_is_this_song";
constexpr TStringBuf TEMPLATE_PLAYER_DISLIKE = "player_dislike";
constexpr TStringBuf TEMPLATE_PLAYER_LIKE = "player_like";
constexpr TStringBuf TEMPLATE_PLAYER_NEXT_TRACK = "player_next_track";
constexpr TStringBuf TEMPLATE_PLAYER_PREV_TRACK = "player_no_prev_track";
constexpr TStringBuf TEMPLATE_PLAYER_SHUFFLE = "player_shuffle";
constexpr TStringBuf TEMPLATE_PLAYER_TIMESTAMP_SKIP = "player_timestamp_skip";
constexpr TStringBuf TEMPLATE_PLAYER_REPEAT = "player_repeat";
constexpr TStringBuf TEMPLATE_PLAYER_REPLAY = "player_replay";
constexpr TStringBuf TEMPLATE_PLAYER_REWIND = "player_rewind";
constexpr TStringBuf TEMPLATE_PLAYER_CONTINUE = "player_continue";

inline const TString CENTAUR_COLLECT_MAIN_SCREEN_FRAME{TStringBuf("alice.centaur.collect_main_screen")};
inline const TString MUSIC_PLAY_FRAME{TStringBuf("personal_assistant.scenarios.music_play")};
inline const TString MUSIC_PLAY_ANAPHORA_FRAME{TStringBuf("personal_assistant.scenarios.music_play_anaphora")};
inline const TString MUSIC_PLAY_LESS_FRAME{TStringBuf("personal_assistant.scenarios.music_play_less")};
inline const TString MUSIC_PLAY_FIXLIST_FRAME{TStringBuf("personal_assistant.scenarios.music_play_fixlist")};
inline const TString FM_RADIO_PLAY_FRAME{TStringBuf("alice.music.fm_radio_play")};
inline const TString FORCE_EXIT_FRAME{TStringBuf("alice.proactivity.force_exit")};
inline const TString GET_EQUALIZER_SETTINGS_FRAME{TStringBuf("alice.get_equalizer_settings")};

inline const TString MUSIC_CONTINUATION{TStringBuf("TMusicContinuation")};
inline const TString ORIGINAL_INTENT{TStringBuf("hw_original_intent")};

constexpr TStringBuf MUSIC_ARGUMENTS_ITEM = "hw_music_arguments";

constexpr TStringBuf DATASYNC_MORNING_SHOW_PATH = "/v1/personality/profile/alisa/kv/morning_show";

constexpr TStringBuf DEFAULT_MORNING_SHOW_NEWS_SOURCE = "6e24a5bb-yandeks-novost";
constexpr TStringBuf DEFAULT_MORNING_SHOW_NEWS_RUBRIC = "__mixed_news__";

inline const TString ALICE_SHOW_INTENT = "personal_assistant.scenarios.morning_show";
inline const TString LITE_HARDCODED_MUSIC_PLAY_INTENT = "personal_assistant.scenarios.hardcoded_music_play";

inline const TString ACTION_REQUEST_AUTOPLAY = "autoplay";
inline const TString ACTION_REQUEST_DISLIKE = "dislike";
inline const TString ACTION_REQUEST_LIKE = "like";

constexpr TStringBuf MUSIC_GUEST_OAUTH_TOKEN_AES_ENCRYPTION_KEY_SECRET = "MUSIC_GUEST_OAUTH_TOKEN_AES_ENCRYPTION_KEY";
constexpr TStringBuf MUSIC_GUEST_OAUTH_TOKEN_AES_ENCRYPTION_KEY_SECRET_DEFAULT_VALUE = "F7290DAB8BE3A73F86AD4C963C9E9784D31D85153E853C262E6DF2B043D58859";

TMorningShowProfile ParseMorningShowProfile(const NScenarios::TMementoData& mementoData);

TFrame CreateSpecialAnswerFrame(const NJson::TJsonValue& fixlist, bool hasAudioClient = false);

void AddActionRequestSlot(TFrame& frame, const TString& action);

void AddAutoPlaySlot(TFrame& frame);

bool TryUpdateMorningShowProfileFromFrame(TMorningShowProfile& profile, const TMaybe<THardcodedMorningShowSemanticFrame>& sourceFrame, bool clearCorrespondingConfig);

bool IsDefaultMorningShowProfile(TMaybe<TMorningShowProfile> profile);

bool IsDefaultMorningShowProfile(
    const TMorningShowNewsConfig& newsConfig,
    const TMorningShowTopicsConfig& topicsConfig,
    const TMorningShowSkillsConfig& skillsConfig
);

[[nodiscard]] bool IsNewContentRequestedByCommandByDefault(TMusicArguments_EPlayerCommand playerCommand);

TMusicFmRadioConfig ParseFmRadioConfig(const TRespGetUserObjects& response);

template <typename T>
TConfigKeyAnyPair CreateUserConfigsWithPromoConfig(EConfigKey key, const T& promoConfig) {
    TConfigKeyAnyPair promoConfigPair;
    promoConfigPair.SetKey(key);

    ::google::protobuf::Any configAny;
    configAny.PackFrom(promoConfig);
    *promoConfigPair.MutableValue() = configAny;

    return promoConfigPair;
}

// TODO(klim-roma): Implement TMusicArgumentsBuilder
struct TMusicArgumentsParams {
    TMusicArguments::EExecutionFlowType ExecFlowType;
    bool IsNewContentRequestedByUser;
    const NAlice::TBlackBoxUserInfo* BlackBoxUserInfo{nullptr};
    const NAlice::TIoTUserInfo* IotUserInfo{nullptr};
    const NAlice::TGuestOptions* GuestOptions{nullptr};
    const NAlice::TEnvironmentState* EnvironmentState{nullptr};
    bool IsClientBiometryRunRequest{false};
};

TMusicArguments MakeMusicArgumentsImpl(const TMusicArgumentsParams& musicArgumentsParams);

TMusicArguments MakeMusicArguments(TRTLogger& logger,
                                   const TScenarioRunRequestWrapper& request,
                                   TMusicArguments::EExecutionFlowType execFlowType,
                                   bool isNewContentRequestedByUser);

void AddAliceShowPushDirective(ui32 pushesSent, IRng& rng, TResponseBodyBuilder& bodyBuilder);

bool TryAddAliceShowPushDirective(const TScenarioRunRequestWrapper& request, bool tuned, ui32 pushesSent, ui64 prevTimestamp, IRng& rng, TResponseBodyBuilder& bodyBuilder);

bool HasMusicSubscription(const TScenarioRunRequestWrapper& request);

bool IsLikeDislikeAction(const TString& action, bool& isLike);

bool IsAskingFavorite(const TOnboardingState& onboardingState);
const TOnboardingState::TAskingFavorite& GetAskingFavorite(const TOnboardingState& onboardingState);

bool IsThinRadioSupported(const TScenarioBaseRequestWrapper& request);

bool IsAudioPlayerVsMusicTheLatest(const TDeviceState& deviceState);

bool IsAudioPlayerVsMusicAndBluetoothTheLatest(const TDeviceState& deviceState);

} // namespace NAlice::NHollywood
