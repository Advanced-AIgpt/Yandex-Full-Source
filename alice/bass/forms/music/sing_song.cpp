#include "sing_song.h"

#include "music.h"
#include "providers.h"

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/music/defs.h>

namespace NBASS {

namespace {
static constexpr TStringBuf SING_SONG = "personal_assistant.scenarios.music_sing_song";
static constexpr TStringBuf SING_SONG_NEXT = "personal_assistant.scenarios.music_sing_song__next";

using namespace NMusic;

void SuggestWithSpecialPlaylist(TContext& ctx, TStringBuf playlistName) {
    NSc::TValue formUpdate;
    formUpdate["name"] = MUSIC_PLAY_FORM_NAME;
    formUpdate["resubmit"].SetBool(true);

    NSc::TValue& slotPlaylist = formUpdate["slots"].SetArray().Push();
    slotPlaylist["name"].SetString(TStringBuf("special_playlist"));
    slotPlaylist["type"].SetString(TStringBuf("string"));
    slotPlaylist["optional"].SetBool(true);
    slotPlaylist["value"].SetString(playlistName);

    NSc::TValue& slotAutoplay = formUpdate["slots"].SetArray().Push();
    slotAutoplay["name"].SetString(TStringBuf("action_request"));
    slotAutoplay["type"].SetString(TStringBuf("action_request"));
    slotAutoplay["optional"].SetBool(true);
    slotAutoplay["value"].SetString(TStringBuf("autoplay"));

    ctx.AddSuggest(TStringBuf("music_sing_song__ya_music_playlist"), NSc::TValue().SetString(playlistName), formUpdate);
}

}

TResultValue TSingSongFormHandler::Do(TRequestHandler &r) {
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::PROMO);
    if (ctx.MetaClientInfo().IsSmartSpeaker()) {
        TContext::TPtr newCtx = TSearchMusicHandler::SetAsResponse(ctx, false);
        newCtx->CreateSlot(/* name */ "special_playlist", /* type */ "special_playlist", /* optional */ true, /* value */ "ny_alice_playlist");
        return ctx.RunResponseFormHandler();
    }

    ctx.AddSuggest(TStringBuf("music_sing_song__another_song"));
    SuggestWithSpecialPlaylist(ctx, TStringBuf("ny_alice_playlist"));
    ctx.AddSearchSuggest();
    ctx.AddOnboardingSuggest();

    return TResultValue();
}

void TSingSongFormHandler::Register(THandlersMap *handlers) {
    auto cbSingSongForm = []() {
        return MakeHolder<TSingSongFormHandler>();
    };
    handlers->emplace(SING_SONG, cbSingSongForm);
    handlers->emplace(SING_SONG_NEXT, cbSingSongForm);
}

}
