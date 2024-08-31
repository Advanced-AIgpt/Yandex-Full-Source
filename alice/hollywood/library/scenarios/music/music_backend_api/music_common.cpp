#include "consts.h"
#include "music_common.h"

#include <alice/hollywood/library/scenarios/music/biometry/process_biometry.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/hollywood/library/crypto/aes.h>
#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/personal_data/personal_data.h>
#include <alice/hollywood/library/player_features/player_features.h>

#include <alice/bass/libs/client/experimental_flags.h>

#include <library/cpp/string_utils/base64/base64.h>


namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf MUSIC_CONTEXT_ITEM = "hw_music_context";
constexpr TStringBuf MUSIC_THIN_CLIENT_FLAG = "hw_music_thin_client";
constexpr TStringBuf MUSIC_NEED_DISLIKE_TRACK_FLAG = "need_dislike_track";
constexpr TStringBuf MUSIC_NEED_TRACK_FULL_INFO_FLAG = "need_track_full_info";
constexpr TStringBuf MUSIC_NEED_TRACK_SEARCH_FLAG = "need_track_search";
constexpr TStringBuf MUSIC_NEED_PLAYLIST_SEARCH_FLAG = "need_playlist_search";
constexpr TStringBuf MUSIC_NEED_SPECIAL_PLAYLIST_FLAG = "need_special_playlist";
constexpr TStringBuf MUSIC_NEED_NOVELTY_ALBUM_SEARCH_FLAG = "need_novelty_album_search";
constexpr TStringBuf MUSIC_NEED_FEEDBACK_RADIO_STARTED_FLAG = "need_feedback_radio_started";
constexpr TStringBuf MUSIC_NEED_FIND_TRACK_IDX_FLAG = "need_find_track_idx";
constexpr TStringBuf ALPHANUMERIC = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
constexpr TStringBuf DRAW_LED_SCREEN_DIRECTIVE_NAME = "draw_led_screen";
constexpr TStringBuf PLAYER_GIF_URI_PREFIX = "https://static-alice.s3.yandex.net/led-production/player/";
constexpr TStringBuf STATION_PROMO_URL_SOURCE_PP =
    "https://plus.yandex.ru/station-lite?utm_source=pp&utm_medium=dialog_alice&utm_campaign=MSCAMP-24|lite";

constexpr double YAMUSIC_AUDIOBRANDING_PROBABILITY = 0.2;
constexpr double STATION_PROMO_PROBABILITY = 0.2;

THashMap<TMusicArguments_EPlayerCommand, TString> FRONTAL_LED_IMAGE_MAP = {
    {TMusicArguments_EPlayerCommand_NextTrack, "next.gif"},
    {TMusicArguments_EPlayerCommand_PrevTrack, "previous.gif"},
    {TMusicArguments_EPlayerCommand_Continue, "resume.gif"},
    {TMusicArguments_EPlayerCommand_Like, "like.gif"},
    {TMusicArguments_EPlayerCommand_Dislike, "dislike.gif"},
};

} // namespace

TMusicContext GetMusicContext(NAppHost::IServiceContext& ctx) {
    if (auto mCtx = GetMaybeOnlyProto<TMusicContext>(ctx, MUSIC_CONTEXT_ITEM)) {
        return *mCtx;
    }
    return {};
}

