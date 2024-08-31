#include "user_aware_handler.h"

#include "common/biometry_delegate.h"
#include "common/blackbox_api.h"
#include "common/data_sync_api.h"
#include "common/uid_utils.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/request/request.sc.h>

#include <util/generic/variant.h>

namespace NBASS {

namespace {

constexpr TStringBuf BLOCK_USER_NAME = "user_name";
constexpr TStringBuf BLOCK_FIRST_NAME = "first_name";
constexpr TStringBuf BLOCK_TYPE = "type";
constexpr TStringBuf BLOCK_USER_INFO_TYPE = "user_info";
constexpr TStringBuf BLOCK_IS_SILENT = "is_silent";

constexpr TStringBuf HANDCRAFTED_FORM_NAME_PREFIX = "personal_assistant.handcrafted.";
constexpr TStringBuf MUSIC_PLAY_FORM_NAME_PREFIX = "personal_assistant.scenarios.music_play";
constexpr TStringBuf PLAYER_LIKE_FORM_NAME = "personal_assistant.scenarios.player_like";
constexpr TStringBuf PLAYER_DISLIKE_FORM_NAME = "personal_assistant.scenarios.player_dislike";

using namespace NAlice::NBiometry;

bool IsMusicPersonalization(const TContext& ctx) {
    return (
        (
            ctx.FormName().StartsWith(MUSIC_PLAY_FORM_NAME_PREFIX) ||
            ctx.FormName() == PLAYER_LIKE_FORM_NAME ||
            ctx.FormName() == PLAYER_DISLIKE_FORM_NAME
        ) &&
        ctx.MetaClientInfo().IsSmartSpeaker() &&
        ctx.HasExpFlag(MUSIC_PERSONALIZATION_EXPERIMENT));
}

class TUserInfoRequester {
public:
    TUserInfoRequester(TContext& ctx, const TUserAwareHandler::TConfig& config, TUserAwareHandler::TDelegate& delegate,
                       TBlackBoxAPI& blackBoxAPI, TDataSyncAPI& dataSyncAPI)
        : Config(config)
        , Delegate(delegate)
        , Ctx(ctx)
        , UidAcquireType(GetUidAcquireType(ctx))
        , BlackBoxAPI(blackBoxAPI)
        , DataSyncAPI(dataSyncAPI)
        , BiometryDelegate(ctx, blackBoxAPI, dataSyncAPI) {
        if (IsMusicPersonalization(Ctx)) {
            // same mode as in quasar_provider.cpp
            Biometry.ConstructInPlace(Ctx.Meta(), BiometryDelegate, TBiometry::EMode::MaxAccuracy);
        } else {
            Biometry.ConstructInPlace(Ctx.Meta(), BiometryDelegate, TBiometry::EMode::HighTNR);
        }
    }

    void Start() {
        if (!NeedToAttachNameToResult()) {
            return;
        }

        switch (UidAcquireType) {
            case EUidAcquireType::BLACK_BOX:
                if (!BlackBoxAPI.GetUserInfo(Ctx, UserInfo)) {
                    return;
                } else {
                    Uid = UserInfo.GetUid();
                }
                break;
            case EUidAcquireType::BIOMETRY:
                if (Biometry->GetUserId(Uid)) {
                    return;
                }
                break;
            case EUidAcquireType::UNAUTHORIZED:
                Uid = GetUnauthorizedUid(Ctx);
                break;
        }

        if (!Uid.empty()) {
            UsernameGetter = DataSyncAPI.GetAsync(Ctx, Uid, TPersonalDataHelper::EUserSpecificKey::UserName,
                                                  Config.PersonalizationAdditionalDataSyncTimeout);
        }
    }

