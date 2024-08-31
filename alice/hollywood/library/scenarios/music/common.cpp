#include "common.h"

#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/hollywood/library/biometry/client_biometry.h>
#include <alice/hollywood/library/crypto/aes.h>
#include <alice/hollywood/library/multiroom/multiroom.h>
#include <alice/hollywood/library/response/push.h>

#include <alice/library/experiments/flags.h>
#include <alice/library/music/defs.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <alice/protos/data/news_provider.pb.h>
#include <alice/protos/data/scenario/music/topic.pb.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <util/stream/file.h>

#include <google/protobuf/util/message_differencer.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr std::array<TStringBuf, 2> NON_BASS_SLOTS = {
    TStringBuf("name"),
    TStringBuf("nlg"),
};

template<typename TConfig>
TConfig ParseConfig(const TRespGetUserObjects& response, const EConfigKey key) {
    TConfig config;
    for (const auto& keyValuePair : response.GetUserConfigs()) {
        if (keyValuePair.GetKey() == key && keyValuePair.GetValue().Is<TConfig>()) {
            keyValuePair.GetValue().UnpackTo(&config);
        }
    }
    return config;
}

} // namespace

TMorningShowProfile ParseMorningShowProfile(const NScenarios::TMementoData& mementoData) {
    TMorningShowProfile profile;
    const auto& userConfigs = mementoData.GetUserConfigs();
    *profile.MutableTopicsConfig() = userConfigs.GetMorningShowTopicsConfig();
    *profile.MutableNewsConfig() = userConfigs.GetMorningShowNewsConfig();
    *profile.MutableSkillsConfig() = userConfigs.GetMorningShowSkillsConfig();
    return profile;
}


TFrame CreateSpecialAnswerFrame(const NJson::TJsonValue& fixlist, bool hasAudioClient) {
    TFrame frame{MUSIC_PLAY_FRAME};
    TString answerType, id, startFrom;
    for (const auto& [key, value] : fixlist.GetMap()) {
        if (!IsIn(NON_BASS_SLOTS, key)) {
            if (!hasAudioClient && key == NAlice::NMusic::SLOT_OBJECT_TYPE) {
                answerType = value["value"].GetString();
            } else if (!hasAudioClient && key == NAlice::NMusic::SLOT_OBJECT_ID) {
                id = value["value"].GetString();
            } else if (!hasAudioClient && key == NAlice::NMusic::SLOT_START_FROM_TRACK_ID) {
                startFrom = value["value"].GetString();
            } else {
                frame.AddSlot(TSlot{
                    value["name"].GetString(),
                    value["type"].GetString(),
                    TSlot::TValue{value["value"].GetStringRobust()},
                });
            }
        }
    }
    AddAutoPlaySlot(frame);
    if (answerType && id) {
        NJson::TJsonValue value;
        if (startFrom) {
            value["answer_type"] = "track";
            value["id"] = startFrom;
        } else {
            value["answer_type"] = answerType;
            if (answerType == "playlist") {
                TStringBuf ownerId, kind;
                TStringBuf(id).Split(':', ownerId, kind);
                value["kind"] = kind;
                value["owner"]["id"] = ownerId;
            } else {
                value["id"] = id;
            }
        }
        frame.AddSlot(TSlot{
            ToString(NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO),
            ToString(NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO_TYPE),
            TSlot::TValue{value.GetStringRobust()},
        });
    }
    return frame;
}

void AddActionRequestSlot(TFrame& frame, const TString& action) {
    frame.AddSlot(TSlot{ToString(NAlice::NMusic::SLOT_ACTION_REQUEST),
                        ToString(NAlice::NMusic::SLOT_ACTION_REQUEST_TYPE),
                        TSlot::TValue{action}});
}

void AddAutoPlaySlot(TFrame& frame) {
    AddActionRequestSlot(frame, ACTION_REQUEST_AUTOPLAY);
}

