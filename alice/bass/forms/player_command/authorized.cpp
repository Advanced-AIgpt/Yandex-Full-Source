#include "alice/bass/forms/music/providers.h"
#include "alice/bass/forms/radio.h"
#include <alice/bass/forms/automotive/media_control.h>
#include <alice/bass/forms/common/biometry_delegate.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/player_command.h>
#include <alice/bass/forms/player/player.h>
#include <alice/bass/forms/tv/tv_helper.h>
#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/forms/video/defs.h>
#include <alice/bass/forms/video/utils.h>
#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS {

namespace {

constexpr TStringBuf ATTENTION_BIOMETRY_GUEST = "biometry_guest";
constexpr TStringBuf ATTENTION_CHANGED_DISLIKE_TO_NEXT_TRACK = "changed_dislike_to_next_track";

const i32 LIKE_DELAY_SHORT_SEC = 60; // 1 minute
const i32 LIKE_DELAY_LONG_SEC = 15*LIKE_DELAY_SHORT_SEC; // 15 minute
const i64 MIN_MICRO_SEC = 1e15;
const i64 MIN_MILLI_SEC = 1e12;

constexpr TStringBuf PLAYER_DISLIKE_COMMAND = "player_dislike";
constexpr TStringBuf PLAYER_LIKE_COMMAND = "player_like";

const TSet<TStringBuf> AUTHORIZED_COMMANDS = {
    PLAYER_DISLIKE_COMMAND,
    PLAYER_LIKE_COMMAND,
};

bool IsMusicPlayerWithBiometry(TContext& ctx) {
    if (!ctx.IsAuthorizedUser() || !ctx.MetaClientInfo().IsSmartSpeaker()) {
        return false;
    }
    if (!ctx.HasExpFlag("music_biometry") || !ctx.HasExpFlag("biometry_like")) {
        return false;
    }
    if (const auto error = NAlice::NBiometry::IsValidBiometricsContext(ctx.ClientFeatures(), ctx.Meta())) {
        LOG(ERR) << error->ToJson() << Endl;
        return false;
    }
    if (!NAlice::NBiometry::HasNonEmptyBiometricsScores(ctx.Meta())) {
        return false;
    }
    NSc::TValue lastWatched;
    TStringBuf player = NPlayerCommand::SelectPlayer(ctx, &lastWatched);
    return player == NPlayerCommand::MUSIC_PLAYER;
}

 /** Music timestamp can contain value in seconds, milliseconds or microseconds, without any reference to the actual unit.
  * Try to find the unit empirically and extract its value in second.
  * TODO: Update the function/ALICE-4119
  */

i32 GetDiffTimeStamp(TContext& ctx, i64 musicTimestamp) {
    i32 now = ctx.GetRequestStartTime().Seconds();
    if (musicTimestamp >= MIN_MICRO_SEC)
        musicTimestamp /= 1000000;
    else if (musicTimestamp >= MIN_MILLI_SEC)
        musicTimestamp /= 1000;
    return now - musicTimestamp;
}

bool isMusicCanLike(TContext& ctx) {
    const auto& deviceState = ctx.Meta().DeviceState();
    i32 difference = GetDiffTimeStamp(ctx, deviceState.Music().Player().GetRawValue()->Get("timestamp").GetNumber(0));
    if (deviceState.IsTvPluggedIn()) {
        if (NVideo::GetCurrentScreen(ctx) == NVideo::EScreenId::MusicPlayer) {
            return difference < LIKE_DELAY_LONG_SEC;
        }
        return false;
    }
    if (!NPlayer::IsRadioPaused(ctx)) {
        return false;
    }
    return difference < LIKE_DELAY_SHORT_SEC;
}

void AddDislikeDirectives(TContext& ctx, NSc::TValue data) {
    ctx.AddCommand<TPlayerDislikeDirective>(PLAYER_DISLIKE_COMMAND, std::move(data));
}

void AddLikeDirectives(TContext& ctx, NSc::TValue data) {
    ctx.AddCommand<TPlayerLikeDirective>(PLAYER_LIKE_COMMAND, std::move(data));
}

TResultValue AddLikeDislikeCommand(TContext& ctx, TStringBuf command, NSc::TValue data,
                                   TStringBuf userId = TStringBuf("")) {
    if (command == PLAYER_DISLIKE_COMMAND) {
        AddDislikeDirectives(ctx, std::move(data));
    } else if (command == PLAYER_LIKE_COMMAND) {
        bool isNotPlayingAudio = NPlayer::AreAudioPlayersPaused(ctx);
        TContext::TSlot* slotResult = ctx.GetSlot(TStringBuf("music_context"), TStringBuf("music_result"));
        if (isNotPlayingAudio) {
            if (!IsSlotEmpty(slotResult)) {
                return NMusic::TBaseMusicProvider::SendLike(ctx, slotResult->Value, userId);
            } else if (isMusicCanLike(ctx)) {
                AddLikeDirectives(ctx, std::move(data));
            } else {
                ctx.AddAttention(TStringBuf("unknown_music"));
            }
            return TResultValue();
        } else {
            AddLikeDirectives(ctx, std::move(data));
        }
    } else {
        ctx.AddCommand<TPlayerUnknownCommandDirective>(command, std::move(data));
    }
    return ResultSuccess();
}

TResultValue HandleMusicBiometry(TContext& ctx, TStringBuf command, TBlackBoxAPI& blackBoxAPI,
                                 TDataSyncAPI& dataSyncAPI) {
    using namespace NAlice::NBiometry;

    NSc::TValue data;

    TBiometryDelegate biometryDelegate{ctx, blackBoxAPI, dataSyncAPI};
    TBiometry biometry{ctx.Meta(), biometryDelegate, TBiometry::EMode::HighTPR};
    TString userId;
    if (const auto err = biometry.GetUserId(userId)) {
        return TError{TError::EType::BIOMETRY, err->Msg};
    }
    data["uid"] = userId;
    if (biometry.IsGuestUser()) {
        TBiometry noguestBiometry{ctx.Meta(), biometryDelegate, TBiometry::EMode::NoGuest};
        ctx.AddAttention(ATTENTION_BIOMETRY_GUEST);
        TString userName;
        if (const auto err = noguestBiometry.GetUserName(userName)) {
            LOG(ERR) << "Error getting user name " << err->ToJson() << Endl;
        } else {
            auto slot = ctx.GetOrCreateSlot("user_name", "string");
            if (slot) {
                slot->Value.SetString(userName);
            } else {
                LOG(ERR) << "user_name slot is null" << Endl;
            }
        }
        if (command == TStringBuf("player_dislike") && !ctx.Meta().DeviceState().Music().Player().IsNull()) {
            ctx.AddAttention(ATTENTION_CHANGED_DISLIKE_TO_NEXT_TRACK);
            ctx.AddCommand<TPlayerNextTrackDirective>("player_next_track", std::move(data));
        }
        return ResultSuccess();
    }
    return AddLikeDislikeCommand(ctx, command, std::move(data), userId);
}

} // end namespace

