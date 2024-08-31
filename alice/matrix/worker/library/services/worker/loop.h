#pragma once

#include "sync.h"

#include <alice/matrix/worker/library/config/config.pb.h>
#include <alice/matrix/worker/library/storages/worker/storage.h>

#include <library/cpp/http/simple/http_client.h>

#include <util/system/condvar.h>
#include <util/thread/factory.h>

namespace NMatrix::NWorker {

class TWorkerLoop {
public:
    TWorkerLoop(
        const TWorkerLoopSettings& config,
        const NYdb::TDriver& driver,
        TRtLogClient& rtLogClient
    );
    ~TWorkerLoop();

    TExpected<TWorkerSync::ESyncStatus, TString> ManualSync(TLogContext logContext);

    // Thread unsafe but it is ok to call this method twice or more,
    // all calls starting from the second one will not do anything
    void SyncShutdown();

private:
    void InitMetrics();

    void Loop(
        TKeepAliveHttpClient& matrixNotificatorClient
    );

    TExpected<TWorkerSync::ESyncStatus, TString> Sync(
        const TString& syncGuid,
        TKeepAliveHttpClient& matrixNotificatorClient,
        TLogContext logContext
    );

private:
    static inline constexpr TStringBuf NAME = "worker_loop";
    static std::once_flag INIT_METRICS_ONCE_FLAG;

    TWorkerStorage WorkerStorage_;
    TRtLogClient& RtLogClient_;

    std::atomic<bool> IsRunning_;
    TCondVar IsRunningCondVar_;
    TMutex IsRunningCondVarMutex_;
    TVector<THolder<IThreadFactory::IThread>> LoopThreads_;
    // We need this because TKeepAliveHttpClient is thread unsafe
    TVector<TKeepAliveHttpClient> PerLoopMatrixNotificatorClients_;

    const bool ManualMode_;

    const TDuration DefaultMinLoopInterval_;
    const TDuration DefaultMaxLoopInterval_;
    const TDuration MinLoopIntervalForSkippedSync_;
    const TDuration MaxLoopIntervalForSkippedSync_;

    const ui64 SelectLimit_;

    const TDuration PerActionTimeout_;

    const TDuration MaxHeartbeatInactivityPeriodToAcquireLock_;
    const TDuration MaxHeartbeatInactivityPeriodToReleaseLock_;
    const TDuration MaxHeartbeatInactivityPeriodToEnsureLeading_;

    const TDuration MinEnsureShardLockLeadingAndDoHeartbeatPeriod_;

    // TODO(ZION-217) Add a separate class for matrix notificator client
    const TString MatrixNotificatorClientHost_;
    const ui32 MatrixNotificatorClientPort_;
    const TDuration MatrixNotificatorClientSocketTimeout_;
    const TDuration MatrixNotificatorClientConnectTimeout_;
};

} // namespace NMatrix::NWorker
