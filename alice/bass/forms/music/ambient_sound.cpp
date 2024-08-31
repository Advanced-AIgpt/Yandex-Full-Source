#include "ambient_sound.h"
#include "catalog.h"
#include "common_headers.h"
#include "music.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/forms/music/providers.h>
#include <alice/bass/forms/music/base_provider.cpp>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/music/defs.h>

#include <util/string/builder.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS {

namespace {

const TStringBuf AMBIENT_SOUND_FORM_NAME = "personal_assistant.scenarios.music_ambient_sound";
const TStringBuf PODCAST_FORM_NAME = "personal_assistant.scenarios.music_podcast";
const TStringBuf HARDCODED_MUSIC_FORM_NAME = "personal_assistant.scenarios.hardcoded_music";

const TStringBuf STRING_TYPE = "string";
const TStringBuf PLAYLIST_TYPE = "playlist";

const TStringBuf DEFAULT_AMBIENT_SOUND = "playlist/103372440:1919";
const TStringBuf DEFAULT_CHILD_PODCAST = "playlist/970829816:1064";
const TStringBuf DEFAULT_PODCAST = "playlist/414787002:1104";
const TStringBuf ALICE_PODCAST = "album/6270545";
const TStringBuf DEFAULT_HARDCODED_MUSIC = "playlist/105590476:1052";

const TStringBuf ITEM_ID_SLOT = "item_id";
const TStringBuf AMBIENT_SOUND = "ambient_sound";
const TStringBuf UNKNOWN_SOUND = "unknown_ambient_sound";
const TStringBuf UNKNOWN_MUSIC_ITEM = "unknown_music_item";

TResultValue NotFound(TContext& ctx) {
    NSc::TValue errData;
    errData["code"].SetString(AMBIENT_SOUND_FORM_NAME == ctx.FormName() ? UNKNOWN_SOUND : UNKNOWN_MUSIC_ITEM);
    ctx.AddErrorBlock(
        TError(TError::EType::MUSICERROR, AMBIENT_SOUND_FORM_NAME == ctx.FormName() ? UNKNOWN_SOUND : UNKNOWN_MUSIC_ITEM),
        std::move(errData)
    );
    return TResultValue();
}

} // namespace

TResultValue TAmbientSoundHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(ProductScenarioName);
    ctx.DeleteSlot("answer");

    const TStringBuf inputSlotName = AMBIENT_SOUND_FORM_NAME == ctx.FormName() ? AMBIENT_SOUND : ITEM_ID_SLOT;

    // Change form to music for unknown music items (text requests)
    TContext::TSlot* slotMusicItemText = ctx.GetSlot(inputSlotName, STRING_TYPE);
    if (!IsSlotEmpty(slotMusicItemText)) {
        TContext::TPtr newCtx = NMusic::TSearchMusicHandler::SetAsResponse(ctx, /* callbackSlot */ false);
        TStringBuilder searchText;
        if (PODCAST_FORM_NAME == ctx.FormName()) {
            searchText << TStringBuf("подкаст ");

            TContext::TSlot* slotOffset = ctx.GetSlot(NAlice::NMusic::SLOT_OFFSET);
            if (!IsSlotEmpty(slotOffset)) {
                newCtx->CreateSlot(
                    NAlice::NMusic::SLOT_OFFSET,
                    NAlice::NMusic::SLOT_OFFSET_TYPE,
                    /* optional */ true,
                    /* value */ slotOffset->Value.GetString()
                );
            }
        } else if (AMBIENT_SOUND_FORM_NAME == ctx.FormName()) {
            searchText << TStringBuf("звук ");
        }
        searchText << slotMusicItemText->Value.GetString();
        newCtx->CreateSlot(TStringBuf("search_text"), TStringBuf("string"), true, NSc::TValue(searchText), NSc::TValue(searchText));
        ctx.RunResponseFormHandler();
    }
    TContext::TSlot* slotMusicItem = ctx.GetSlot(inputSlotName); // slot type can be one of many entities
    TStringBuf soundValue = "";
    if (!IsSlotEmpty(slotMusicItem)) {
        soundValue = slotMusicItem->Value.GetString();
    } else if (AMBIENT_SOUND_FORM_NAME == ctx.FormName()) {
        soundValue = DEFAULT_AMBIENT_SOUND;
    } else if (PODCAST_FORM_NAME == ctx.FormName()) {
        ctx.AddAttention("is_general_playlist");
        if (ctx.GetIsClassifiedAsChildRequest() || ctx.GetContentRestrictionLevel() == EContentRestrictionLevel::Safe) {
            soundValue = DEFAULT_CHILD_PODCAST;
        } else {
            soundValue = ctx.HasExpFlag(EXPERIMENTAL_FLAG_ALICE_PODCAST) ? ALICE_PODCAST : DEFAULT_PODCAST;
        }
    } else if (HARDCODED_MUSIC_FORM_NAME == ctx.FormName()) {
        soundValue = DEFAULT_HARDCODED_MUSIC;
    }

    if ((ctx.MetaClientInfo().IsTvDevice() || ctx.MetaClientInfo().IsLegatus()) &&
        !(ctx.ClientFeatures().SupportsMusicQuasarClient() || ctx.ClientFeatures().SupportsAudioClient()))
    {
        ctx.AddStopListeningBlock();
        ctx.AddAttention(TStringBuf("music_play_not_supported_on_device"), {});
        return TResultValue();
    }

    // for other forms, the default music item must be supported as well
    TStringBuf type;
    TStringBuf fullId;
    if (!soundValue.TrySplit('/', type, fullId)) {
        return NotFound(ctx);
    }

    if (!ctx.ClientFeatures().SupportsMusicPlayer()) {
        return CreateCatalogAnswer(ctx, type, fullId);
    }

    ctx.CreateSlot(TStringBuilder() << type << TStringBuf("_id"), STRING_TYPE, /* optional */ true, NSc::TValue(fullId));
    if (AMBIENT_SOUND_FORM_NAME == ctx.FormName()) {
        ctx.CreateSlot(TStringBuf("order"), STRING_TYPE, /* optional */ true, TStringBuf("shuffle"));
    }
    ctx.CreateSlot(TStringBuf("repeat"), STRING_TYPE, /* optional */ true, TStringBuf("repeat"));
    return NMusic::TSearchMusicHandler::DoWithoutCallback(ctx);
}

