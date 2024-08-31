#pragma once

#include <apphost/api/service/cpp/service_loop.h>
#include <alice/cachalot/library/config/application.cfgproto.pb.h>

#include <type_traits>


namespace NPrivate {

    template <typename TService>
    THolder<TService> MakeServicePtr(const NCachalot::TApplicationSettings& settings) {
        const auto serviceSettingsMaybe = TService::FindServiceSettings(settings);
        if (serviceSettingsMaybe.Defined() && serviceSettingsMaybe.GetRef().Enabled()) {
            return MakeHolder<TService>(serviceSettingsMaybe.GetRef());
        }
        return nullptr;
    }

}


namespace NCachalot {

class IService {
public:
    virtual ~IService() = default;

    virtual bool Integrate(NAppHost::TLoop& loop, uint16_t port) = 0;

    /**
     * @brief suspend serving requests and start reply with 503 Service Unavailable
     */
    virtual void Suspend() {
        AtomicSet(Status, 0);
    }

    /**
     * @brief resume serving requests
     */
    virtual void Resume() {
        AtomicSet(Status, 1);
    }

    /**
     * @brief check service is suspended
     */
    virtual bool IsSuspended() {
        return AtomicGet(Status) == 0;
    }

private:
    TAtomic Status { 0 };
};


template <class TService, class ... TServices>
class TServiceIntegratorImpl
    : public TServiceIntegratorImpl<TService>
    , public TServiceIntegratorImpl<TServices...>
{
public:
    explicit TServiceIntegratorImpl(const TApplicationSettings& settings)
        : TServiceIntegratorImpl<TService>(settings)
        , TServiceIntegratorImpl<TServices...>(settings)
    { }

    void Integrate(NAppHost::TLoop& loop, uint16_t port) {
        TServiceIntegratorImpl<TService>::Integrate(loop, port);
        TServiceIntegratorImpl<TServices...>::Integrate(loop, port);
    }

    void Suspend() {
        TServiceIntegratorImpl<TService>::Suspend();
        TServiceIntegratorImpl<TServices...>::Suspend();
    }

    void Resume() {
        TServiceIntegratorImpl<TService>::Resume();
        TServiceIntegratorImpl<TServices...>::Resume();
    }
};


template <class TService>
class TServiceIntegratorImpl<TService> {
public:
    explicit TServiceIntegratorImpl(const TApplicationSettings& settings)
        : Service(::NPrivate::MakeServicePtr<TService>(settings))
    {
    }

    void Integrate(NAppHost::TLoop& loop, uint16_t port) {
        if (Service != nullptr) {
            Service->Integrate(loop, port);
        }
    }

    void Suspend() {
        if (Service != nullptr) {
            Service->Suspend();
        }

    }

    void Resume() {
        if (Service != nullptr) {
            Service->Resume();
        }
    }

private:
    THolder<TService> Service = nullptr;
};


template <class ... TServices>
class TServiceIntegrator : public TServiceIntegratorImpl<TServices...> {
public:
    TServiceIntegrator(NAppHost::TLoop& loop, uint16_t port, const TApplicationSettings& settings)
        : TServiceIntegratorImpl<TServices...>(settings)
    {
        TServiceIntegratorImpl<TServices...>::Integrate(loop, port);
        TServiceIntegratorImpl<TServices...>::Resume();
    }

    ~TServiceIntegrator() { }
};


}   // namespace NCachalot
