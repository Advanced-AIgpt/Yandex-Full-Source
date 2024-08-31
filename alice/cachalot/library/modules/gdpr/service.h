#pragma once

#include <alice/cachalot/library/modules/gdpr/storage.h>

#include <alice/cachalot/library/service.h>

namespace NCachalot {

    class TGDPRService: public IService {
    public:
        static TMaybe<TGDPRServiceSettings> FindServiceSettings(const TApplicationSettings& settings);

    public:
        TGDPRService(const NCachalot::TGDPRServiceSettings& settings);

        bool Integrate(NAppHost::TLoop& loop, ui16 port) override;

    private:
        void OnGDPRRequest(const NNeh::IRequestRef& req);

    private:
        //NCachalot::TGDPRSettings Settings;
        TIntrusivePtr<IGDPRStorage> Storage;
    };
}