bool TryUpdateMorningShowProfileFromFrame(TMorningShowProfile& profile, const TMaybe<THardcodedMorningShowSemanticFrame>& sourceFrame, bool clearCorrespondingConfig) {
    if (sourceFrame.Defined()) {
        if (sourceFrame->GetNewsProvider().HasSerializedData()) {
            NJson::TJsonValue newsProvider = JsonFromString(sourceFrame->GetNewsProvider().GetSerializedData());
            NData::TNewsProvider newsProviderProto = JsonToProto<NData::TNewsProvider>(newsProvider);
            if (clearCorrespondingConfig) {
                profile.MutableNewsConfig()->MutableNewsProviders()->Clear();
                profile.MutableNewsConfig()->SetDisabled(false);
            } else {
                for (const auto& prov : profile.GetNewsConfig().GetNewsProviders()) {
                    if (google::protobuf::util::MessageDifferencer::Equals(newsProviderProto, prov)) {
                        return false;
                    }
                }
            }
            *profile.MutableNewsConfig()->AddNewsProviders() = newsProviderProto;
            profile.MutableNewsConfig()->SetDefault(false);
            return true;
        } else if (sourceFrame->GetTopic().HasSerializedData()) {
            NJson::TJsonValue topic = JsonFromString(sourceFrame->GetTopic().GetSerializedData());
            NData::NMusic::TTopic topicProto = JsonToProto<NData::NMusic::TTopic>(topic);
            if (clearCorrespondingConfig) {
                profile.MutableTopicsConfig()->MutableTopics()->Clear();
                profile.MutableTopicsConfig()->SetDisabled(false);
            } else {
                for (const auto& topic : profile.GetTopicsConfig().GetTopics()) {
                    if (google::protobuf::util::MessageDifferencer::Equals(topicProto, topic)) {
                        return false;
                    }
                }
            }
            *profile.MutableTopicsConfig()->AddTopics() = topicProto;
            profile.MutableTopicsConfig()->SetDefault(false);
            return true;
        }
    }
    return false;
}

bool IsDefaultMorningShowProfile(TMaybe<TMorningShowProfile> profile) {
    return profile.Defined() && IsDefaultMorningShowProfile(
        profile->GetNewsConfig(),
        profile->GetTopicsConfig(),
        profile->GetSkillsConfig()
    );
}

bool IsDefaultMorningShowProfile(
    const TMorningShowNewsConfig& newsConfig,
    const TMorningShowTopicsConfig& topicsConfig,
    const TMorningShowSkillsConfig& skillsConfig
) {
    return newsConfig.GetDefault() && topicsConfig.GetDefault() && skillsConfig.GetDefault();
}

[[nodiscard]] bool IsNewContentRequestedByCommandByDefault(TMusicArguments_EPlayerCommand playerCommand) {
    switch(playerCommand) {
        case TMusicArguments_EPlayerCommand_None:
        case TMusicArguments_EPlayerCommand_NextTrack:
        case TMusicArguments_EPlayerCommand_PrevTrack:
        case TMusicArguments_EPlayerCommand_Continue:
        case TMusicArguments_EPlayerCommand_Like:
        case TMusicArguments_EPlayerCommand_Dislike:
        case TMusicArguments_EPlayerCommand_Shuffle:
        case TMusicArguments_EPlayerCommand_Replay:
        case TMusicArguments_EPlayerCommand_Rewind:
        case TMusicArguments_EPlayerCommand_Repeat:
        case TMusicArguments_EPlayerCommand_Unshuffle:
            return false;
        case TMusicArguments_EPlayerCommand_ChangeTrackNumber:
        case TMusicArguments_EPlayerCommand_ChangeTrackVersion:
            return true;
        default:
            throw yexception() << "Unexpected player command: " << TMusicArguments_EPlayerCommand_Name(playerCommand)
                               << ". Can't determine if it leads to a request for new content.";
    }
}

TMusicFmRadioConfig ParseFmRadioConfig(const TRespGetUserObjects& response) {
    return ParseConfig<TMusicFmRadioConfig>(response, EConfigKey::CK_MUSIC_FM_RADIO);
}

