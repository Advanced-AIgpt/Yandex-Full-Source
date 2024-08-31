#include "answer.h"

#include "fairytale_linear_albums.h"

#include <alice/library/url_builder/url_builder.h>

#include <dict/dictutil/str.h>

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/generic/algorithm.h>
#include <util/string/builder.h>

namespace {

constexpr TStringBuf DEFAULT_COVER_SIZE = "200x200";
constexpr TStringBuf DEFAULT_COVER = "https://avatars.mds.yandex.net/get-bass/469429/music_default_img/orig";
constexpr TStringBuf ORIGIN_SPECIAL_PLAYLIST_COVER_URI = "https://avatars.mds.yandex.net/get-bass/787408/orig_playlist/orig";

TString GetForceString(const NJson::TJsonValue& json) {
    if (json.IsString()) {
        return json.GetString();
    }

    if (json.IsInteger()) {
        return ToString(json.GetInteger());
    }

    if (json.IsUInteger()) {
        return ToString(json.GetUInteger());
    }

    if (json.IsDouble()) {
        return ToString(json.GetDouble());
    }

    if (json.IsBoolean()) {
        return ToString(int(json.GetBoolean()));
    }

    return "";
}

NJson::TJsonValue MakeArtistBlock(const NJson::TJsonValue& artist) {
    NJson::TJsonValue artistItem;
    artistItem["name"] = artist["name"].GetString();
    artistItem["id"] = GetForceString(artist["id"]);
    artistItem["is_various"] = artist["various"].GetBoolean();
    artistItem["composer"] = artist["composer"].GetBoolean();
    return artistItem;
}

NJson::TJsonValue MakeArtistsList(const NJson::TJsonValue::TArray& artists) {
    NJson::TJsonValue artistsValue;
    for (const auto& artist : artists) {
        artistsValue.AppendValue(MakeArtistBlock(artist));
        if (artist.Has("decomposed") && !artist["decomposed"].GetArray().empty()) {
            for (const auto& a : artist["decomposed"].GetArray()) {
                if (a.IsMap()) {
                    artistsValue.AppendValue(MakeArtistBlock(a));
                }
            }
        }
    }
    return artistsValue;
}

TString MakeCoverUri(TStringBuf origUri) {
    if (origUri.empty()) {
        return TString{DEFAULT_COVER};
    } else {
        TString coverUri(origUri);
        if (!coverUri.StartsWith("http")) {
            coverUri = TString::Join("https://", coverUri);
        }
        size_t pos = coverUri.find("%%");
        if (pos != TString::npos) {
            coverUri.replace(pos, 2, DEFAULT_COVER_SIZE);
        }
        return coverUri;
    }
}

TString MakeLinkForMusicObject(
    const TString& path,
    const bool needAutoplay,
    const NAlice::TClientInfo& clientInfo,
    const bool supportsIntentUrls
) {
    if (path.empty()) {
        return TString();
    }

    TCgiParameters cgi;
    if (needAutoplay && !clientInfo.IsSmartSpeaker()) {
        cgi.InsertEscaped(TStringBuf("play"), TStringBuf("1"));
    }

    return GenerateMusicAppUri(
        supportsIntentUrls,
        clientInfo,
        NAlice::EMusicUriType::Music,
        path,
        cgi
    );
}

TString MakeLinkForTrack(
    const TString& path,
    TStringBuf trackId,
    bool needAutoplay,
    const NAlice::TClientInfo& clientInfo,
    const bool supportsIntentUrls
) {
    if (path.empty()) {
        return TString();
    }

    TCgiParameters cgi;
    if (needAutoplay) {
        if (clientInfo.IsIOS()) {
            cgi.InsertEscaped(TStringBuf("playTrack"), trackId);
        } else if (!clientInfo.IsSmartSpeaker()) {
            cgi.InsertEscaped(TStringBuf("play"), TStringBuf("1"));
        }
    }

    return GenerateMusicAppUri(
        supportsIntentUrls,
        clientInfo,
        NAlice::EMusicUriType::Music,
        path,
        cgi
    );
}

} // namespace

