#pragma once

#include <alice/cachalot/library/modules/cache/request.h>
#include <alice/cachalot/library/modules/cache/storage.h>
#include <alice/cachalot/library/modules/cache/storage_tag_to_context.h>

#include <alice/cachalot/library/service.h>
#include <util/generic/ptr.h>
#include <util/thread/pool.h>


namespace NCachalot {

class TCacheService : public IService {
public:
    static TMaybe<TCacheServiceSettings> FindServiceSettings(const TApplicationSettings& settings);

public:
    TCacheService(const TCacheServiceSettings& settings);

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) override;

private:
    NThreading::TFuture<void> OnHttpRequest(const NNeh::IRequestRef& request);

    NThreading::TFuture<void> OnApphostGetRequest(NAppHost::TServiceContextPtr ctx);

    NThreading::TFuture<void> OnApphostSetRequest(NAppHost::TServiceContextPtr ctx);

    NThreading::TFuture<void> OnApphostDeleteRequest(NAppHost::TServiceContextPtr ctx);

    template<typename TProtoRequest, typename TRequestFactory>
    NThreading::TFuture<void> UnwrapApphostRequest(NAppHost::TServiceContextPtr ctx, TRequestFactory& requestFactory);

    NThreading::TFuture<void> ServeRequest(TRequestPtr req, NAppHost::TServiceContextPtr ctx, TString requestItemTag);

private:
    TStorageTag2Context StorageTag2Context;
};


}   // namespace NCachalot
