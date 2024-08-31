#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>

#include <util/generic/strbuf.h>

namespace NAlice::NHollywood::NMusic {

void ParseTrackFullInfoResponse(const TStringBuf response, TMusicQueueWrapper& mq);

void ParseTrackSearchResponse(const TStringBuf response, TMusicQueueWrapper& mq);

} // namespace NAlice::NHollywood::NMusic
