#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/library/logger/logger.h>
#include <alice/library/metrics/sensors.h>

#include <library/cpp/json/json_value.h>

namespace NAlice::NHollywood::NMusic {

void ParseRadio(TRTLogger& logger, NMetrics::ISensors& sensors, const NJson::TJsonValue& radioJson,
                TMusicQueueWrapper& mq, bool hasMusicSubscription);

} // NAlice::NHollywoood::NMusic
