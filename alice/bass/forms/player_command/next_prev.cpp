#include <alice/bass/forms/automotive/media_control.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/player_command.h>
#include <alice/bass/forms/player_command/defs.h>
#include <alice/bass/forms/player_command/player_command.sc.h>
#include <alice/bass/forms/radio.h>
#include <alice/bass/forms/video/video.h>
#include <alice/bass/forms/video/player_command.h>
#include <alice/bass/forms/video/defs.h>
#include <alice/bass/forms/video/utils.h>
#include <alice/bass/forms/tv/tv_helper.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <util/string/cast.h>

namespace NBASS {

namespace {

constexpr TStringBuf NOTHING_IS_PLAYING_CODE = "nothing_is_playing";

} // namespace

using namespace NPlayerCommand;

IContinuation::TPtr TPlayerNextPrevCommandHandler::Prepare(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    SetPlayerCommandProductScenario(ctx);
    TStringBuf formName = ctx.FormName();
    TStringBuf command;
    const bool isMusicPlayerOnly = ctx.GetSlot(SLOT_NAME_MUSIC_PLAYER_ONLY, SLOT_TYPE_FLAG);
    if (isMusicPlayerOnly) {
        auto& block = *ctx.Block();
        block["type"] = "features_data";
        block["data"]["is_player_command"].SetBool(true);
    }
    formName.AfterPrefix(TStringBuf("personal_assistant.scenarios."), command);

    if (ctx.MetaClientInfo().IsYaAuto()) {
        return TCompletedContinuation::Make(ctx, NAutomotive::HandleMediaControl(ctx, command));
    }

    if (!ctx.ClientFeatures().SupportsAnyPlayer() || ctx.MetaClientInfo().IsLegatus()) {
        ctx.AddErrorBlock(TError(TError::EType::NOTSUPPORTED, TStringBuf("unsupported_operation")), NSc::Null());
        return TCompletedContinuation::Make(ctx);
    }

    if (!AssertPlayerCommandIsSupported(command, ctx)) {
        return TCompletedContinuation::Make(ctx);
    }

    NSc::TValue output;
    NSc::TValue lastWatched;

    const auto& multiroomSessionId = ctx.Meta().DeviceState().Multiroom().MultiroomSessionId();
    if (!multiroomSessionId.IsNull()) {
        output["multiroom_session_id"] = multiroomSessionId.GetRawValue()->GetString();
    }

    TMaybe<NVideo::EScreenId> currentScreen = NVideo::GetCurrentScreen(ctx);
    TStringBuf player = SelectPlayer(ctx, &lastWatched);

    if (currentScreen == NVideo::EScreenId::Main || currentScreen == NVideo::EScreenId::MordoviaMain) {
        if (isMusicPlayerOnly) {
            ctx.AddErrorBlockWithCode(TError{TError::EType::PLAYERERROR}, NOTHING_IS_PLAYING_CODE);
        }
        return TCompletedContinuation::Make(ctx, NVideo::AddAttention(ctx, NVideo::ATTENTION_NOTHING_IS_PLAYING));
    }

    bool isMusicCase = (player == MUSIC_PLAYER || player == BLUETOOTH_PLAYER);
    if (isMusicPlayerOnly && !isMusicCase) {
        ctx.AddErrorBlockWithCode(TError{TError::EType::MUSICERROR}, IRRELEVANT_PLAYER_CODE);
        return TCompletedContinuation::Make(ctx);
    }

    if (isMusicCase) {
        output["player"].SetString(player);
        if (command == TStringBuf("player_next_track")) {
            ctx.AddCommand<TPlayerNextTrackDirective>(command, std::move(output));
        } else if (command == TStringBuf("player_previous_track")) {
            ctx.AddCommand<TPlayerPrevTrackDirective>(command, std::move(output));
        } else {
            ctx.AddCommand<TPlayerUnknownCommandDirective>(command, std::move(output));
        }
        return TCompletedContinuation::Make(ctx);

    } else if (player == VIDEO_PLAYER) {
        // TODO: think about all screens
        bool isNext = command == NPlayerCommand::PLAYER_NEXT_TRACK;
        if (currentScreen == NVideo::EScreenId::VideoPlayer) {
            if (NVideo::GetCurrentVideoItemType(r.Ctx()) == NVideo::EItemType::TvStream) {
                TTvChannelsHelper tvHelper(ctx);
                TSchemeHolder<NVideo::TVideoItemScheme> holder = NVideo::GetCurrentVideoItem(ctx);
                NVideo::TVideoItemConstScheme tvStreamItem = holder.Scheme();
                if (tvStreamItem.TvStreamInfo().IsPersonal() && isNext) {
                    return TCompletedContinuation::Make(ctx, tvHelper.HandleUserReaction(tvStreamItem, "Skip"));
                    //TODO: Implement going to PREV personal TV episode someday?
                }
            }
            return isNext ? PreparePlayNextVideo(ctx) : PreparePlayPreviousVideo(ctx);
        }
        // FIXME: for other screens
        return TCompletedContinuation::Make(ctx, NVideo::AddAttention(ctx, isNext ? NVideo::ATTENTION_NO_NEXT_VIDEO
                                                                                  : NVideo::ATTENTION_NO_PREV_VIDEO));

    } else if (player == RADIO_PLAYER) {
        const NSc::TValue* radioState = ctx.Meta().DeviceState().Radio().GetRawValue();
        TContext::TPtr newContext = TRadioFormHandler::SetAsResponse(ctx, false);

        const auto lastPlayTimestamp = radioState->Get("last_play_timestamp").GetNumber(0);
        newContext->AddPlayerFeaturesBlock(/* restorePlayer= */ true, lastPlayTimestamp);

        TString radioId = ToString(TRadioFormHandler::GetCurrentlyPlayingRadioId(radioState));
        NRadio::ESelectionMethod selectionMethod = (command == NPlayerCommand::PLAYER_NEXT_TRACK)
                                                       ? NRadio::ESelectionMethod::Next
                                                       : NRadio::ESelectionMethod::Previous;
        return TCompletedContinuation::Make(
            ctx, TRadioFormHandler::HandleRadioStream(*newContext, radioId, selectionMethod));
    }

    // TODO: For Payment - add attention
    return TCompletedContinuation::Make(ctx);
}

}