    void Finish() {
        if (!UsernameGetter.Defined()) {
            return;
        }

        TContext* responseForm = Ctx.GetResponseForm().Get();
        if (responseForm == nullptr) {
            responseForm = &Ctx;
        }

        TString userName;
        TDataSyncAsyncResponseVisitor visitor(userName);
        TDataSyncAPI::TAsyncResult asyncResult = (*UsernameGetter)();
        std::visit(visitor, asyncResult);

        if (Ctx.MetaClientInfo().IsYaAuto()) {
            if (!UserInfo.GetFirstName().empty()) {
                TContext::TBlock& userInfoBlock = *responseForm->Block();
                userInfoBlock[BLOCK_FIRST_NAME] = UserInfo.GetFirstName();
                userInfoBlock[BLOCK_TYPE] = BLOCK_USER_INFO_TYPE;
                userInfoBlock[BLOCK_IS_SILENT] = false;
            }
        } else {
            bool isSilent = !NeedToPronounceName();
            auto timestamp = Delegate.GetTimestamp().MilliSeconds();
            auto& sessionState = responseForm->SessionState();
            sessionState.LastUserInfoTimestamp() = timestamp;
            if (!userName.empty()) {
                TContext::TBlock& userInfoBlock = *responseForm->Block();
                userInfoBlock[BLOCK_TYPE] = BLOCK_USER_INFO_TYPE;
                userInfoBlock[BLOCK_USER_NAME] = userName;
                userInfoBlock[BLOCK_IS_SILENT].SetBool(isSilent);
            }
            if (IsMusicPersonalization(Ctx)) {
                if (isSilent) {
                    i64 userInfoNoPronounceCount = 0;
                    if (!sessionState.UserInfoNoPronounceCount().IsNull()) {
                        userInfoNoPronounceCount = sessionState.UserInfoNoPronounceCount();
                    }
                    sessionState.UserInfoNoPronounceCount() = std::min(
                        userInfoNoPronounceCount + 1, Config.MusicNamePronounceDelayCount);
                } else {
                    auto* musicPronounceTimestamps = sessionState.UserInfoMusicPronounceTimestamps().GetRawValue();
                    if (musicPronounceTimestamps->IsNull()) {
                        musicPronounceTimestamps->SetArray();
                    }
                    musicPronounceTimestamps->Push(timestamp);

                    // O(n^2), but MusicNamePronounceDelayCount is small (e.g 5), so it should not matter
                    while (static_cast<i64>(musicPronounceTimestamps->ArraySize()) >
                        Config.MusicNamePronouncePeriod)
                    {
                        musicPronounceTimestamps->Delete(0);
                    }
                    sessionState.UserInfoNoPronounceCount() = 0;
                }
            }
        }
    }

private:
    struct TDataSyncAsyncResponseVisitor {
        explicit TDataSyncAsyncResponseVisitor(TString& userName)
            : UserName(userName) {
        }
        void operator()(const TError& error) {
            LOG(ERR) << "Error retrieving username " << ToString(error.ToJson()) << Endl;
        }
        void operator()(const TString& userName) {
            UserName = userName;
        }
        TString& UserName;
    };

private:
    bool NeedToAttachNameToResult() const {
        if (const auto error = IsValidBiometricsContext(Ctx.ClientFeatures(), Ctx.Meta())) {
            LOG(ERR) << error->ToJson() << Endl;
            return false;
        }
        if (Ctx.MetaClientInfo().IsYaAuto()) {
            return true;
        }
        if (IsMusicPersonalization(Ctx)) {
            return true;
        }

        // TODO(thefacetak): remove once feature is out of experiment (also, perhaps, change tests)
        if (!Ctx.FormName().StartsWith(HANDCRAFTED_FORM_NAME_PREFIX) && !Ctx.HasExpFlag(AUTO_INSERT_EXPERIMENT)) {
            return false;
        }
        if (!Ctx.HasExpFlag(PERSONALIZATION_EXPERIMENT)) {
            return false;
        }
        if (!Ctx.SessionState().IsNull() &&
            !Ctx.SessionState().LastUserInfoTimestamp().IsNull() &&
            TInstant::MilliSeconds(Ctx.SessionState().LastUserInfoTimestamp()) +
                Config.NameDelay >= Delegate.GetTimestamp())
        {
            return false;
        }
        if (UidAcquireType == EUidAcquireType::BIOMETRY &&
            (!Biometry->IsKnownUser() ||
             (!Biometry->IsModeUsed() && (*Biometry->GetBestScore()) < Config.BiometryScoreThreshold)))
        {
            return false;
        }

        return Ctx.GetRng().RandomDouble() >= Config.PersonalizationDropProbabilty;
    }

    bool NeedToPronounceName() const {
        if (!IsMusicPersonalization(Ctx)) {
            return true;
        }
        const auto& sessionState = Ctx.SessionState();
        if (!sessionState.IsNull()) {
            const auto& musicPronounceTimestamps = sessionState.UserInfoMusicPronounceTimestamps();
            if (!musicPronounceTimestamps.IsNull() &&
                static_cast<i64>(musicPronounceTimestamps.Size()) >= Config.MusicNamePronouncePeriod &&
                TInstant::MilliSeconds(musicPronounceTimestamps.GetRawValue()->Front()) +
                    Config.MusicNamePronounceDelayPeroid >= Delegate.GetTimestamp()) {
                return false;
            }

            auto musicNoPronounceCount = sessionState.UserInfoNoPronounceCount();
            if (!musicNoPronounceCount.IsNull() && musicNoPronounceCount != Config.MusicNamePronounceDelayCount) {
                return false;
            }
        }
        return true;
    }

private:
    const TUserAwareHandler::TConfig& Config;
    TUserAwareHandler::TDelegate& Delegate;

    TContext& Ctx;
    EUidAcquireType UidAcquireType;
    TString Uid;
    TMaybe<TDataSyncAPI::TAsyncGetter> UsernameGetter;

    TBlackBoxAPI& BlackBoxAPI;
    TDataSyncAPI& DataSyncAPI;
    TBiometryDelegate BiometryDelegate;
    TMaybe<TBiometry> Biometry;
    TPersonalDataHelper::TUserInfo UserInfo;
};

} // namespace

// TUserAwareHandler::TDelegate ------------------------------------------------
TInstant TUserAwareHandler::TDelegate::GetTimestamp() {
    return TInstant::Now();
}

// TUserAwareHandler -----------------------------------------------------------
TUserAwareHandler::TUserAwareHandler(THolder<IHandler> slaveHandler, const TUserAwareHandler::TConfig& config,
                                     THolder<TUserAwareHandler::TDelegate> delegate, THolder<TBlackBoxAPI> blackBoxAPI,
                                     THolder<TDataSyncAPI> dataSyncAPI)
    : Config(config)
    , Delegate(std::move(delegate))
    , SlaveHandler(std::move(slaveHandler))
    , BlackBoxAPI(std::move(blackBoxAPI))
    , DataSyncAPI(std::move(dataSyncAPI)) {
}

TResultValue TUserAwareHandler::Do(TRequestHandler& r) {
    TUserInfoRequester requester(r.Ctx(), Config, *Delegate, *BlackBoxAPI.Get(), *DataSyncAPI.Get());
    requester.Start();
    TResultValue result = SlaveHandler->Do(r);
    requester.Finish();
    return result;
}

TResultValue TUserAwareHandler::DoSetup(TSetupContext& ctx) {
    return SlaveHandler->DoSetup(ctx);
}

} // namespace NBASS
