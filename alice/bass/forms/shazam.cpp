#include "shazam.h"
#include "directives.h"

#include "music/answers.h"
#include "music/music.h"
#include "music/providers.h"

#include "radio.h"
#include "player/player.h"

#include "urls_builder.h"

#include <util/string/builder.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/music/defs.h>
#include <alice/bass/forms/common/directives.h>

namespace NBASS {

namespace {

const TStringBuf ERROR_RECOGNIZE_ERROR = "recognize_error";
const TStringBuf ERROR_UNSUPPORTED_OPERATION = "unsupported_operation";
const TStringBuf ERROR_NOT_MUSIC = "not_music";
const TStringBuf ERROR_NOT_RECOGNIZED = "music_not_recognized";
const TStringBuf ERROR_TIMEOUT = "timeout";
const TStringBuf ERROR_INCORRECT_ANSWER = "recognize_incorrect_answer";
const TStringBuf ERROR_OTHER = "other";

const TStringBuf DIRECTIVE_START_RECOGNIZE = "start_music_recognizer";

const TStringBuf SUGGEST_RECOGNIZE_AGAIN = "music__recognise_again";

const TStringBuf PLAY_THIS_MUSIC = "play_this_music";

const TStringBuf SLOT_NAME_ANSWER = "answer";
const TStringBuf SLOT_NAME_ACTION_REQUEST = "action_request";

const TStringBuf SLOT_TYPE_MUSIC_RESULT = "music_result";
const TStringBuf SLOT_TYPE_NON_MUSIC_RESULT = "non_music_result";

NSc::TValue ConvertRecognizeError(TStringBuf status) {
    NSc::TValue err;
    if (status == TStringBuf("no-matches")) {
        err["code"].SetString(ERROR_NOT_RECOGNIZED);
    } else if (status == TStringBuf("not-music")) {
        err["code"].SetString(ERROR_NOT_MUSIC);
    } else if (status == TStringBuf("response-timeout")) {
        err["code"].SetString(ERROR_TIMEOUT);
    } else {
        err["code"].SetString(ERROR_OTHER);
    }
    return err;
}

template<typename TPlayerState>
NSc::TValue GetCurrentTrackInfo(const TPlayerState& playerState) {
    return *playerState.CurrentlyPlaying().TrackInfo().GetRawValue();
}

} // namespace

using namespace NMusic;

TResultValue TShazamHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::MUSIC_WHAT_IS_PLAYING);
    TContext::TSlot* slotResult = ctx.GetSlot(SLOT_NAME_ANSWER, SLOT_TYPE_MUSIC_RESULT);
    if (!IsSlotEmpty(slotResult)) {
        TContext::TPtr newCtx = TSearchMusicHandler::SetAsResponse(ctx, /* callbackSlot */ false);
        newCtx->CopySlotsFrom(ctx, { SLOT_NAME_ANSWER, SLOT_NAME_ACTION_REQUEST });
        return ctx.RunResponseFormHandler();
    }

    NSc::TValue trackInfo;
    bool realRecognizer = false;

    if (!NPlayer::IsRadioPaused(ctx)) {
        NSc::TValue result;
        result["radio"] = *ctx.Meta().DeviceState().Radio().CurrentlyPlaying().GetRawValue();
        ctx.CreateSlot(SLOT_NAME_ANSWER, SLOT_TYPE_NON_MUSIC_RESULT, /* optional */ true, result);
        if (ctx.MetaClientInfo().IsSmartSpeaker()) {
            ctx.AddStopListeningBlock();
        }

        const NSc::TValue* radioState = ctx.Meta().DeviceState().Radio().GetRawValue();
        const auto lastPlayTimestamp = radioState->Get("last_play_timestamp").GetNumber(0);

        ctx.AddPlayerFeaturesBlock(/* restorePlayer = */ true, lastPlayTimestamp);
        return TResultValue();
    }

    if (!NPlayer::IsMusicPaused(ctx)) {
        const auto& musicPlayer = ctx.Meta().DeviceState().Music();
        trackInfo = GetCurrentTrackInfo(musicPlayer);

        ctx.AddPlayerFeaturesBlock(/* restorePlayer = */ true, *musicPlayer.LastPlayTimestamp());
    } else if (!NPlayer::IsBluetoothPlayerPaused(ctx)) {
        const auto& bluetoothPlayer = ctx.Meta().DeviceState().Bluetooth();
        trackInfo = GetCurrentTrackInfo(bluetoothPlayer);

        if (trackInfo.IsNull()) {
            NSc::TValue result;
            result["is_bluetooth_playing"] = true;
            ctx.CreateSlot(SLOT_NAME_ANSWER, SLOT_TYPE_NON_MUSIC_RESULT, /* optional */ true, result);
            if (ctx.MetaClientInfo().IsSmartSpeaker()) {
                ctx.AddStopListeningBlock();
            }
            return TResultValue();
        }

        ctx.AddPlayerFeaturesBlock(/* restorePlayer = */ true, *bluetoothPlayer.LastPlayTimestamp());
    } else {
        if (!ctx.ClientFeatures().SupportsMusicRecognizer()) {
            NSc::TValue errorData;
            TStringBuf errorMsg;
            if (ctx.MetaClientInfo().IsSmartSpeaker() && !ctx.ClientFeatures().SupportsNoMicrophone()) {
                errorData["code"].SetString(ERROR_NOT_MUSIC);
                errorMsg = ERROR_RECOGNIZE_ERROR;
            } else {
                errorData["code"].SetString(ERROR_UNSUPPORTED_OPERATION);
                errorMsg = ERROR_UNSUPPORTED_OPERATION;
                TryAddShowPromoDirective(ctx);
            }
            ctx.AddErrorBlock(
                TError(TError::EType::MUSICERROR, errorMsg),
                std::move(errorData)
            );
            return TResultValue();
        }

        auto utteranceData = ctx.Meta().UtteranceData();
        if (!utteranceData->Has("result")) {
            // Start recognizing
            ctx.AddCommand<TMusicRecognizerDirective>(DIRECTIVE_START_RECOGNIZE, NSc::Null());
            return TResultValue();
        }

        ctx.AddSuggest(SUGGEST_RECOGNIZE_AGAIN);

        TStringBuf status = utteranceData->Get("result").GetString();
        if (status != TStringBuf("success")) {
            NSc::TValue err = ConvertRecognizeError(status);
            ctx.AddErrorBlock(
                TError(TError::EType::MUSICERROR, ERROR_RECOGNIZE_ERROR),
                std::move(err)
            );

            return TResultValue();
        }

        ctx.AddSuggest(PLAY_THIS_MUSIC);

        trackInfo = utteranceData->TrySelect("data/match");
        realRecognizer = true;
    }

    if (trackInfo.IsNull()) {
        ctx.AddErrorBlock(TError(TError::EType::MUSICERROR, ERROR_INCORRECT_ANSWER));
        return TResultValue();
    }

    TYandexMusicAnswer answer(ctx.ClientFeatures());
    answer.InitWithShazamAnswer("track", trackInfo, /* autoplay */ true);

    NSc::TValue out;
    if (!answer.ConvertAnswerToOutputFormat(&out)) {
        ctx.AddErrorBlock(TError(TError::EType::MUSICERROR, ERROR_INCORRECT_ANSWER));
        return TResultValue();
    }

    if (ctx.ClientFeatures().SupportsDivCards()) {
        TStringBuf coverUri = out.Get("coverUri").GetString();
        if (!coverUri.empty()) {
            NSc::TValue card;
            card["coverUri"].SetString(coverUri);
            card["log_id"].SetString("music_recognizer");
            card["play_inside_app"].SetBool(ctx.ClientFeatures().SupportsMusicSDKPlayer());

            // UGC-content or track is not available on Yandex.Music
            if (!trackInfo["available"].GetBool(false)) {
                card["not_available"].SetBool(true);
                TStringBuilder request;
                request << out["title"].GetString();
                for (const auto& artist : out["artists"].GetArray()) {
                    request << " " << artist["name"].GetString();
                }
                card["search_url"].SetString(GenerateSearchUri(&ctx, request));
            }

            if (!card["not_available"].GetBool(false) && card["play_inside_app"].GetBool(false)) {
                card["playUri"].SetString(
                    NMusic::TBaseMusicAnswer::MakeDeeplink(ctx.ClientFeatures(), NSc::TValue().AddAll({
                        {"type", "track"},
                        {"id", NSc::TValue().AddAll({
                            {"id", out.Get("id").GetString()}
                        })},
                        {"playerSettings", NSc::TValue().AddAll({
                            {"shuffle", false},
                            {"repeat", "repeatOff"}
                        })}
                    }))
                );
            }

            ctx.AddDivCardBlock("music__track", std::move(card));
        }
    } else if (ctx.MetaClientInfo().IsYaMusic() && realRecognizer) {
        NSc::TValue commandData;
        commandData["uri"].SetString(TYandexMusicAnswer::MakeLinkToTrackForShazam(ctx.ClientFeatures(), out));
        ctx.AddCommand<TYaMusicOpenTrackFromShazamDirective>(TStringBuf("open_uri"), std::move(commandData));
        ctx.AddCommand<TYaMusicRecognitionDirective>(TStringBuf("music_recognition"), out);
    }

    ctx.CreateSlot(SLOT_NAME_ANSWER, SLOT_TYPE_MUSIC_RESULT, /* optional */ true, std::move(out));
    if (ctx.MetaClientInfo().IsSmartSpeaker()) {
        ctx.AddStopListeningBlock();
    }
    return TResultValue();
}

void TShazamHandler::Register(THandlersMap *handlers) {
    auto handler = []() {
        return MakeHolder<TShazamHandler>();
    };
    handlers->emplace(TStringBuf("personal_assistant.scenarios.music_what_is_playing"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.music_what_is_playing__ellipsis"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.music_what_is_playing__play"), handler);
}

}
