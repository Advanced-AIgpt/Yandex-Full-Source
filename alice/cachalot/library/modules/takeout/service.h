#pragma once

#include <alice/cachalot/library/modules/takeout/storage.h>

#include <alice/cachalot/library/service.h>

namespace NCachalot {

    class TTakeoutService: public IService {
    public:
        static TMaybe<TTakeoutServiceSettings> FindServiceSettings(const TApplicationSettings& settings);

    public:
        TTakeoutService(const TTakeoutServiceSettings& settings);

        bool Integrate(NAppHost::TLoop& loop, ui16 port) override;

    private:
        void OnTakeoutRequest(const NNeh::IRequestRef& req);

    private:
        TIntrusivePtr<ITakeoutResultsStorage> Storage;
    };

}
