#include <alice/bass/forms/automotive/media_control.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/player/player.h>
#include <alice/bass/forms/player_command.h>
#include <alice/bass/forms/player_command/defs.h>
#include <alice/bass/forms/player_command/player_command.sc.h>

#include <alice/bass/libs/config/config.h>

#include <alice/bass/forms/music/music.h>
#include <alice/bass/forms/music/providers.h>
#include <alice/bass/forms/radio.h>

#include <alice/bass/forms/video/video.h>
#include <alice/bass/forms/video/defs.h>
#include <alice/bass/forms/video/player_command.h>
#include <alice/bass/forms/video/video_slots.h>
#include <alice/bass/forms/video/utils.h>

#include <alice/bass/forms/radio.h>

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/client/experimental_flags.h>

#include <alice/library/video_common/defs.h>
#include <alice/library/music/defs.h>

namespace NBASS {

namespace NPlayerCommand {

namespace {

i64 GetTimestamp(const NSc::TValue& playerInfo) {
    return playerInfo.Get("timestamp").GetNumber(0);
}

TStringBuf SelectByTimestamp(TContext& ctx, const NSc::TValue& lastWatchedVideo = NSc::Null()) {
    const auto& deviceState = ctx.Meta().DeviceState();

    TVector<std::pair<i64, TStringBuf>> variants = {
        {GetTimestamp(*deviceState.Music().Player().GetRawValue()), MUSIC_PLAYER},
        {GetTimestamp(*deviceState.Radio().Player().GetRawValue()), RADIO_PLAYER},
        {GetTimestamp(*deviceState.Bluetooth().Player().GetRawValue()), BLUETOOTH_PLAYER},
    };

    if (deviceState.IsTvPluggedIn() && lastWatchedVideo.IsDict()) {
        variants.push_back({GetTimestamp(lastWatchedVideo), VIDEO_PLAYER});
    }

    size_t maxId = 0;
    for (size_t i = 1; i < variants.size(); ++i) {
        if (variants[i].first > variants[maxId].first) {
            maxId = i;
        }
    }

    return variants[maxId].second;
}

}

TStringBuf SelectPlayer(TContext& ctx, NSc::TValue* lastWatchedVideo) {
    const TContext::TSlot* slot = ctx.GetSlot(TStringBuf("player_type"));
    if (!ctx.ClientFeatures().SupportsVideoPlayer() || !ctx.Meta().DeviceState().IsTvPluggedIn()) {
        if (!NPlayer::IsMusicPaused(ctx)) {
            return MUSIC_PLAYER;
        }
        if (!NPlayer::IsRadioPaused(ctx)) {
            return RADIO_PLAYER;
        }
        if (!NPlayer::IsBluetoothPlayerPaused(ctx)) {
            return BLUETOOTH_PLAYER;
        }
        if (!IsSlotEmpty(slot) && slot->Value.GetString() == VIDEO_PLAYER) {
            if (ctx.ClientFeatures().IsMiniSpeaker()) {
                ctx.AddAttention(TStringBuf("video_play_not_supported_on_device"), {});
            }
            if (ctx.ClientFeatures().IsQuasar()) {
                ctx.AddAttention(TStringBuf("video_play_is_off_on_device"), {});
            }
            return STOP_PLAYER;
        }

        return SelectByTimestamp(ctx);
    }
    TMaybe<NVideo::EScreenId> currentScreen = NVideo::GetCurrentScreen(ctx);
    if (!IsSlotEmpty(slot)) {
        TStringBuf player = slot->Value.GetString();
        if (player == VIDEO_PLAYER) {
            if (currentScreen != NVideo::EScreenId::VideoPlayer) {
                (*lastWatchedVideo) = NVideo::FindLastWatchedItem(ctx);
            }
            return VIDEO_PLAYER;
        }
        if (player == RADIO_PLAYER) {
            return RADIO_PLAYER;
        }
        return MUSIC_PLAYER;
    }

    if (currentScreen == NVideo::EScreenId::VideoPlayer) {
        return VIDEO_PLAYER;
    }

    if (currentScreen == NVideo::EScreenId::Description ||
        currentScreen == NVideo::EScreenId::ContentDetails ||
        currentScreen == NVideo::EScreenId::WebViewVideoEntity ||
        currentScreen == NVideo::EScreenId::WebviewVideoEntityWithCarousel ||
        currentScreen == NVideo::EScreenId::WebviewVideoEntityDescription ||
        currentScreen == NVideo::EScreenId::WebviewVideoEntitySeasons ||
        currentScreen == NVideo::EScreenId::SeasonGallery ||
        currentScreen == NVideo::EScreenId::WebViewChannels ||
        currentScreen == NVideo::EScreenId::Gallery ||
        currentScreen == NVideo::EScreenId::WebViewFilmsSearchGallery ||
        currentScreen == NVideo::EScreenId::WebViewVideoSearchGallery ||
        currentScreen == NVideo::EScreenId::WebviewVideoEntityRelated ||
        currentScreen == NVideo::EScreenId::TvExpandedCollection ||
        currentScreen == NVideo::EScreenId::SearchResults)
    {
        (*lastWatchedVideo) = NVideo::FindLastWatchedItem(ctx);
        return VIDEO_PLAYER;
    }

    if (currentScreen == NVideo::EScreenId::MusicPlayer) {
        return MUSIC_PLAYER;
    }

    if (currentScreen == NVideo::EScreenId::RadioPlayer) {
        return RADIO_PLAYER;
    }

    if (currentScreen == NVideo::EScreenId::Bluetooth) {
        return BLUETOOTH_PLAYER;
    }

    if (currentScreen == NVideo::EScreenId::Main || currentScreen == NVideo::EScreenId::MordoviaMain || currentScreen == NVideo::EScreenId::TvMain) {
        (*lastWatchedVideo) = NVideo::FindLastWatchedItem(ctx);
        return SelectByTimestamp(ctx, *lastWatchedVideo);
    }

    // For Screen = Payment:
    return TStringBuf("");
}

} // end namespace

using namespace NPlayerCommand;

TResultValue TPlayerContinueCommandHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    SetPlayerCommandProductScenario(ctx);
    if (ctx.MetaClientInfo().IsYaAuto()) {
        return NAutomotive::HandleMediaControl(ctx, TStringBuf("player_continue"));
    }
    if (!ctx.ClientFeatures().SupportsPlayerContinueDirective()) {
        r.Ctx().AddErrorBlock(
            TError(TError::EType::NOTSUPPORTED, TStringBuf("unsupported_operation")),
            NSc::Null()
        );
        return TResultValue();
    }

