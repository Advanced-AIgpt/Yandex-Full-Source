#pragma once

#include "sensors.h"
#include "tvm.h"

#include <alice/personal_cards/config/config.pb.h>
#include <alice/personal_cards/push_cards_storage/push_cards_storage.h>

#include <library/cpp/http/server/http.h>

#include <util/system/rwlock.h>

namespace NPersonalCards {

class TServer;
class TConfig;

// Singleton class for keeping all application wide things like
// HTTP server, thread pool, counters, etc.
class TApplication : public THttpServer::ICallBack {
public:
    // Get current global instance.
    static TApplication* GetInstance() {
        return Instance_;
    }

    // Initialize singleton, should be called from main.
    static TApplication* Init(int argc, const char** argv);

    // Run loop and return exit code.
    int Run();

    // Initiate gracefully shutdown, loop will terminate soon.
    void Shutdown();

    // Return true if graceful shutdown was initiated.
    bool IsShutdown() const {
        return IsShutdownInitiated_;
    }

    // Return threading pool for the app, graceful shutdown waits for all threads in the pool.
    IThreadPool& GetThreadPool() {
        return *MtpQueue_;
    }

    // Return cards server.
    TServer* GetCardsServer() {
        return CardsServer_.Get();
    }

    TTvmClientPtr GetTvmClient() {
        {
            bool tvmClientIsNull = false;
            {
                TReadGuardBase<TLightRWLock> readGuard(TvmClientCreationLock_);
                tvmClientIsNull = (TvmClient_ == nullptr);
            }
            if (tvmClientIsNull) {
                TWriteGuardBase<TLightRWLock> writeGuard(TvmClientCreationLock_);
                if (!TvmClient_) {
                    TvmClient_ = CreateTvmClient(Config_.GetTvm(), false /* withRetries */);
                }
            }
        }

        Y_ENSURE(TvmClient_, "TvmClient is null.");

        return TvmClient_;
    }

    const THashSet<ui32>& GetTrustedServices() const {
        return TrustedServices_;
    }

    // THttpServer::ICallBack methods:
    TClientRequest* CreateClient() override;

    void OnMaxConn() override;

    std::atomic<size_t>& GetParallelHttpRequestCountRef() {
        return ParallelHttpRequestCount_;
    }

    void UpdateMaxParallelHttpRequestCountInWindow(const size_t newCount) {
        size_t prevCount = MaxParallelHttpRequestCountInWindow_;
        while (prevCount < newCount && !MaxParallelHttpRequestCountInWindow_.compare_exchange_weak(prevCount, newCount)) {
        }
    }

    TDuration GetRequestTimeLimit() const {
        return RequestTimeLimit_;
    }

    void BeforeSensors() {
        PushCardsStorage_->UpdateSensors();
        UpdateMaxParallelHttpRequestCountInWindowSensor();
    }

private:
    void InitSensors();

    ui32 GetMonitoringPort() const {
        return Config_.GetHttpServer().GetPort() + 1;
    }

    void UpdateMaxParallelHttpRequestCountInWindowSensor() {
        static NInfra::TIntGaugeSensor gaugeSensor(SENSOR_GROUP, "parallel_http_request_count");
        size_t currentMaxParallelHttpRequestCountInWindow = MaxParallelHttpRequestCountInWindow_.exchange(ParallelHttpRequestCount_.load());
        gaugeSensor.Set(currentMaxParallelHttpRequestCountInWindow);
    }

    TApplication(TConfig&& config);
    ~TApplication();

private:
    static TApplication* Instance_;

    const TConfig Config_;
    THttpServer::TMtpQueueRef MtpQueue_;
    THolder<THttpServer> HttpServer_;
    std::atomic<bool> IsShutdownInitiated_;
    THolder<TServer> CardsServer_;
    TPushCardsStoragePtr PushCardsStorage_;
    std::atomic<size_t> ParallelHttpRequestCount_;
    std::atomic<size_t> MaxParallelHttpRequestCountInWindow_;
    TTvmClientPtr TvmClient_;
    TLightRWLock TvmClientCreationLock_;
    const THashSet<ui32> TrustedServices_;
    const TDuration RequestTimeLimit_;
};

} // namespace NPersonalCards