void TryAddStationPromoAttention(
    NJson::TJsonValue& stateJson,
    const TStationPromoFastData& stationPromoFastData,
    const TScenarioApplyRequestWrapper& applyRequest,
    IRng& rng)
{
    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();

    if (applyRequest.ClientInfo().IsSearchApp() && !applyArgs.GetHasSmartDevices()) {
        bool tryAddPromo = applyArgs.GetAccountStatus().GetHasPlus();
        if (!tryAddPromo) {
            ui64 puid;
            if (TryFromString(applyArgs.GetAccountStatus().GetUid(), puid)) {
                tryAddPromo = stationPromoFastData.HasNoPlusPuid(puid);
            }
        }

        if (tryAddPromo) {
            double data = STATION_PROMO_PROBABILITY;
            TMaybe<TString> experimentValue = applyRequest.GetValueFromExpPrefix(NBASS::EXPERIMENTAL_FLAG_STATION_PROMO);
            if (experimentValue.Defined()) {
                TryFromString<double>(experimentValue.GetRef(), data);
            }

            if (!applyArgs.GetAccountStatus().GetHasPlus() || rng.RandomDouble(0, 1) < data) {
                Y_ENSURE(stateJson.Has("blocks"));

                if (applyArgs.GetAccountStatus().GetHasPlus()) {
                    NJson::TJsonValue plusAttention;
                    plusAttention["type"] = "attention";
                    plusAttention["attention_type"] = "has_plus";
                    stateJson["blocks"].AppendValue(std::move(plusAttention));
                } else {
                    EraseIf(stateJson["blocks"].GetArraySafe(), [](const NJson::TJsonValue& block) {
                        return (block["type"].GetString() == "attention" &&
                                block["attention_type"].GetString() == "suggest_yaplus") ||
                               (block["type"].GetString() == "suggest" &&
                                block["suggest_type"].GetString() == "yaplus") ||
                               (block["type"].GetString() == "text_card" &&
                                block["phrase_id"].GetString() == "music_start");
                    });
                }

                NJson::TJsonValue textCardBlock;
                textCardBlock["type"] = "text_card";
                textCardBlock["phrase_id"] = "music_start";
                stateJson["blocks"].AppendValue(std::move(textCardBlock));

                NJson::TJsonValue stationPromoAttention;
                stationPromoAttention["type"] = "attention";
                stationPromoAttention["attention_type"] = "station_promo";
                stateJson["blocks"].AppendValue(std::move(stationPromoAttention));

                NJson::TJsonValue stationPromoSuggest;
                stationPromoSuggest["type"] = "suggest";
                stationPromoSuggest["suggest_type"] = "station_promo";
                stationPromoSuggest["data"]["uri"] = STATION_PROMO_URL_SOURCE_PP;
                stateJson["blocks"].AppendValue(std::move(stationPromoSuggest));
            }
        }
    }
}

void AddAudiobrandingAttention(
        NJson::TJsonValue& stateJson,
        const TMusicFastData& fastData,
        const TScenarioApplyRequestWrapper& applyRequest)
{
    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();
    ui64 puid;

    if (stateJson.Has("blocks") &&
        applyRequest.ClientInfo().IsSmartSpeaker()  &&
        TryFromString<ui64>(applyArgs.GetAccountStatus().GetUid(), puid) &&
        fastData.HasTargetingPuid(puid))
    {
        NJson::TJsonValue audiobrandingAttention;
        audiobrandingAttention["type"] = "attention";
        audiobrandingAttention["attention_type"] = "yamusic_audiobranding";

        double data = YAMUSIC_AUDIOBRANDING_PROBABILITY;
        TMaybe<TString> experimentValue = applyRequest.GetValueFromExpPrefix(NBASS::EXPERIMENTAL_FLAG_YAMUSIC_AUDIOBRANDING);
        if (experimentValue.Defined()) {
            TryFromString<double>(experimentValue.GetRef(), data);
        }
        audiobrandingAttention["data"] = data;

        stateJson["blocks"].AppendValue(audiobrandingAttention);
    }
}

void AddMusicContext(NAppHost::IServiceContext& ctx, TMusicContext& mCtx) {
    ctx.AddProtobufItem(mCtx, MUSIC_CONTEXT_ITEM);
}

void AddMusicThinClientFlag(NAppHost::IServiceContext& ctx) {
    ctx.AddFlag(MUSIC_THIN_CLIENT_FLAG);
}

void AddNeedDislikeTrackFlag(NAppHost::IServiceContext& ctx) {
    ctx.AddFlag(MUSIC_NEED_DISLIKE_TRACK_FLAG);
}

void AddTrackFullInfoFlag(NAppHost::IServiceContext& ctx) {
    ctx.AddFlag(MUSIC_NEED_TRACK_FULL_INFO_FLAG);
}

void AddTrackSearchFlag(NAppHost::IServiceContext& ctx) {
    ctx.AddFlag(MUSIC_NEED_TRACK_SEARCH_FLAG);
}

void AddPlaylistSearchFlag(NAppHost::IServiceContext& ctx) {
    ctx.AddFlag(MUSIC_NEED_PLAYLIST_SEARCH_FLAG);
}

