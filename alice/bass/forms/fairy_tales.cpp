#include "fairy_tales.h"

#include <alice/bass/forms/music/music.h>

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/music/defs.h>

namespace NBASS {

namespace {

const TStringBuf FAIRY_TALES_FORM_NAME = "personal_assistant.scenarios.music_fairy_tale";

const TStringBuf STRING_TYPE = "string";
const TStringBuf DEFAULT_GENRE = "fairytales";
const TStringBuf DEFAULT_PLAYLIST = "970829816:1039";
const TStringBuf FAIRY_TALE = "fairy_tale";

const TStringBuf ATTENTION_UNKNOWN_FAIRY_TALE = "unknown_fairy_tale";

} // namespace

TResultValue TFairyTalesHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::MUSIC_FAIRY_TALE);
    if (ctx.MetaClientInfo().IsElariWatch()) {
        ctx.AddErrorBlockWithCode(
            TError(TError::EType::NOTSUPPORTED),
            TStringBuf("this_device_is_not_supporting_fairy_tales")
        );
        return TResultValue();
    }

    bool nothingFound = false;
    const bool shouldFallbackIfNothingFound = !ctx.HasExpFlag("fairytale_no_fallback");
    TContext::TSlot* slotFormIsCallback = ctx.GetSlot("this_form_is_callback");
    if (!IsSlotEmpty(slotFormIsCallback)) {
        ctx.DeleteSlot("callback_form");
        ctx.DeleteSlot("order");
        ctx.DeleteSlot("album_id");
        ctx.DeleteSlot("track_id");
        ctx.DeleteSlot("this_form_is_callback");

        const bool wasDefaultPlaylistRequest = !IsSlotEmpty(ctx.GetSlot("playlist_id"));
        if (IsSlotEmpty(ctx.GetSlot("answer")) && shouldFallbackIfNothingFound && !wasDefaultPlaylistRequest) {
            nothingFound = true;
        } else {
            ctx.DeleteSlot("playlist_id");
            return TResultValue();
        }
    }

    ctx.DeleteSlot("answer");

    TContext::TPtr newCtx = NMusic::TSearchMusicHandler::SetAsResponse(ctx, /* callbackSlot */ true);
    TContext::TSlot* slotFairyTale = ctx.GetSlot(FAIRY_TALE);

    if (IsSlotEmpty(slotFairyTale) || nothingFound) {
        if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_FAIRYTALE_RADIO)) {
            newCtx->CreateSlot(TStringBuf("genre"), STRING_TYPE, /* optional */ true, DEFAULT_GENRE);
        } else {
            newCtx->AddAttention("is_general_playlist");
            newCtx->CreateSlot(TStringBuf("playlist_id"), STRING_TYPE, /* optional */ true, DEFAULT_PLAYLIST);
            newCtx->CreateSlot(TStringBuf("order"), STRING_TYPE, /* optional */ true, TStringBuf("shuffle"));
        }

        const bool isUnknownFairyTale = !IsSlotEmpty(slotFairyTale) && slotFairyTale->Type == STRING_TYPE;
        if (isUnknownFairyTale) {
            NSc::TValue data;
            data["text"] = slotFairyTale->Value;
            newCtx->AddAttention(ATTENTION_UNKNOWN_FAIRY_TALE, data);
        }
    } else {
        // VINS and Hollywood music send search text in different slot fields: VINS uses SourceText, while HW music uses slot value
        const auto fairyTaleSourceText = !slotFairyTale->SourceText.IsNull()
            ? slotFairyTale->SourceText.GetString()
            : slotFairyTale->Value.GetString();
        const auto searchText = NSc::TValue(TString("сказка ") + fairyTaleSourceText);

        newCtx->CreateSlot(TStringBuf("search_text"), STRING_TYPE, /* optional */ true, searchText, searchText);
    }

    newCtx->CreateSlot(TStringBuf("action_request"), STRING_TYPE, /* optional */ true, /* value */ TStringBuf("autoplay"));
    newCtx->CreateSlot(TStringBuf("is_fairy_tale_filter_genre"), TStringBuf("bool"), /* optional */ true, /* value */ true);

    TContext::TSlot* slotOffset = ctx.GetSlot(NAlice::NMusic::SLOT_OFFSET);
    if (!IsSlotEmpty(slotOffset)) {
        newCtx->CreateSlot(
            NAlice::NMusic::SLOT_OFFSET,
            NAlice::NMusic::SLOT_OFFSET_TYPE,
            /* optional */ true,
            /* value */ slotOffset->Value.GetString()
        );
    }

    return ctx.RunResponseFormHandler();
}

void TFairyTalesHandler::Register(THandlersMap* handlers) {
    handlers->emplace(FAIRY_TALES_FORM_NAME, []() { return MakeHolder<TFairyTalesHandler>(); });
}

}
