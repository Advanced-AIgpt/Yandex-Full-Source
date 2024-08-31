#include "track_album_id.h"

#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <util/string/builder.h>

namespace NAlice::NHollywood::NMusic {

TTrackAlbumId::TTrackAlbumId(const TString& trackId, const TString& albumId)
    : TrackId(trackId)
    , AlbumId(albumId) {
}

TTrackAlbumId::TTrackAlbumId(TStringBuf trackId, TStringBuf albumId)
    : TrackId(trackId)
    , AlbumId(albumId) {
}

TString TTrackAlbumId::ToString() const {
    if (IsUgcTrackId(TrackId)) {
        return TrackId;
    } else {
        Y_ENSURE(!AlbumId.empty());
        return TStringBuilder() << TrackId << ":" << AlbumId;
    }
}

TTrackAlbumId TTrackAlbumId::FromString(const TStringBuf trackAlbumId) {
    TStringBuf trackId, albumId;
    if (trackAlbumId.TrySplit(':', trackId, albumId)) {
        return TTrackAlbumId(TString(trackId), TString(albumId));
    } else {
        Y_ENSURE(IsUgcTrackId(trackAlbumId));
        return TTrackAlbumId(TString(trackAlbumId), "");
    }
}

} // namespace NAlice::NHollywood::NMusic