void AddSpecialPlaylistFlag(NAppHost::IServiceContext& ctx) {
    ctx.AddFlag(MUSIC_NEED_SPECIAL_PLAYLIST_FLAG);
}

void AddNoveltyAlbumSearchFlag(NAppHost::IServiceContext& ctx) {
    ctx.AddFlag(MUSIC_NEED_NOVELTY_ALBUM_SEARCH_FLAG);
}

void AddFeedbackRadioStartedFlag(NAppHost::IServiceContext& ctx) {
    ctx.AddFlag(MUSIC_NEED_FEEDBACK_RADIO_STARTED_FLAG);
}

void AddFindTrackIdxFlag(NAppHost::IServiceContext& ctx) {
    ctx.AddFlag(MUSIC_NEED_FIND_TRACK_IDX_FLAG);
}

TString GenerateRandomString(IRng& rng, size_t size) {
    TString rv;
    for (size_t i = 0; i < size; ++i) {
        rv += ALPHANUMERIC[rng.Uniform(ALPHANUMERIC.size())];
    }
    return rv;
}

bool IsGuestPlaybackMode(TBiometryOptions::EPlaybackMode playbackMode) {
    return playbackMode == TBiometryOptions_EPlaybackMode_GuestMode;
}

bool IsIncognitoPlaybackMode(TBiometryOptions::EPlaybackMode playbackMode) {
    return playbackMode == TBiometryOptions_EPlaybackMode_IncognitoMode;
}

bool IsMusicThinClientNextTrackCallback(const TStringBuf name) {
    return name == MUSIC_THIN_CLIENT_NEXT_CALLBACK;
}

bool IsMusicTrackPlayLifeCycleCallback(const TStringBuf name) {
    return name == MUSIC_THIN_CLIENT_ON_STARTED_CALLBACK ||
           name == MUSIC_THIN_CLIENT_ON_STOPPED_CALLBACK ||
           name == MUSIC_THIN_CLIENT_ON_FINISHED_CALLBACK ||
           name == MUSIC_THIN_CLIENT_ON_FAILED;
}

bool IsMusicThinClientRecoveryCallback(const TStringBuf name) {
    return name == MUSIC_THIN_CLIENT_RECOVERY_CALLBACK;
}

bool IsMusicOwnerOfAudioPlayer(const NAlice::TDeviceState& deviceState) {
    if (!deviceState.HasAudioPlayer()) {
        return false;
    }
    const auto& audioPlayer = deviceState.GetAudioPlayer();
    const auto& scenarioMeta = audioPlayer.GetScenarioMeta();
    if (const auto it = scenarioMeta.find(SCENARIO_META_OWNER); it != scenarioMeta.end()) {
        return it->second == SCENARIO_META_MUSIC;
    }
    return false;
}

bool IsAudioPlayerPlaying(const NAlice::TDeviceState& deviceState) {
    return deviceState.GetAudioPlayer().GetPlayerState() == TDeviceState_TAudioPlayer_TPlayerState_Playing;
}

void FillPlayerFeatures(TRTLogger& logger, const TScenarioRunRequestWrapper& request, THwFrameworkRunResponseBuilder& response) {
    auto playerFeatures =
        CalcPlayerFeaturesForAudioPlayer(logger, request,
                                         [](const TDeviceState& deviceState) { return NMusic::IsMusicOwnerOfAudioPlayer(deviceState); });
    response.AddPlayerFeatures(std::move(playerFeatures));
}

TAtomicSharedPtr<IRequestMetaProvider> MakeGuestRequestMetaProvider(const NScenarios::TRequestMeta& meta,
                                                                    const TString& guestOAuthTokenEncrypted) {
    TString guestOAuthToken;
    Y_ENSURE(NCrypto::AESDecryptWeakWithSecret(MUSIC_GUEST_OAUTH_TOKEN_AES_ENCRYPTION_KEY_SECRET,
                                               Base64StrictDecode(guestOAuthTokenEncrypted), guestOAuthToken),
                                               "Error while decrypting guest OAuth token");
    return MakeAtomicShared<TGuestRequestMetaProvider>(meta, std::move(guestOAuthToken));
}