    if (!AssertPlayerCommandIsSupported(TStringBuf("player_continue"), ctx)) {
        return TResultValue();
    }

    const bool isMusicPlayerOnly = ctx.GetSlot(SLOT_NAME_MUSIC_PLAYER_ONLY, SLOT_TYPE_FLAG);
    if (isMusicPlayerOnly) {
        auto& block = *ctx.Block();
        block["type"] = "features_data";
        block["data"]["is_player_command"].SetBool(true);
    }

    NSc::TValue output;
    NSc::TValue lastWatched;

    TMaybe<NVideo::EScreenId> currentScreen = NVideo::GetCurrentScreen(ctx);
    TStringBuf player = SelectPlayer(ctx, &lastWatched);

    // XXX(vitvlkv): Yes, this looks weird, but bluetooth player is kinda 'isMusicPlayerOnly' too...
    if (isMusicPlayerOnly && (player != MUSIC_PLAYER && player != BLUETOOTH_PLAYER)) {
        ctx.AddErrorBlockWithCode(TError{TError::EType::MUSICERROR}, IRRELEVANT_PLAYER_CODE);
        return TResultValue();
    }

    if (player == STOP_PLAYER) {
        return TResultValue();
    }

    if (player == MUSIC_PLAYER) {
        const NSc::TValue* musicState = ctx.Meta().DeviceState().Music().CurrentlyPlaying().GetRawValue();
        // Switch to music_play if no music state
        if (musicState->IsNull() || musicState->Get("track_id").ForceString().empty()) {
            NMusic::TSearchMusicHandler::SetAsResponse(ctx, /* callbackSlot */ false);
            return ctx.RunResponseFormHandler();
        }
        output["player"].SetString("music");
        ctx.AddCommand<TMusicPlayerContinueDirective>(TStringBuf("player_continue"), std::move(output));
        return TResultValue();
    }

