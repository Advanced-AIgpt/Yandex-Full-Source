#include "providers.h"
#include "answers.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/forms/search/search.h>
#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS::NMusic {

namespace {

NSc::TValue ConvertArtists(const NSc::TValue& snippet) {
    NSc::TValue artists;
    if (snippet["grp"].IsArray()) {
        for (const auto& g : snippet["grp"].GetArray()) {
            NSc::TValue& artist = artists.Push();
            artist["name"] = g["name"];
            artist["id"] = g["id"];
            artist["is_various"] = g.Get("various").GetBool(false);
        }
    }
    return artists;
}

} // end anon namespace

TSnippetProvider::TSnippetProvider(TContext& ctx)
    : TYandexMusicProvider(ctx)
{
}

bool TSnippetProvider::InitRequestParams(const NSc::TValue& slotData) {
    LOG(DEBUG) << "TSnippetProvider::InitRequestParams called" << Endl;
    if (DataGetHasKey(slotData, SNIPPET_SLOTS)) {
        Snippet = slotData[SNIPPET_SLOTS]["snippet"].Clone();
        Ctx.DeleteSlot("snippet");
        return true;
    }
    return false;
}

// static method
NSc::TValue TSnippetProvider::MakeDataFromSnippet(TContext& ctx, bool autoplay, const NSc::TValue& snippet) {
    if (snippet.IsNull()) {
        return NSc::Null();
    }

    NSc::TValue res;
    bool needAutoplay = autoplay && !ctx.MetaClientInfo().IsSmartSpeaker();

    TStringBuf subtype = snippet["subtype"].GetString();
    TPathVector path;

    res["type"].SetString(subtype);

    if (subtype == TStringBuf("track")) {
        TString albumId = snippet["alb_id"].ForceString();
        TString trackId = snippet["track_id"].ForceString();

        res["id"] = trackId;
        res["title"] = snippet["track_name"];
        res["artists"] = ConvertArtists(snippet);

        res["album"].SetDict();
        res["album"]["id"] = albumId;
        res["album"]["title"] = snippet["alb_name"];
        res["coverUri"].SetString(TYandexMusicAnswer::MakeCoverUri(snippet["alb_image_uri"].GetString()));

        if (albumId && trackId) {
            path.emplace_back("album", albumId);
            path.emplace_back("track", trackId);
            res["uri"].SetString(TYandexMusicAnswer::MakeLinkForTrack(ctx.ClientFeatures(), path, trackId, needAutoplay));
        } else if (trackId) {
            path.emplace_back("track", trackId);
            res["uri"].SetString(TYandexMusicAnswer::MakeLinkForTrack(ctx.ClientFeatures(), path, trackId, needAutoplay));
        }
    } else if (subtype == TStringBuf("artist")) {
        const NSc::TValue& artist = snippet.TrySelect("/grp[0]"); // take first group only
        if (artist.IsNull()) {
            return NSc::Null();
        }
        TString artistId = artist["id"].ForceString();
        res["id"] = artistId;
        res["name"] = artist["name"];
        res["coverUri"].SetString(TYandexMusicAnswer::MakeCoverUri(snippet["alb_image_uri"].GetString()));

        path.emplace_back("artist", artistId);
        res["uri"].SetString(TYandexMusicAnswer::MakeLinkForMusicObject(ctx.ClientFeatures(), path, needAutoplay));
    } else if (subtype == TStringBuf("album")) {
        TString albumId = snippet["alb_id"].ForceString();
        res["id"] = albumId;
        res["title"] = snippet["alb_name"];
        res["artists"] = snippet["grp"].Clone();
        res["coverUri"].SetString(TYandexMusicAnswer::MakeCoverUri(snippet["alb_image_uri"].GetString()));

        path.emplace_back("album", albumId);
        res["uri"].SetString(TYandexMusicAnswer::MakeLinkForMusicObject(ctx.ClientFeatures(), path, needAutoplay));
    } else if (subtype == TStringBuf("playlist")) {
        TString user = snippet["pls_user_login"].ForceString();
        if (user.empty()) {
            user = snippet["pls_user_id"].ForceString();
            if (user.empty()) {
                return NSc::Null();
            }
        }
        TString plsKind = snippet["pls_kind"].ForceString();
        res["title"] = snippet["pls_name"];
        res["id"] = TStringBuilder() << snippet["pls_user_id"].ForceString() << ":" << plsKind;
        res["coverUri"].SetString(TYandexMusicAnswer::MakeCoverUri(snippet["alb_image_uri"].GetString()));

        path.emplace_back("users", user);
        path.emplace_back("playlists", plsKind);
        res["uri"].SetString(TYandexMusicAnswer::MakeLinkForMusicObject(ctx.ClientFeatures(), path, needAutoplay));
    }

    if (res["uri"].GetString().empty()) {
        return NSc::Null();
    }

    return res;
}

TResultValue TSnippetProvider::Apply(NSc::TValue* out,
                                     NSc::TValue&& /* applyArguments */) {
    // autoplay = false, because we don't know about action in user query
    // This code is only for redirect search -> music
    (*out) = MakeDataFromSnippet(Ctx, /* autoplay */ false, Snippet);

    if (out->IsNull()) {
        return TError(
            TError::EType::SYSTEM,
            TStringBuf("snippet_parse_error")
        );
    }

    return TResultValue();
}

//TODO: remove this override method when VINS fixes re-sending filled form back to us
void TSnippetProvider::AddSuggest(const NSc::TValue& /* result */) {
}

} // namespace NBASS::NMusic
