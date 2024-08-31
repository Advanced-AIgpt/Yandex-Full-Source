#pragma once

#include <alice/cachalot/library/modules/activation/common.h>
#include <alice/cachalot/library/modules/activation/storage.h>

#include <alice/cachalot/library/service.h>


namespace NCachalot {

class TActivationService : public IService {
public:
    static TMaybe<TActivationServiceSettings> FindServiceSettings(const TApplicationSettings& settings);

public:
    TActivationService(const TActivationServiceSettings& settings);

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    NThreading::TFuture<void> OnAnnouncementRequest(const NNeh::IRequestRef& request);
    NThreading::TFuture<void> OnFinalRequest(const NNeh::IRequestRef& request);


    // Handlers for voice_input graph are declared below.

    // First announcement.
    // Forwards TActivationAnnouncementRequest to the second stage (OnVoiceInputSecondAnnouncementRequest).
    NThreading::TFuture<void> OnVoiceInputFirstAnnouncementRequest(NAppHost::TServiceContextPtr ctx);

    // Receives TActivationAnnouncementRequest from first stage apphost-node.
    // Receives spotter validation result from extra apphost context item (TSpotterResponseForActivation).
    // Merges TSpotterFeatures.
    // Responds with TActivationAnnouncementResponse (for MM_RUN) and TActivationFinalRequest (for third stage).
    NThreading::TFuture<void> OnVoiceInputSecondAnnouncementRequest(NAppHost::TServiceContextPtr ctx);

    // Receives TActivationFinalRequest from second stage apphost-node.
    // Runs only after MM_RUN (unused fake item is awaited by apphost).
    // Responds with TActivationFinalResponse.
    NThreading::TFuture<void> OnVoiceInputFinalRequest(NAppHost::TServiceContextPtr ctx);

private:
    TIntrusivePtr<IActivationStorage> Storage;
    TActivationServiceConfig Config;
};


}   // namespace NCachalot