TMusicArguments MakeMusicArgumentsImpl(const TMusicArgumentsParams& musicArgumentsParams) {
    TMusicArguments args;
    args.SetExecutionFlowType(musicArgumentsParams.ExecFlowType);
    args.SetIsNewContentRequestedByUser(musicArgumentsParams.IsNewContentRequestedByUser);
    if (musicArgumentsParams.BlackBoxUserInfo) {
        auto& accountStatus = *args.MutableAccountStatus();
        accountStatus.SetUid(musicArgumentsParams.BlackBoxUserInfo->GetUid());
        accountStatus.SetHasPlus(musicArgumentsParams.BlackBoxUserInfo->GetHasYandexPlus());
        accountStatus.SetHasMusicSubscription(musicArgumentsParams.BlackBoxUserInfo->GetHasMusicSubscription());
        accountStatus.SetMusicSubscriptionRegionId(musicArgumentsParams.BlackBoxUserInfo->GetMusicSubscriptionRegionId());
    }
    if (musicArgumentsParams.IotUserInfo) {
        args.MutableIoTUserInfo()->CopyFrom(*musicArgumentsParams.IotUserInfo);
    }
    if (musicArgumentsParams.GuestOptions) {
        args.SetIsOwnerEnrolled(musicArgumentsParams.GuestOptions->GetIsOwnerEnrolled());
        if (musicArgumentsParams.IsClientBiometryRunRequest) {
            auto& guestCredentials = *args.MutableGuestCredentials();
            guestCredentials.SetUid(musicArgumentsParams.GuestOptions->GetYandexUID());

            TString guestOAuthTokenEncrypted;
            Y_ENSURE(NCrypto::AESEncryptWeakWithSecret(MUSIC_GUEST_OAUTH_TOKEN_AES_ENCRYPTION_KEY_SECRET,
                                                    musicArgumentsParams.GuestOptions->GetOAuthToken(),
                                                    guestOAuthTokenEncrypted), "Error while encrypting guest OAuth token");
            guestCredentials.SetOAuthTokenEncrypted(Base64Encode(guestOAuthTokenEncrypted));
        }
    }
    if (musicArgumentsParams.EnvironmentState) {
        args.MutableEnvironmentState()->CopyFrom(*musicArgumentsParams.EnvironmentState);
    }
    return args;
}

TMusicArguments MakeMusicArguments(TRTLogger& logger,
                                   const TScenarioRunRequestWrapper& request,
                                   TMusicArguments::EExecutionFlowType execFlowType,
                                   bool isNewContentRequestedByUser)
{
    TMusicArgumentsParams musicArgumentsParams;
    musicArgumentsParams.ExecFlowType = execFlowType;
    musicArgumentsParams.IsNewContentRequestedByUser = isNewContentRequestedByUser;
    musicArgumentsParams.BlackBoxUserInfo = GetUserInfoProto(request);
    musicArgumentsParams.IotUserInfo = GetIoTUserInfoProto(request);
    musicArgumentsParams.GuestOptions = GetGuestOptionsProto(request);
    musicArgumentsParams.EnvironmentState = GetEnvironmentStateProto(request);
    musicArgumentsParams.IsClientBiometryRunRequest = IsClientBiometryModeRunRequest(logger, request, musicArgumentsParams.GuestOptions);
    return MakeMusicArgumentsImpl(musicArgumentsParams);
}

namespace {

constexpr size_t PUSH_COUNT = 4;

const std::array<TString, PUSH_COUNT> PUSH_TITLES_FIRST = {
    "❤️ Настройте мое шоу!",
    "Настройте мое шоу!",
    "💎 Настройте шоу Алисы!",
    "⚡️ Настройте шоу Алисы!",
};
const std::array<TString, PUSH_COUNT> PUSH_TITLES_REPEATED = {
    "Настройте шоу Алисы!",
    "👍 Настройте шоу Алисы!",
    "❤️ Я всё еще жду вас",
    "Я всё еще жду вас",
};
const std::array<TString, PUSH_COUNT> PUSH_TEXTS = {
    "Выбирайте, что слушать в Шоу Алисы",
    "Выбирайте, что вам нравится слушать",
    "Выберите новости по своему вкусу",
    "Выбирайте, что слушать в Шоу Алисы",
};
const TString PUSH_TAG_FIRST = "alice_morning_show_settings_1";
const TString PUSH_TAG_REPEATED = "alice_morning_show_settings_2";
const TString PUSH_URL = "https://yandex.ru/quasar/account/show";
const TString PUSH_POLICY_UNLIMITED = "unlimited_policy";

constexpr ui64 PUSH_TIMEOUT = 7 * 24 * 60 * 60;

bool ShouldSendAliceShowPush(const ui32 pushes, const ui64 prevTimestamp, const ui64 curTimestamp, const bool tuned) {
    if (pushes > 1) {
        return false;
    }
    if (curTimestamp - prevTimestamp < PUSH_TIMEOUT) {
        return false;
    }
    if (tuned) {
        return false;
    }
    return true;
}

} // namespace