TPlayerAuthorizedCommandHandler::TPlayerAuthorizedCommandHandler(THolder<TBlackBoxAPI> blackBoxAPI,
                                                                 THolder<TDataSyncAPI> dataSyncAPI)
    : BlackBoxAPI(std::move(blackBoxAPI))
    , DataSyncAPI(std::move(dataSyncAPI)) {
}

TResultValue TPlayerAuthorizedCommandHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    NPlayerCommand::SetPlayerCommandProductScenario(ctx);
    TStringBuf formName = ctx.FormName();
    TStringBuf command;

    if (formName.AfterPrefix(TStringBuf("personal_assistant.scenarios."), command)) {
        if (ctx.MetaClientInfo().IsYaAuto()) {
            return NAutomotive::HandleMediaControl(ctx, command);
        }

        if (!NPlayerCommand::AssertPlayerCommandIsSupported(command, ctx)) {
            return TResultValue();
        }

        if (!ctx.ClientFeatures().SupportsAnyPlayer() ||
            !AUTHORIZED_COMMANDS.contains(command) ||
            ctx.MetaClientInfo().IsLegatus())
        {
            TStringBuilder errStr;
            errStr << "Unsupported player command: " << command;
            LOG(ERR) << errStr << Endl;
            TResultValue err = TError(TError::EType::NOTSUPPORTED, errStr);
            ctx.AddErrorBlock(*err, NSc::Null());
        } else {
            if (!NMusic::TryAddAuthorizationSuggest(ctx)) {
                if (NVideo::GetCurrentScreen(ctx) == NVideo::EScreenId::VideoPlayer &&
                    NVideo::GetCurrentVideoItemType(r.Ctx()) == NVideo::EItemType::TvStream) {
                    TTvChannelsHelper tvHelper(ctx);
                    TSchemeHolder<NVideo::TVideoItemScheme> holder = NVideo::GetCurrentVideoItem(ctx);
                    NVideo::TVideoItemConstScheme tvStreamItem = holder.Scheme();
                    ctx.AddStopListeningBlock();
                    return tvHelper.HandleUserReaction(tvStreamItem, command == TStringBuf("player_dislike")
                                                                         ? TStringBuf("Dislike")
                                                                         : TStringBuf("Like"));
                } else {
                    if (IsMusicPlayerWithBiometry(ctx)) {
                        return HandleMusicBiometry(ctx, command, *BlackBoxAPI, *DataSyncAPI);
                    }
                    return AddLikeDislikeCommand(ctx, command, {} /* data */);
                }
            }
        }
    }

    return TResultValue();
}

} // end namespace NBASS
