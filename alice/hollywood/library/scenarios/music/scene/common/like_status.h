#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/track_album_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>

namespace NAlice::NHollywoodFw::NMusic {

// only likes for tracks supported now
TMaybe<NHollywood::NMusic::TTrackAlbumId> TryGetTrackAlbumId(
    const NHollywood::NMusic::TMusicQueueWrapper& musicQueue,
    const NData::NMusic::TContentId* contentId);

} // namespace NAlice::NHollywoodFw::NMusic