TAtomicSharedPtr<IRequestMetaProvider> MakeRequestMetaProviderFromMusicArgs(
    const NScenarios::TRequestMeta& meta,
    const TMusicArguments& applyArgs,
    bool isClientBiometryModeRequest
)
{
    if (isClientBiometryModeRequest) {
        Y_ENSURE(applyArgs.HasGuestCredentials() && !applyArgs.GetGuestCredentials().GetOAuthTokenEncrypted().Empty());
        return MakeGuestRequestMetaProvider(meta, applyArgs.GetGuestCredentials().GetOAuthTokenEncrypted());
    } else {
        return MakeAtomicShared<TRequestMetaProvider>(meta);
    }
}

TAtomicSharedPtr<IRequestMetaProvider> MakeRequestMetaProviderFromPlaybackBiometry(
    const NScenarios::TRequestMeta& meta,
    const TBiometryOptions& biometryOpts
)
{
    if (IsGuestPlaybackMode(biometryOpts.GetPlaybackMode())) {
        Y_ENSURE(!biometryOpts.GetGuestOAuthTokenEncrypted().Empty());
        return MakeGuestRequestMetaProvider(meta, biometryOpts.GetGuestOAuthTokenEncrypted());
    } else {
        return MakeAtomicShared<TRequestMetaProvider>(meta);
    }
}

TMusicRequestModeInfo MakeMusicRequestModeInfo(
    EAuthMethod authMethod,
    const TStringBuf ownerUserId,
    const TScenarioState& scState
)
{
    const auto& biometryOpts = scState.GetQueue().GetPlaybackContext().GetBiometryOptions();
    return TMusicRequestModeInfoBuilder()
            .SetAuthMethod(authMethod)
            .SetRequestMode(ToRequestMode(biometryOpts.GetPlaybackMode()))
            .SetOwnerUserId(ownerUserId)
            .SetRequesterUserId(biometryOpts.GetUserId())
            .BuildAndMove();
}

TMusicRequestModeInfo MakeMusicRequestModeInfoFromMusicArgs(
    const TMusicArguments& applyArgs,
    const TScenarioState& scState,
    EAuthMethod authMethod,
    bool isClientBiometryModeRequest
)
{
    const auto& biometryOpts = scState.GetQueue().GetPlaybackContext().GetBiometryOptions();
    const auto& userId = isClientBiometryModeRequest ? applyArgs.GetGuestCredentials().GetUid() : biometryOpts.GetUserId();
    Y_ENSURE(!userId.Empty());

    auto musicRequestModeInfoBuilder = TMusicRequestModeInfoBuilder()
                            .SetAuthMethod(authMethod)
                            .SetRequesterUserId(userId)
                            .SetOwnerUserId(applyArgs.GetAccountStatus().GetUid());
    if (isClientBiometryModeRequest) {
        if (userId == applyArgs.GetAccountStatus().GetUid()) {
            musicRequestModeInfoBuilder.SetRequestMode(ERequestMode::Owner);
        } else {
            musicRequestModeInfoBuilder.SetRequestMode(ERequestMode::Guest);
        }
    } else {
        musicRequestModeInfoBuilder.SetRequestMode(ToRequestMode(biometryOpts.GetPlaybackMode()));
    }
    return musicRequestModeInfoBuilder.BuildAndMove();
}

void SetUpPlaybackModeUsingClientBiometryDeprecated(TRTLogger& logger,
                                          TScenarioState& scState,
                                          const NHollywood::TScenarioApplyRequestWrapper& request,
                                          const TMusicArguments& applyArgs,
                                          bool isClientBiometryModeApplyRequest)
{
    const auto& ownerUid = applyArgs.GetAccountStatus().GetUid();
    if (isClientBiometryModeApplyRequest) {
        scState.SetBiometryUserId(applyArgs.GetGuestCredentials().GetUid());
        scState.SetGuestOAuthTokenEncrypted(applyArgs.GetGuestCredentials().GetOAuthTokenEncrypted());
        scState.SetIncognito(false);
        if (scState.GetBiometryUserId() == ownerUid) {
            scState.SetPlaybackMode(TScenarioState_EPlaybackMode_OwnerMode);
        } else {
            scState.SetPlaybackMode(TScenarioState_EPlaybackMode_GuestMode);
        }
    } else {
        scState.ClearGuestOAuthTokenEncrypted();
        auto kolonkishUid = GetKolonkishUidFromDataSync(request);
        if (!applyArgs.GetIsOwnerEnrolled() || !kolonkishUid) {
            if (applyArgs.GetIsOwnerEnrolled()) {
                LOG_ERROR(logger) << "Owner is enrolled, but it is failed to get kolonkish uid from DataSync";
            }
            scState.SetBiometryUserId(ownerUid);
            scState.SetIncognito(false);
            scState.SetPlaybackMode(TScenarioState_EPlaybackMode_OwnerMode);
        } else {
            scState.SetBiometryUserId(*kolonkishUid);
            scState.SetIncognito(true);
            scState.SetPlaybackMode(TScenarioState_EPlaybackMode_IncognitoMode);
        }
    }
}