namespace NAlice::NMusic {

NJson::TJsonValue MakeBestTrack(
    const NAlice::TClientInfo& clientInfo,
    const bool supportsIntentUrls,
    const bool needAutoplay,
    const NJson::TJsonValue& track
) {
    const TString& trackId = GetForceString(track["id"]);
    if (trackId.empty()) {
        return NJson::TJsonValue(NJson::JSON_NULL);
    }

    const NJson::TJsonValue& album = track["albums"][0];
    const TString& albumId = GetForceString(album["id"]);

    if (NAlice::NMusic::IsTalesAlbumWithChapters(albumId)) {
        return MakeBestAlbum(
            clientInfo,
            supportsIntentUrls,
            needAutoplay,
            album,
            /* firstTrack */ true
        );
    }

    NJson::TJsonValue res;
    res["type"] = "track";
    res["subtype"] = track["type"];
    res["title"] = track["title"].GetString();
    res["durationMs"] = GetForceString(track["durationMs"]);
    res["childContent"] = track["childContent"].GetBoolean();
    res["id"] = trackId;

    res["artists"] = MakeArtistsList(track["artists"].GetArray());

    res["album"]["title"] = album["title"].GetString();
    res["album"]["id"] = albumId;
    res["album"]["genre"] = album["genre"].GetString();
    res["album"]["type"] = album["type"].GetString();
    res["album"]["childContent"] = album["childContent"].GetBoolean();

    res["coverUri"] = MakeCoverUri(album["coverUri"].GetString());

    if (track.Has("allPartsContainer")) {
        const auto& container = track["allPartsContainer"];
        res["allPartsContainer"]["type"] = container["type"].GetString();
        res["allPartsContainer"]["data"]["id"] = container["data"]["id"].GetInteger();
        res["allPartsContainer"]["data"]["genre"] = container["data"]["genre"].GetString();
    }

    const TString uriPath = albumId.empty()
        ? TString::Join("track/", trackId, "/")
        : TString::Join("album/", albumId, "/track/", trackId, "/");

    res["uri"] = MakeLinkForTrack(
        uriPath,
        trackId,
        needAutoplay,
        clientInfo,
        supportsIntentUrls
    );

    return res;
}

NJson::TJsonValue MakeBestAlbum(
    const NAlice::TClientInfo& clientInfo,
    const bool supportsIntentUrls,
    const bool needAutoplay,
    const NJson::TJsonValue& album,
    bool firstTrack
) {
    const TString& albumId = GetForceString(album["id"]);
    if (albumId.empty()) {
        return NJson::TJsonValue(NJson::JSON_NULL);
    }

    NJson::TJsonValue res;
    res["type"] = "album";
    res["subtype"] = album["type"];
    res["title"] = album["title"].GetString();
    res["genre"] = album["genre"].GetString();
    res["childContent"] = album["childContent"].GetBoolean();
    res["coverUri"] = MakeCoverUri(album["coverUri"].GetString());
    res["id"] = albumId;

    res["artists"] = MakeArtistsList(album["artists"].GetArray());

    if (firstTrack) {
        TString trackId = GetForceString(album["id"]);
        if (trackId.empty()) {
            return NJson::TJsonValue(NJson::JSON_NULL);
        }

        auto path = TString::Join("album/", albumId, "/track/", trackId, "/");

        res["uri"] = MakeLinkForTrack(
            path,
            trackId,
            needAutoplay,
            clientInfo,
            supportsIntentUrls
        );

        res["firstTrackUri"] = MakeLinkForTrack(
            path,
            trackId,
            /* autoplay = */ true,
            clientInfo,
            supportsIntentUrls
        );

        res["firstTrack"] = album;
    } else {
        auto path = TString::Join("album/", albumId, "/");

        res["uri"] = MakeLinkForMusicObject(
            path,
            /* autoplay = */ true,
            clientInfo,
            supportsIntentUrls
        );
    }

    return res;
}

NJson::TJsonValue MakeBestArtist(
    const NAlice::TClientInfo& clientInfo,
    const bool supportsIntentUrls,
    const bool needAutoplay,
    const NJson::TJsonValue& artist
) {
    const TString& artistId = GetForceString(artist["id"]);
    if (artistId.empty()) {
        return false;
    }

    NJson::TJsonValue res;
    res["type"] = "artist";
    res["name"] = artist["name"].GetString();
    res["coverUri"] = MakeCoverUri(artist["cover"]["uri"].GetString());
    if (const auto& genres = artist["genres"]; !genres.GetArray().empty()) {
        res["genre"] = genres[0].GetString();
    }

    res["id"] = artistId;

    auto path = TString::Join("artist/", artistId, "/");

    res["uri"] = MakeLinkForMusicObject(
        path,
        needAutoplay,
        clientInfo,
        supportsIntentUrls
    );

    return res;
}

NJson::TJsonValue MakePlaylist(
    const NAlice::TClientInfo& clientInfo,
    const bool supportsIntentUrls,
    const bool needAutoplay,
    const NJson::TJsonValue& playlist
) {
    const TString& playlistKind = GetForceString(playlist["kind"]);
    const TString& specialPlaylist = GetForceString(playlist["special_playlist"]);

    if (playlistKind.empty() && specialPlaylist.empty()) {
        return NJson::TJsonValue(NJson::JSON_NULL);
    }

    NJson::TJsonValue res;
    TString path;

    res["type"] = "playlist";
    res["title"] = playlist["title"].GetString();
    res["childContent"] = playlist["childContent"].GetBoolean();

    if (!playlistKind.empty()) {
        const NJson::TJsonValue& cover = playlist["cover"];
        TStringBuf coverUri;
        if (cover.IsMap()) {
            if (cover["type"].GetString() == "pic") {
                coverUri = cover["uri"].GetString();
            } else if (cover["type"].GetString() == TStringBuf("mosaic")) {
                coverUri = cover["itemsUri"][0].GetString();
            }
        }
        res["coverUri"] = MakeCoverUri(coverUri);

        const TString& userLogin = GetForceString(playlist["owner"]["login"]);
        TString userID = GetForceString(playlist["owner"]["id"]);
        if (userID.empty()) {
            userID = GetForceString(playlist["uid"]);
            if (userID.empty()) {
                return NJson::TJsonValue(NJson::JSON_NULL);
            }
        }

        res["id"] = TString::Join(userID, ":", playlistKind);

        TString user(userLogin.empty() ? userID : userLogin);

        path = TString::Join("users/", user, "/playlists/", playlistKind, "/");

        const NJson::TJsonValue& firstTrack = playlist["tracks"][0]["track"];
        const NJson::TJsonValue& album = firstTrack["albums"][0];
        const TString& albumId = GetForceString(album["id"]);

        if (!albumId.empty()) {
            res["firstTrack"]["album"]["title"] = album["title"].GetString();
            res["firstTrack"]["album"]["id"] = albumId;
            res["firstTrack"]["album"]["genre"] = album["genre"].GetString();
            res["firstTrack"]["album"]["childContent"] = album["childContent"].GetBoolean();
            res["firstTrack"]["subtype"] = firstTrack["type"].GetString();
        }
    } else if (!specialPlaylist.empty()) {
        path = TString::Join("playlist/", specialPlaylist, "/");

        if (specialPlaylist == "origin") {
            res["coverUri"] = MakeCoverUri(ORIGIN_SPECIAL_PLAYLIST_COVER_URI);
            res["mainColor"] = "#9028F8";
            res["secondColor"] = "#280880";
        }
    }

    res["uri"] = MakeLinkForMusicObject(
        path,
        needAutoplay,
        clientInfo,
        supportsIntentUrls
    );

    return res;
}

} // namespace NAlice:NMusic
