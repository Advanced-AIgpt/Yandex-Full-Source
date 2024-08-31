#pragma once

#include <alice/cachalot/library/modules/vins_context/request.h>
#include <alice/cachalot/library/modules/vins_context/storage.h>

#include <alice/cachalot/library/service.h>

/*
    This service is using only for GDPR
    For other purposes, use megamind_session
*/


namespace NCachalot {

    class TVinsContextService: public IService {
    public:
        static TMaybe<TVinsContextServiceSettings> FindServiceSettings(const TApplicationSettings& settings);

    public:
        TVinsContextService(const TVinsContextServiceSettings& settings);

        bool Integrate(NAppHost::TLoop& loop, ui16 port) override;

    private:
        void OnHttpRequest(const NNeh::IRequestRef& request);

    private:
        TIntrusivePtr<IKvStorage<TVinsContextKey, TString, TVinsExtraData>> Storage;
    };

    TVinsContextKey ConstructKeyFromRequest(const NCachalotProtocol::TVinsContextRequest& request);

}
