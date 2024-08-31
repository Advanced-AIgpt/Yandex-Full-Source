#include "content_id.h"
#include "similar_radio_id.h"
#include "playlist_id.h"

namespace {

TString ToStringForRadio(const NAlice::NHollywood::NMusic::TContentId& contentId) {
    if (contentId.GetType() == NAlice::NHollywood::NMusic::TContentId_EContentType_Playlist) {
        const auto playlistId = NAlice::NHollywood::NMusic::TPlaylistId::FromString(contentId.GetId());
        Y_ENSURE(playlistId, "Failed to create playlistId from " << contentId.GetId());
        return playlistId->ToStringForRadio();
    }

    return contentId.GetId();
}

} // empty namespace

namespace NAlice::NHollywood::NMusic {

TString SimilarRadioId(const TContentId& contentId) {
    return TString::Join(ContentTypeToText(contentId.GetType()), ":", ToStringForRadio(contentId));
}

} // namespace NAlice::NHollywood::NMusic