bool TryAddAliceShowPushDirective(const TScenarioRunRequestWrapper& request, const bool tuned, const ui32 pushesSent, const ui64 prevTimestamp, IRng& rng, TResponseBodyBuilder& bodyBuilder) {
    const bool force = request.HasExpFlag(EXP_HW_MORNING_SHOW_FORCE_PUSH);

    if (!force && !ShouldSendAliceShowPush(pushesSent, prevTimestamp, request.ClientInfo().Epoch, tuned)) {
        return {};
    }

    AddAliceShowPushDirective(pushesSent, rng, bodyBuilder);
    return true;
}

void AddAliceShowPushDirective(const ui32 pushesSent, IRng& rng, TResponseBodyBuilder& bodyBuilder) {
    const bool first = (pushesSent == 0);
    const auto idx = rng.RandomInteger(PUSH_COUNT);
    const auto& pushTitle = first ? PUSH_TITLES_FIRST[idx] : PUSH_TITLES_REPEATED[idx];
    const auto& pushText = PUSH_TEXTS[idx];
    const auto& pushTag = first ? PUSH_TAG_FIRST : PUSH_TAG_REPEATED;
    const auto& pushPolicy = PUSH_POLICY_UNLIMITED;

    TPushDirectiveBuilder{pushTitle, pushText, PUSH_URL, pushTag}.SetThrottlePolicy(pushPolicy).BuildTo(bodyBuilder);
}

bool HasMusicSubscription(const TScenarioRunRequestWrapper& request) {
    const auto* userInfo = GetUserInfoProto(request);
    return userInfo && userInfo->GetHasMusicSubscription();
}

bool IsLikeDislikeAction(const TString& action, bool& isLike) {
    isLike = action == ACTION_REQUEST_LIKE;
    return isLike || action == ACTION_REQUEST_DISLIKE;
}

bool IsAskingFavorite(const TOnboardingState& onboardingState) {
    return onboardingState.GetInOnboarding()
        && onboardingState.OnboardingSequenceSize() > 0
        && onboardingState.GetOnboardingSequence(0).HasAskingFavorite();
}

const TOnboardingState::TAskingFavorite& GetAskingFavorite(const TOnboardingState& onboardingState) {
    return onboardingState.GetOnboardingSequence(0).GetAskingFavorite();
}

bool IsThinRadioSupported(const TScenarioBaseRequestWrapper& request) {
    return request.Interfaces().GetHasAudioClient()
        && request.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT);
}

bool IsAudioPlayerVsMusicTheLatest(const TDeviceState& deviceState) {
    // is the thin player was (or is) the latest active player
    const auto& musicPlayer = deviceState.GetMusic();
    const auto& audioPlayer = deviceState.GetAudioPlayer();

    return audioPlayer.GetLastPlayTimestamp() >= musicPlayer.GetLastPlayTimestamp();
}

bool IsAudioPlayerVsMusicAndBluetoothTheLatest(const TDeviceState& deviceState) {
    // is the thin player was (or is) the latest active player
    const auto& musicPlayer = deviceState.GetMusic();
    const auto& bluetoothPlayer = deviceState.GetBluetooth();
    const auto& audioPlayer = deviceState.GetAudioPlayer();

    return audioPlayer.GetLastPlayTimestamp() >= std::max(musicPlayer.GetLastPlayTimestamp(),
                                                          bluetoothPlayer.GetLastPlayTimestamp());
}

} // namespace NAlice::NHollywood::NMusic
