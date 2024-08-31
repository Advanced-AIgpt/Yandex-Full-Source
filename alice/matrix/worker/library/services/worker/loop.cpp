#include "loop.h"

#include <util/generic/guid.h>
#include <util/random/random.h>

namespace NMatrix::NWorker {

namespace {

TDuration GetNextLoopInterval(TDuration minInterval, TDuration maxInterval, TDuration lastSyncDuration) {
    return (minInterval + TDuration::MicroSeconds(
        RandomNumber<TDuration::TValue>((maxInterval - minInterval).MicroSeconds() + 1)
    )) - lastSyncDuration;
}

} // namespace

std::once_flag TWorkerLoop::INIT_METRICS_ONCE_FLAG;

TWorkerLoop::TWorkerLoop(
    const TWorkerLoopSettings& config,
    const NYdb::TDriver& driver,
    TRtLogClient& rtLogClient
)
    : WorkerStorage_(
        driver,
        config.GetYDBClient()
    )
    , RtLogClient_(rtLogClient)

    , IsRunning_(true)

    , ManualMode_(config.GetManualMode())

    , DefaultMinLoopInterval_(FromString<TDuration>(config.GetDefaultMinLoopInterval()))
    , DefaultMaxLoopInterval_(FromString<TDuration>(config.GetDefaultMaxLoopInterval()))

    , MinLoopIntervalForSkippedSync_(FromString<TDuration>(config.GetMinLoopIntervalForSkippedSync()))
    , MaxLoopIntervalForSkippedSync_(FromString<TDuration>(config.GetMaxLoopIntervalForSkippedSync()))

    , SelectLimit_(config.GetSelectLimit())

    , PerActionTimeout_(FromString<TDuration>(config.GetPerActionTimeout()))

    // WARNING: Don't touch this magic '* 2' and '* 3'
    // These constants are specially selected to prevent split brain
    , MaxHeartbeatInactivityPeriodToAcquireLock_(PerActionTimeout_ * 3)
    , MaxHeartbeatInactivityPeriodToReleaseLock_(PerActionTimeout_ * 2)
    , MaxHeartbeatInactivityPeriodToEnsureLeading_(PerActionTimeout_ * 2)

    , MinEnsureShardLockLeadingAndDoHeartbeatPeriod_(FromString<TDuration>(config.GetMinEnsureShardLockLeadingAndDoHeartbeatPeriod()))

