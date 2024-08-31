#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>

#include <alice/library/logger/logger.h>

#include <util/generic/strbuf.h>

namespace NAlice::NHollywood::NMusic {

std::pair<size_t, size_t> FindOptimalPageParameters(const TMusicQueueWrapper& mq, const size_t trackIdx);

void ParseFindTrackIdxResponse(const TStringBuf response, const TFindTrackIdxRequest& request,
                               TMusicQueueWrapper& mq);

} // namespace NAlice::NHollywood::NMusic