TResultValue TAmbientSoundHandler::CreateCatalogAnswer(TContext& ctx, TStringBuf objectType, TStringBuf objectValue) {
    if (objectType.empty() || objectValue.empty()) {
        return NotFound(ctx);
    }

    // TODO(a-square): switch to bulk-info handler
    const auto commonHeaders = NMusic::CreateCommonHeaders(ctx);
    NMusic::TMusicCatalogSingle catalogHandler(commonHeaders, ctx.ClientFeatures(), objectType, true,
                                               ctx.HasExpFlag(NAlice::NExperiments::EXP_MUSIC_LOG_CATALOG_RESPONSE));
    TStringBuilder path;
    if (objectType == PLAYLIST_TYPE) {
        const TStringBuf userId = objectValue.NextTok(':');
        path << TString::Join("/users/", userId, "/playlists/", objectValue, "?rich-tracks=false");
    } else {
        path << '/' << objectType << TStringBuf("s/") << objectValue;
        if (objectType == TStringBuf("artist")) {
            path << TStringBuf("/similar");
        }
    }
    catalogHandler.CreateRequestHandler(ctx.GetSources().MusicCatalog(path).MakeOrAttachRequest(nullptr),
                                        TCgiParameters());

    NSc::TValue answer;
    if (catalogHandler.ProcessRequest(&answer) && !answer.IsNull()) {
        ctx.CreateSlot(TStringBuf("answer"), TStringBuf("music_result"), true, answer);
    } else {
        return NotFound(ctx);
    }

    if (answer.Get("uri").IsString()) {
        NSc::TValue commandData;
        commandData["uri"] = answer.Get("uri");
        if (AMBIENT_SOUND_FORM_NAME == ctx.FormName()) {
            ctx.AddCommand<TMusicAmbientSoundDirective>(TStringBuf("open_uri"), commandData);
            if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_MUSIC_ATTRACTIVE_CARD)) {
                ctx.AddSuggest(TStringBuf("ambient_sound__open_uri"));
            }
         } else {
            ctx.AddCommand<TMusicHardcodedSoundDirective>(TStringBuf("open_uri"), commandData);
            if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_MUSIC_ATTRACTIVE_CARD)) {
                ctx.AddSuggest(TStringBuf("hardcoded_music__open_uri"));
            }
        }
        if (!ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_MUSIC_ATTRACTIVE_CARD)) {
            NSc::TValue card;
            card["coverUri"].SetString(answer["coverUri"]);
            ctx.AddTextCardBlock("music_start");
            ctx.AddDivCardBlock("music__track", std::move(card));
        }
    }

    return TResultValue();
}

void TAmbientSoundHandler::Register(THandlersMap* handlers) {
    handlers->emplace(AMBIENT_SOUND_FORM_NAME, []() {
        return MakeHolder<TAmbientSoundHandler>(NAlice::NProductScenarios::MUSIC_AMBIENT_SOUND);
    });
    handlers->emplace(PODCAST_FORM_NAME, []() {
        return MakeHolder<TAmbientSoundHandler>(NAlice::NProductScenarios::MUSIC_PODCAST);
    });
    handlers->emplace(HARDCODED_MUSIC_FORM_NAME, []() {
        return MakeHolder<TAmbientSoundHandler>(NAlice::NProductScenarios::MUSIC);
    });
}

}