    , MatrixNotificatorClientHost_(config.GetMatrixNotificatorClient().GetHost())
    , MatrixNotificatorClientPort_(config.GetMatrixNotificatorClient().GetPort())
    , MatrixNotificatorClientSocketTimeout_(FromString<TDuration>(config.GetMatrixNotificatorClient().GetSocketTimeout()))
    , MatrixNotificatorClientConnectTimeout_(FromString<TDuration>(config.GetMatrixNotificatorClient().GetConnectTimeout()))
{
    std::call_once(
        INIT_METRICS_ONCE_FLAG,
        std::bind(&TWorkerLoop::InitMetrics, this)
    );

    if (!ManualMode_) {
        PerLoopMatrixNotificatorClients_.reserve(config.GetMainLoopThreads());
        for (size_t i = 0; i < config.GetMainLoopThreads(); ++i) {
            PerLoopMatrixNotificatorClients_.emplace_back(
                MatrixNotificatorClientHost_,
                MatrixNotificatorClientPort_,
                MatrixNotificatorClientSocketTimeout_,
                MatrixNotificatorClientConnectTimeout_
            );
        }

        LoopThreads_.resize(config.GetMainLoopThreads());
        for (size_t i = 0; i < config.GetMainLoopThreads(); ++i) {
            LoopThreads_[i] = SystemThreadFactory()->Run(
                std::bind(
                    &TWorkerLoop::Loop,
                    this,
                    std::ref(PerLoopMatrixNotificatorClients_[i])
                )
            );
        }
    }
}

TWorkerLoop::~TWorkerLoop() {
    SyncShutdown();
}

TExpected<TWorkerSync::ESyncStatus, TString> TWorkerLoop::ManualSync(TLogContext logContext) {
    if (!ManualMode_) {
        static const TString error = "Manual sync mode is disabled";
        return error;
    }

    // We need this because TKeepAliveHttpClient is thread unsafe
    // TODO(ZION-217): Do it in a better way
    // NOTE: ManualSync is only needed for tests and will not be used in production
    // so it's "ok" to create new keep alive client for every request
    TKeepAliveHttpClient matrixNotificatorClient(
        MatrixNotificatorClientHost_,
        MatrixNotificatorClientPort_,
        MatrixNotificatorClientSocketTimeout_,
        MatrixNotificatorClientConnectTimeout_
    );

    return Sync(
        logContext.RtLogPtr() ? logContext.RtLogPtr()->GetToken() : CreateGuidAsString(),
        matrixNotificatorClient,
        logContext
    );
}

void TWorkerLoop::SyncShutdown() {
    IsRunning_.store(false, std::memory_order_release);

    if (!LoopThreads_.empty()) {
        {
            TGuard<TMutex> guard(IsRunningCondVarMutex_);
            IsRunningCondVar_.BroadCast();
        }

        for (const auto& threadPtr : LoopThreads_) {
            threadPtr->Join();
        }
        LoopThreads_.clear();
    }
}

void TWorkerLoop::InitMetrics() {
    // Init special metrics to avoid no data in alerts

    TSourceMetrics metrics(NAME);

    // Sync
    metrics.PushRate(0, "sync", "ok");
    metrics.PushRate(0, "sync", "skip");
    metrics.PushRate(0, "sync", "error");
    metrics.PushRate(0, "sync", "exception");
}

void TWorkerLoop::Loop(
    TKeepAliveHttpClient& matrixNotificatorClient
) {
    while (IsRunning_.load(std::memory_order_acquire)) {
        TDuration nextLoopInterval = TDuration::Zero();
        {
            TInstant syncStartTime = TInstant::Now();
            TString syncGuid = CreateGuidAsString();
            TLogContext logContext = TLogContext(
                SpawnLogFrame(),
                RtLogClient_.CreateRequestLogger(syncGuid)
            );
            TSourceMetrics metrics(NAME);

            bool isSyncSkipped = false;
            try {
                if (auto res = Sync(syncGuid, matrixNotificatorClient, logContext); res) {
                    switch (res.Success()) {
                        case TWorkerSync::ESyncStatus::PERFORMED: {
                            metrics.PushRate("sync", "ok");
                            break;
                        }
                        case TWorkerSync::ESyncStatus::SKIPPED: {
                            isSyncSkipped = true;
                            metrics.PushRate("sync", "skip");
                            break;
                        }
                    }
                } else {
                    metrics.PushRate("sync", "error");
                }
            } catch (...) {
                metrics.PushRate("sync", "exception");
                metrics.SetError("exception");
                logContext.LogEventErrorCombo<NEvClass::TMatrixWorkerSyncException>(CurrentExceptionMessage());
            }

            if (isSyncSkipped) {
                nextLoopInterval = GetNextLoopInterval(
                    MinLoopIntervalForSkippedSync_,
                    MaxLoopIntervalForSkippedSync_,
                    TInstant::Now() - syncStartTime
                );
            } else {
                nextLoopInterval = GetNextLoopInterval(
                    DefaultMinLoopInterval_,
                    DefaultMaxLoopInterval_,
                    TInstant::Now() - syncStartTime
                );
            }
            logContext.LogEventInfoCombo<NEvClass::TMatrixWorkerNextLoopInterval>(ToString(nextLoopInterval));
        }

        {
            TGuard<TMutex> guard(IsRunningCondVarMutex_);
            if (IsRunning_.load(std::memory_order_acquire)) {
                IsRunningCondVar_.WaitT(IsRunningCondVarMutex_, nextLoopInterval, [this]() {
                    return !IsRunning_.load(std::memory_order_acquire);
                });
            }
        }
    }
}

TExpected<TWorkerSync::ESyncStatus, TString> TWorkerLoop::Sync(
    const TString& syncGuid,
    TKeepAliveHttpClient& matrixNotificatorClient,
    TLogContext logContext
) {
    return TWorkerSync(
        WorkerStorage_,
        RtLogClient_,
        matrixNotificatorClient,

        syncGuid,

        SelectLimit_,
        PerActionTimeout_,
        MaxHeartbeatInactivityPeriodToAcquireLock_,
        MaxHeartbeatInactivityPeriodToReleaseLock_,
        MaxHeartbeatInactivityPeriodToEnsureLeading_,

        MinEnsureShardLockLeadingAndDoHeartbeatPeriod_
    ).Run(
        logContext
    );
}

} // namespace NMatrix::NWorker