void SetUpPlaybackModeUsingServerBiometryDeprecated(TRTLogger& logger,
                                          TScenarioState& scState,
                                          const NHollywood::TScenarioApplyRequestWrapper& request,
                                          const TMusicArguments& applyArgs)
{
    const auto biometryData = ProcessBiometryOrFallback(logger, request, TStringBuf{applyArgs.GetAccountStatus().GetUid()});
    Y_ENSURE(!biometryData.IsGuestUser, "BioCapability is supported, use SetUpPlaybackModeUsingClientBiometryDeprecated function instead");
    Y_ENSURE(!biometryData.UserId.Empty());
    scState.ClearGuestOAuthTokenEncrypted();
    scState.SetIncognito(biometryData.IsIncognitoUser);
    scState.SetBiometryUserId(biometryData.UserId);
    if (biometryData.IsIncognitoUser) {
        scState.SetPlaybackMode(TScenarioState_EPlaybackMode_IncognitoMode);
    } else {
        scState.SetPlaybackMode(TScenarioState_EPlaybackMode_OwnerMode);
    }
}

bool ShouldRequestNewContent(const TMusicContext& mCtx, const TMusicQueueWrapper& mq) {
    return mCtx.GetFirstPlay() || mq.NeedToChangeState();
}

bool ShouldReceiveNewContent(const TMusicContext& mCtx) {
    return mCtx.GetContentStatus().GetErrorVer2() == NMusic::NoError;
}

bool ShouldReturnContentResponse(const TMusicContext& mCtx, const TMusicQueueWrapper& mq) {
    return ShouldRequestNewContent(mCtx, mq) && ShouldReceiveNewContent(mCtx);
}

bool CommandSupportsFrontalLedImage(const TMusicArguments_EPlayerCommand playerCommand) {
    return FRONTAL_LED_IMAGE_MAP.contains(playerCommand);
}

TString GetFrontalLedImage(const TMusicArguments_EPlayerCommand playerCommand) {
    if (const auto* drawLedImage = FRONTAL_LED_IMAGE_MAP.FindPtr(playerCommand); drawLedImage != nullptr) {
        return TString::Join(PLAYER_GIF_URI_PREFIX, *drawLedImage);
    }
    ythrow yexception() << "Can not get gif for command " << TMusicArguments_EPlayerCommand_Name(playerCommand);
}

NScenarios::TDirective BuildDrawLedScreenDirective(const TString& frontalLedImage) {
    NScenarios::TDirective directive{};
    auto& drawLedScreenDirective = *directive.MutableDrawLedScreenDirective();

    drawLedScreenDirective.SetName(TString(DRAW_LED_SCREEN_DIRECTIVE_NAME));

    auto& drawItem = *drawLedScreenDirective.AddDrawItem();
    drawItem.SetFrontalLedImage(frontalLedImage);
    return directive;
}

bool IsOnYourWaveRequest(const TFrame& frame) {
    return frame.Slots().GetSize() == 1 && frame.FindSlot(NAlice::NMusic::SLOT_ACTION_REQUEST).IsValid();
}

bool IsOnYourWaveRequest(const TMusicArguments& args) {
    if (!args.HasRadioRequest()) {
        return false;
    }
    const auto& radioRequest = args.GetRadioRequest();
    return AnyOf(radioRequest.GetStationIds(), [](const auto& id) {
        return id == "user:onyourwave";
    });
}

} // namespace NAlice::NHollywood::NMusic
