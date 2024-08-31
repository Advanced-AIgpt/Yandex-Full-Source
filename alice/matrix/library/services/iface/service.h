#pragma once

#include <apphost/api/service/cpp/service_loop.h>

#include <util/generic/noncopyable.h>

namespace NMatrix {

class IService : public TNonCopyable {
public:
    IService();
    virtual ~IService();

    virtual bool Integrate(NAppHost::TLoop& loop, uint16_t port) = 0;

    /**
     * @brief suspend serving requests and start reply with 503 Service Unavailable
     */
    virtual void Suspend() {
        IsSuspended_.store(true, std::memory_order_release);
    }

    /**
     * @brief resume serving requests
     */
    virtual void Resume() {
        IsSuspended_.store(false, std::memory_order_release);
    }

    /**
     * @brief check service is suspended
     */
    virtual bool IsSuspended() {
        return IsSuspended_.load(std::memory_order_acquire);
    }

    /**
     * @brief await all active requests to service
     */
    virtual void SyncShutdown();

    std::atomic<size_t>& GetActiveRequestCounterRef() {
        return ActiveRequestCounter_;
    }

private:
    std::atomic<bool> IsSuspended_;
    std::atomic<size_t> ActiveRequestCounter_;
};

} // namespace NMatrix
