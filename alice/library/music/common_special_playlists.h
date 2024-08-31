#pragma once

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/variant.h>

namespace NAlice::NMusic {

struct TSpecialPlaylistInfo{
    struct TPlaylist {
        TStringBuf Kind;
        TStringBuf OwnerLogin;
        TStringBuf OwnerId;
    };

    struct TAlbum{
        TStringBuf Id;
        TStringBuf ArtistName;
        TStringBuf ArtistId;
        TStringBuf Genre;
    };

    constexpr TSpecialPlaylistInfo(
        const TStringBuf title,
        const TStringBuf kind,
        const TStringBuf ownerLogin,
        const TStringBuf ownerId
    ) noexcept
        : Title{title}
        , Info{TPlaylist{kind, ownerLogin, ownerId}}
    {
    }

    constexpr TSpecialPlaylistInfo(
        const TStringBuf title,
        const TStringBuf id,
        const TStringBuf artistName,
        const TStringBuf artistId,
        const TStringBuf genre
    ) noexcept
        : Title{title}
        , Info{TAlbum{id, artistName, artistId, genre}}
    {
    }

    template <class T>
    T Convert() const {
        T rv;
        rv["title"] = Title;
        if(std::holds_alternative<TPlaylist>(Info)) {
            const auto& playlist = std::get<TPlaylist>(Info);
            rv["kind"] = playlist.Kind;
            auto& owner = rv["owner"];
            owner["login"] = playlist.OwnerLogin;
            owner["id"] = playlist.OwnerId;
        } else {
            const auto& album = std::get<TAlbum>(Info);
            rv["id"] = album.Id;
            auto& artist = rv["artists"][0];
            artist["name"] = album.ArtistName;
            artist["id"] = album.ArtistId;
            rv["genre"] = album.Genre;
            rv["answer_type"] = "album";
        }
        return rv;
    }

public:
    TStringBuf Title;
    std::variant<TPlaylist, TAlbum> Info;
};

const THashMap<TStringBuf, TSpecialPlaylistInfo>& GetCommonSpecialPlaylists();

} // namespace NAlice::NMusic
