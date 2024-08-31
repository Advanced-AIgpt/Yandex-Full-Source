#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <library/cpp/json/json_value.h>

namespace NAlice::NHollywood::NMusic {

struct TParseTrackParams {
    const TMaybe<int> Position = Nothing();
    const TMaybe<TStringBuf> AlbumId = Nothing();
    const bool ForceChildSafe = false;
};

void ParseSingleTrack(const NJson::TJsonValue& resultJson, TMusicQueueWrapper& mq, bool hasMusicSubscription);
TQueueItem ParseTrack(const NJson::TJsonValue& trackJson, TParseTrackParams params = TParseTrackParams{});

} // namespace NAlice::NHollywood::NMusic
