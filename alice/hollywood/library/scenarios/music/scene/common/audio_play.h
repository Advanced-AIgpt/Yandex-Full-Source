#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/proto/play_audio.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

class TAudioPlayBuilder {
public:
    TAudioPlayBuilder(const NHollywood::NMusic::TMusicQueueWrapper& musicQueue);
    TAudioPlayBuilder& AddOnStartedCallback(const NHollywood::NMusic::TCallbackPayload& callback);
    TAudioPlayBuilder& AddOnStoppedCallback(const NHollywood::NMusic::TCallbackPayload& callback);
    TAudioPlayBuilder& AddOnFailedCallback(const NHollywood::NMusic::TCallbackPayload& callback);
    TAudioPlayBuilder& AddOnFinishedCallback(const NHollywood::NMusic::TCallbackPayload& callback);

    NScenarios::TAudioPlayDirective&& Build() &&;

private:
    NScenarios::TAudioPlayDirective Directive_;
};

} // namespace NAlice::NHollywoodFw::NMusic
