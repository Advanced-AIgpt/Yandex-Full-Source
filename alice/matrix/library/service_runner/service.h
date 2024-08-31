#pragma once

#include <alice/matrix/library/rtlog/rtlog.h>

#include <apphost/api/service/cpp/service_loop.h>

#include <ydb/public/sdk/cpp/client/ydb_driver/driver.h>

namespace NMatrix {

template <typename TService, typename ...TServices>
class TServiceIntegratorImpl
    : public TServiceIntegratorImpl<TService>
    , public TServiceIntegratorImpl<TServices...>
{
public:
    template <typename TServicesCommonContext>
    explicit TServiceIntegratorImpl(const TServicesCommonContext& servicesCommonContext)
        : TServiceIntegratorImpl<TService>(servicesCommonContext)
        , TServiceIntegratorImpl<TServices...>(servicesCommonContext)
    {}

    bool Integrate(NAppHost::TLoop& loop, uint16_t port) {
        return TServiceIntegratorImpl<TService>::Integrate(loop, port)
            && TServiceIntegratorImpl<TServices...>::Integrate(loop, port);
    }

    void Suspend() {
        TServiceIntegratorImpl<TService>::Suspend();
        TServiceIntegratorImpl<TServices...>::Suspend();
    }

    void Resume() {
        TServiceIntegratorImpl<TService>::Resume();
        TServiceIntegratorImpl<TServices...>::Resume();
    }

    void SyncShutdown() {
        TServiceIntegratorImpl<TService>::SyncShutdown();
        TServiceIntegratorImpl<TServices...>::SyncShutdown();
    }
};

template <typename TService>
class TServiceIntegratorImpl<TService> : public TService {
public:
    template <typename TServicesCommonContext>
    explicit TServiceIntegratorImpl(const TServicesCommonContext& servicesCommonContext)
        : TService(servicesCommonContext)
    {}
};

template <typename ...TServices>
class TServiceIntegrator : public TServiceIntegratorImpl<TServices...> {
public:
    template <typename TServicesCommonContext>
    TServiceIntegrator(
        NAppHost::TLoop& loop,
        uint16_t port,
        const TServicesCommonContext& servicesCommonContext
    )
        : TServiceIntegratorImpl<TServices...>(servicesCommonContext)
        , Driver_(servicesCommonContext.YDBDriver)
    {
        TServiceIntegratorImpl<TServices...>::Integrate(loop, port);
        TServiceIntegratorImpl<TServices...>::Resume();
    }

    ~TServiceIntegrator() {
        TServiceIntegratorImpl<TServices...>::SyncShutdown();
        Driver_.Stop(true /* wait */);
    }

private:
    NYdb::TDriver& Driver_;
};

} // namespace NMatrix
