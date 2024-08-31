#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

NScenarios::TAudioPlayDirective::TAudioPlayMetadata::TGlagolMetadata BuildGlagolMetadata(const NHollywood::NMusic::TMusicQueueWrapper& musicQueue);
NScenarios::TSetGlagolMetadataDirective BuildSetGlagolMetadataDirective(const NHollywood::NMusic::TMusicQueueWrapper& musicQueue);

} // namespace NAlice::NHollywoodFw::NMusic
