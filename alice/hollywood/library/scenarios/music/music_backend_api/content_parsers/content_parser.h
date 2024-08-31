#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/library/logger/logger.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/util/rng.h>

namespace NAlice::NHollywood::NMusic {

void ParseContent(TRTLogger& logger, NMetrics::ISensors& sensors, const TStringBuf contentResp,
                  TMusicQueueWrapper& mq, TMusicContext& mCtx, bool moveFromQueueToHistory = true);

} // namespace NAlice::NHollywood::NMusic
