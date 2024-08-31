#pragma once

#include <alice/library/client/client_info.h>
#include <library/cpp/json/writer/json_value.h>

namespace NAlice::NMusic {

NJson::TJsonValue MakeBestTrack(
    const NAlice::TClientInfo& clientInfo,
    const bool supportsIntentUrls,
    const bool needAutoplay,
    const NJson::TJsonValue& track
);

NJson::TJsonValue MakeBestAlbum(
    const NAlice::TClientInfo& clientInfo,
    const bool supportsIntentUrls,
    const bool needAutoplay,
    const NJson::TJsonValue& album,
    bool firstTrack
);

NJson::TJsonValue MakeBestArtist(
    const NAlice::TClientInfo& clientInfo,
    const bool supportsIntentUrls,
    const bool needAutoplay,
    const NJson::TJsonValue& artist
);

NJson::TJsonValue MakePlaylist(
    const NAlice::TClientInfo& clientInfo,
    const bool supportsIntentUrls,
    const bool needAutoplay,
    const NJson::TJsonValue& playlist
);

} // namespace NAlice::NMusic
