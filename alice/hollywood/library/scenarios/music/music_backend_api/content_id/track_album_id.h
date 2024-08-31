#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NMusic {

struct TTrackAlbumId {
    const TString TrackId;
    const TString AlbumId;

    TTrackAlbumId(const TString& trackId, const TString& albumId = "");

    TTrackAlbumId(TStringBuf trackId, TStringBuf albumId = "");

    TString ToString() const;

    static TTrackAlbumId FromString(const TStringBuf trackAlbumId);
};

} // namespace NAlice::NHollywood::NMusic
