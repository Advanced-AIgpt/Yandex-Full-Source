#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NMusic {

// https://a.yandex-team.ru/arc/trunk/arcadia/music/backend/common/src/main/java/ru/yandex/music/core/site/PlaylistManagerCore.java?rev=r7772896#L82
constexpr TStringBuf PLAYLIST_LIKE_KIND = "3";
constexpr TStringBuf PLAYLIST_ORIGIN_OWNER_UID = "940441070"; // uid у всех плейлистов с Алисой

// https://a.yandex-team.ru/arc_vcs/music/backend/common/src/main/resources/application-default.properties?rev=r8972018#L837
constexpr TStringBuf PLAYLIST_CHART_KIND = "1076";
constexpr TStringBuf PLAYLIST_CHART_UID = "414787002";

struct TPlaylistId {
    const TString Owner;
    const TString Kind;

    TPlaylistId(TString owner, TString kind);

    TPlaylistId(TStringBuf owner, TStringBuf kind);

    TString ToString() const;

    TString ToStringForRadio() const;

    static TMaybe<TPlaylistId> FromString(const TStringBuf playlistId);
};

TString ConvertSpecialPlaylistId(const TStringBuf bassSpecialPlaylistId);

} // namespace NAlice::NHollywood::NMusic
