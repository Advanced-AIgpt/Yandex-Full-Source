#include "music.h"

#include "providers.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/music/defs.h>

namespace NBASS {

using namespace NMusic;

namespace {

const TStringBuf ALBUM = "album";

struct TSlotParams {
    bool HasSearchSlots = false;
    bool NeedSimilar = false;

    TString Target;
};

TSlotParams CheckSlots(const TContext& ctx) {
    TSlotParams params;

    for (const auto& slotName : SLOT_NAMES.find(SEARCH_SLOTS)->second) {
        const TContext::TSlot* slot = ctx.GetSlot(slotName);
        if (!IsSlotEmpty(slot)) {
            params.HasSearchSlots = true;
        }
    }
    const TContext::TSlot* slotTarget = ctx.GetSlot("target_type");
    if (!IsSlotEmpty(slotTarget)) {
        params.Target = slotTarget->Value.GetString();
    }

    if (!IsSlotEmpty(ctx.GetSlot("need_similar"))) {
        params.NeedSimilar = true;
    }

    return params;
}

} // namespace

TResultValue TMusicAnaphoraHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::MUSIC);
    return Run(r.Ctx());
}

TResultValue TMusicAnaphoraHandler::Run(TContext& ctx) {
    TSlotParams params = CheckSlots(ctx);

    const NSc::TValue& currentTrack = *ctx.Meta().DeviceState().Music().CurrentlyPlaying().TrackInfo().GetRawValue();

    TContext::TPtr newCtx = TSearchMusicHandler::SetAsResponse(ctx, false);

    if (!params.HasSearchSlots && currentTrack.IsNull()) {
        NSc::TValue err;
        err["code"] = ERROR_MUSIC_NOT_FOUND;
        newCtx->AddErrorBlock(
            TError(TError::EType::MUSICERROR),
            std::move(err)
        );
        return TResultValue();
    }

    if (params.HasSearchSlots) {
        for (const auto& slotName : SLOT_NAMES.find(SEARCH_SLOTS)->second) {
            const TContext::TSlot* slot = ctx.GetSlot(slotName);
            if (!IsSlotEmpty(slot)) {
                newCtx->CopySlotFrom(ctx, slotName);
            }
        }
        if (params.Target == ALBUM) {
            TContext::TSlot* album = newCtx->GetOrCreateSlot(ALBUM, "string");
            if (IsSlotEmpty(album)) {
                album->Value.SetString(ALBUM);
            }
        }
    } else {
        if (params.Target == ALBUM) {
            newCtx->CreateSlot("album_id", "string", true, currentTrack.TrySelect("albums/0/id"));
        } else if (params.Target == TStringBuf("artist") || (params.Target.empty() && !params.NeedSimilar)) {
            newCtx->CreateSlot("artist_id", "string", true, currentTrack.TrySelect("artists/0/id"));
        } else {
            newCtx->CreateSlot("track_id", "string", true, currentTrack.TrySelect("id"));
        }
    }

    newCtx->CopySlotsFrom(ctx, {
            TStringBuf("callback_form"),
            TStringBuf("need_similar"),
            TStringBuf("action_request"),
            TStringBuf("order"),
            TStringBuf("repeat")
    });

    return ctx.RunResponseFormHandler();
}

void TMusicAnaphoraHandler::Register(THandlersMap* handlers) {
    handlers->emplace(MUSIC_PLAY_ANAPHORA_FORM_NAME, []() { return MakeHolder<TMusicAnaphoraHandler>(); });
}

}
