#pragma once

#include <alice/cachalot/library/modules/yabio_context/request.h>
#include <alice/cachalot/library/modules/yabio_context/storage.h>

#include <alice/cachalot/library/service.h>

namespace NCachalot {

    class TYabioContextService: public IService {
    public:
        static TMaybe<TYabioContextServiceSettings> FindServiceSettings(const TApplicationSettings& settings);

    public:
        TYabioContextService(const TYabioContextServiceSettings& settings);

        bool Integrate(NAppHost::TLoop& loop, ui16 port) override;

    private:
        template <typename TRequestSource>
        NThreading::TFuture<void> OnRequest(const TRequestSource& requestSource);

    private:
        TIntrusivePtr<IKvStorage<TYabioContextKey, TString>> Storage;
    };

}