    if (player == BLUETOOTH_PLAYER) {
        output["player"].SetString("bluetooth");
        ctx.AddCommand<TBluetoothPlayerContinueDirective>(TStringBuf("player_continue"), std::move(output));
        return TResultValue();
    }

    if (player == VIDEO_PLAYER) {
        if (currentScreen == NVideo::EScreenId::VideoPlayer) {
            if (NVideo::GetCurrentVideoItemType(r.Ctx()) == NVideo::EItemType::TvStream) {
                TOpenCurrentVideoHandler::SetAsResponse(ctx, false);
                return ctx.RunResponseFormHandler();
            }
            output["player"].SetString("video");
            ctx.AddCommand<TVideoPlayerContinueDirective>(TStringBuf("player_continue"), std::move(output));
            return TResultValue();
        }
        // Hack for some cases like "Играй"/"Включи" when we get "player_continue" intent instead of "video_selection_action"
        if (currentScreen == NVideo::EScreenId::Description ||
            currentScreen == NVideo::EScreenId::ContentDetails ||
            currentScreen == NVideo::EScreenId::WebViewVideoEntity ||
            currentScreen == NVideo::EScreenId::WebviewVideoEntityWithCarousel ||
            currentScreen == NVideo::EScreenId::WebviewVideoEntityDescription ||
            currentScreen == NVideo::EScreenId::SeasonGallery ||
            currentScreen == NVideo::EScreenId::WebviewVideoEntitySeasons)
        {
            if (ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_DISABLE_VINS_CONTINUE_FOR_VIDEO_SCREENS)) {
                ctx.AddIrrelevantAttention(
                        /* relevantIntent= */ NAlice::NVideoCommon::QUASAR_OPEN_CURRENT_VIDEO,
                        /* reason= */ TStringBuf("https://st.yandex-team.ru/VIDEOFUNC-534"));
                return {};
            }
            TContext::TPtr newCtx = TOpenCurrentVideoHandler::SetAsResponse(ctx, /* callbackSlot */ false);
            newCtx->CreateSlot(TStringBuf("action"),
                               TStringBuf("video_selection_action"),
                               true, TStringBuf("play"));
            return ctx.RunResponseFormHandler();
        }

        if (lastWatched.Has("episode") || lastWatched.Has("season")) {
            ctx.CreateSlot(TStringBuf("season"), TStringBuf("num"), true, lastWatched.Delete("season").GetIntNumber());
            ctx.CreateSlot(TStringBuf("episode"), TStringBuf("num"), true, lastWatched.Delete("episode").GetIntNumber());
        }

        return ContinueLastWatchedVideo(NVideo::TVideoItemConstScheme(&lastWatched), ctx);
    }

    if (player == RADIO_PLAYER) {
        const NSc::TValue* radioState = ctx.Meta().DeviceState().Radio().GetRawValue();
        TContext::TPtr newContext = TRadioFormHandler::SetAsResponse(ctx, /* callbackSlot */ false);

        const auto lastPlayTimestamp = radioState->Get("last_play_timestamp").GetNumber(0);
        newContext->AddPlayerFeaturesBlock(/* restorePlayer= */ true, lastPlayTimestamp);

        TStringBuf currentRadioId = TRadioFormHandler::GetCurrentlyPlayingRadioId(radioState);
        return TRadioFormHandler::HandleRadioStream(*newContext, currentRadioId, currentRadioId.Empty() ? NRadio::ESelectionMethod::Any : NRadio::ESelectionMethod::Current);
    }

    // TODO: For Payment
    return TResultValue();
}

}
