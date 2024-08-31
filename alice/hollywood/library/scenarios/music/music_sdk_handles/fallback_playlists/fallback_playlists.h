#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

// If the device doesn't support musicsdk links with Radio (like Searchapp),
// it should fallback to a playlist
TMaybe<TPlaylistId> TryGetFallbackPlaylist(const TStringBuf metatag);

} // namespace NAlice::NHollywood::NMusic::NMusicSdk
