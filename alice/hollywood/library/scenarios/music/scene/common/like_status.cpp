#include "like_status.h"

#include <alice/protos/data/scenario/music/content_id.pb.h>

using NAlice::NData::NMusic::TContentId;
using NAlice::NHollywood::NMusic::TMusicQueueWrapper;
using NAlice::NHollywood::NMusic::TTrackAlbumId;

namespace NAlice::NHollywoodFw::NMusic {

namespace {

TMaybe<TTrackAlbumId> TryGetTrackAlbumIdFromContentId(const TContentId& contentId) {
    if (contentId.GetType() != NData::NMusic::TContentId_EContentType_Track) {
        return Nothing();
    }

    // id is either "<trackId>:<albumId>" or "<trackId>"
    TStringBuf trackId = contentId.GetId();
    TStringBuf albumId;
    if (trackId.TrySplit(':', trackId, albumId)) {
        return TTrackAlbumId{trackId, albumId};
    } else {
        return TTrackAlbumId{trackId};
    }
}

TMaybe<TTrackAlbumId> TryGetTrackAlbumIdFromMusicQueue(const TMusicQueueWrapper& musicQueue) {
    if (!musicQueue.HasCurrentItem()) {
        // should have current item for making a command on current item
        return Nothing();
    }

    if (musicQueue.IsGenerative() || musicQueue.IsFmRadio()) {
        // commands not supported for generative and fm radio
        return Nothing();
    }

    const auto& currentItem = musicQueue.CurrentItem();
    const TStringBuf trackId = currentItem.GetTrackId();
    const TStringBuf albumId = currentItem.GetTrackInfo().GetAlbumId();
    return TTrackAlbumId{trackId, albumId};
}

} // namespace

TMaybe<TTrackAlbumId> TryGetTrackAlbumId(const TMusicQueueWrapper& musicQueue, const TContentId* contentId) {
    if (contentId) {
        return TryGetTrackAlbumIdFromContentId(*contentId);
    } else {
        return TryGetTrackAlbumIdFromMusicQueue(musicQueue);
    }
}

} // namespace NAlice::NHollywoodFw::NMusic
