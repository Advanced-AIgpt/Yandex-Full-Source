#include "playlist_id.h"

#include <util/string/builder.h>

namespace NAlice::NHollywood::NMusic {

TPlaylistId::TPlaylistId(TString owner, TString kind)
    : Owner(std::move(owner))
    , Kind(std::move(kind)) {
}

TPlaylistId::TPlaylistId(TStringBuf owner, TStringBuf kind)
    : Owner(owner)
    , Kind(kind) {
}

TString TPlaylistId::ToString() const {
    return TStringBuilder() << Owner << ':' << Kind;
}

TMaybe<TPlaylistId> TPlaylistId::FromString(const TStringBuf playlistId) {
    TStringBuf owner, kind;
    if (playlistId.TrySplit(':', owner, kind)) {
        return TPlaylistId(TString(owner), TString(kind));
    }
    return Nothing();
}

TString TPlaylistId::ToStringForRadio() const {
    return TStringBuilder() << Owner << '_' << Kind;
}

TString ConvertSpecialPlaylistId(const TStringBuf bassSpecialPlaylistId) {
    TString musicApiPlaylistId{bassSpecialPlaylistId};
    size_t pos = 0;
    // playlist_of_the_day -> playlistOfTheDay
    while ((pos = musicApiPlaylistId.find('_', pos)) != TString::npos) {
        musicApiPlaylistId.erase(pos, 1);
        if (pos < musicApiPlaylistId.size()) {
            musicApiPlaylistId.to_upper(pos, 1);
        }
    }
    return musicApiPlaylistId;
}

} // namespace NAlice::NHollywood::NMusic
