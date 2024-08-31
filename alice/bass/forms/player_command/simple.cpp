#include <alice/bass/forms/automotive/media_control.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/music/providers.h>
#include <alice/bass/forms/player/player.h>
#include <alice/bass/forms/player_command.h>

#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS {

namespace {

const auto CLEAR_QUEUE_DIRECTIVE_TYPE_INDEX = GetAnalyticsTagIndex<TClearQueueDirective>();

const THashMap<TStringBuf, TVector<TDirectiveFactory::TDirectiveIndex>> ALL_PLAYER_COMMANDS = {
    { TStringBuf("player_pause"), {GetAnalyticsTagIndex<TPlayerPauseDirective>(),
                                   GetAnalyticsTagIndex<TClearQueueDirective>()} },
    { TStringBuf("player_order"), {GetAnalyticsTagIndex<TPlayerOrderDirective>()} },
    { TStringBuf("player_shuffle"), {GetAnalyticsTagIndex<TPlayerShuffleDirective>()} },
    { TStringBuf("player_repeat"), {GetAnalyticsTagIndex<TPlayerRepeatDirective>()} },
    { TStringBuf("player_replay"), {GetAnalyticsTagIndex<TPlayerReplayDirective>()} },
    // old commands (for compatibility with auto)
    { TStringBuf("music_pause"), {GetAnalyticsTagIndex<TMusicPlayerPauseDirective>()} },
    { TStringBuf("music_continue"), {GetAnalyticsTagIndex<TMusicPlayerContinueDirective>()} }
};

const TStringBuf REPEAT = "repeat";
const TStringBuf THIS_FORM_IS_CALLBACK = "this_form_is_callback";

// for compatibility with auto
TError::EType SelectErrorType(TStringBuf command) {
    return command.StartsWith("music_") ? TError::EType::MUSICERROR : TError::EType::NOTSUPPORTED;
}

} // end namespace

TResultValue TPlayerSimpleCommandHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    NPlayerCommand::SetPlayerCommandProductScenario(ctx);
    TStringBuf formName = ctx.FormName();
    TStringBuf command;

    const bool isMusicPlayerOnly = ctx.GetSlot(NPlayerCommand::SLOT_NAME_MUSIC_PLAYER_ONLY,
                                               NPlayerCommand::SLOT_TYPE_FLAG);
    if (isMusicPlayerOnly) {
        auto& block = *ctx.Block();
        block["type"] = "features_data";
        block["data"]["is_player_command"].SetBool(true);
    }

    const auto isCallbackFormSlot = ctx.GetSlot(THIS_FORM_IS_CALLBACK);
    if (!IsSlotEmpty(isCallbackFormSlot) && isCallbackFormSlot->Value.GetBool()) {
        ctx.DeleteSlot(THIS_FORM_IS_CALLBACK);
        ctx.DeleteSlot(TStringBuf("callback_form"));
        return TResultValue();
    }

    if (formName.AfterPrefix(TStringBuf("personal_assistant.scenarios."), command)) {
        if (ctx.MetaClientInfo().IsYaAuto()) {
            return NAutomotive::HandleMediaControl(ctx, command);
        }

        if (!NPlayerCommand::AssertPlayerCommandIsSupported(command, ctx)) {
            return TResultValue();
        }

        if (command == TStringBuf("player_repeat") && ctx.ClientFeatures().IsSmartSpeaker() && !NPlayer::IsMusicPaused(ctx)) {
            TContext::TPtr newCtx = ctx.SetResponseForm(NMusic::MUSIC_PLAY_ANAPHORA_FORM_NAME, true /* setCurrentFormAsCallback */);
            Y_ENSURE(newCtx);
            newCtx->CreateSlot(REPEAT, REPEAT, true, REPEAT);
            newCtx->CreateSlot(TStringBuf("target_type"), TStringBuf("string"), true, TStringBuf("track"));
            return ctx.RunResponseFormHandler();
        }

        NSc::TValue output;
        if (TResultValue err = ExecuteCommand(ctx, command, &output)) {
            if (err->Type == TError::EType::MUSICERROR && err->Msg == TStringBuf("unsupported_operation")) {
                NSc::TValue errorData;
                errorData["code"].SetString("unsupported_operation");
                r.Ctx().AddErrorBlock(*err, std::move(errorData));
            } else if (err->Type == TError::EType::NOTSUPPORTED) {
                r.Ctx().AddErrorBlock(*err, NSc::Null());
            } else {
                return *err;
            }
            return TResultValue();
        } else {
            if (output.Has("command")) {
                auto outputCommand = output.Get("command").GetString();
                const auto& directives = ALL_PLAYER_COMMANDS.at(outputCommand);
                for (const auto& directive : directives) {
                    if (directive == CLEAR_QUEUE_DIRECTIVE_TYPE_INDEX) {
                        if (ctx.ClientFeatures().SupportsDirectiveSequencer()) {
                            r.Ctx().AddCommand("clear_queue", directive, output["data"]);
                        }
                    } else {
                        r.Ctx().AddCommand(outputCommand, directive, output["data"]);
                    }
                }
                return TResultValue();
            }
            return TError(
                TError::EType::SYSTEM,
                TStringBuf("error during command execution")
            );
        }
    }

    return TError(
        TError::EType::NOTSUPPORTED,
        TStringBuf("not supported form name")
    );
}

TResultValue TPlayerSimpleCommandHandler::ExecuteCommand(TContext& ctx, TStringBuf command, NSc::TValue* output) {
    if (!ctx.ClientFeatures().SupportsAnyPlayer() || !ALL_PLAYER_COMMANDS.contains(command) || ctx.ClientFeatures().IsLegatus()) {
        TStringBuilder err;
        err << "Unsupported player command: " << command;
        LOG(ERR) << err << Endl;
        return TError(SelectErrorType(command), err);
    }

    (*output)["data"].SetDict();
    if (command == TStringBuf("player_repeat") && ctx.MetaClientInfo().IsSmartSpeaker()) {
        (*output)["command"].SetString("player_replay");
    } else {
        (*output)["command"].SetString(command);
    }

    return TResultValue();
}

}
