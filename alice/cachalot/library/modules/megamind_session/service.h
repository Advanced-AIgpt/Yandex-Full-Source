#pragma once

#include <alice/cachalot/library/modules/megamind_session/storage.h>

#include <alice/cachalot/library/service.h>
#include <alice/cachalot/library/request.h>


namespace NCachalot {

class TMegamindSessionService : public IService {
public:
    static TMaybe<TMegamindSessionServiceSettings> FindServiceSettings(const TApplicationSettings& settings);

public:
    TMegamindSessionService(const TMegamindSessionServiceSettings& settings);

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    NThreading::TFuture<NCachalotProtocol::TResponse> OnRequest(
        NCachalotProtocol::TMegamindSessionRequest request, const TInstant startTime,
        TMegamindSessionSubMetrics* metrics, TChroniclerPtr logFrame);

    NThreading::TFuture<void> OnApphostRequest(NAppHost::TServiceContextPtr ctx);
    void OnHttpRequest(const NNeh::IRequestRef& request);

    template <typename TProtoRequest>
    TIntrusivePtr<IKvStorage<TMegamindSessionKey, TMegamindSessionData>> SelectStorageForRequest(
        const TProtoRequest& request
    ) const;

private:
    TIntrusivePtr<TMegamindSessionStorage> Storage;

    friend class TRequestMegamindSession;
};

// For HTTP requests
class TRequestMegamindSession : public TRequest {
public:
    TRequestMegamindSession(const NNeh::IRequestRef& req, TMegamindSessionService& service);
    TAsyncStatus ServeAsync() override;

private:
    TMegamindSessionService& Service;
};

TMegamindSessionKey ConstructKeyFromRequest(const NCachalotProtocol::TMegamindSessionRequest& request);

}   // namespace NCachalot
