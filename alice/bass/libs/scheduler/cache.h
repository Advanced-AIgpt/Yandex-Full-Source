#pragma once

#include "scheduler.h"

#include <library/cpp/scheme/scheme.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/condvar.h>
#include <util/thread/pool.h>


namespace NBASS {

static const TDuration CACHE_EXPIRE_TIMEOUT = TDuration::Seconds(3600);

class TCacheData {
public:
    TCacheData(NSc::TValue&& value)
        : Value(std::move(value))
    {
        UpdateTime = TInstant::Now();
    }

    bool IsExpired() const {
        return TInstant::Now() - UpdateTime > CACHE_EXPIRE_TIMEOUT;
    }

    const NSc::TValue& GetValue() const {
        return Value;
    }

private:
    TInstant UpdateTime;
    NSc::TValue Value;
};


class TCacheManager final : public IScheduler {
public:
    // Creates and starts manager.
    TCacheManager();
    ~TCacheManager();

    TAtomicSharedPtr<const TCacheData> GetData(const TStringBuf dataName);

    // IScheduler overrides:
    void Schedule(TJob job, TDuration initialDelay = TDuration::Zero()) override;
    void WaitFirstRun() override;
    void Shutdown() override;

private:
    // Main loop, which works in the thread.
    void CacheUpdateLoop();

private:
    struct TJobDescr {
        TJob JobCallback;
        TDuration LastTimeout;
    };

    using TScheduledQueue = TMultiMap<TInstant, TJobDescr>;

private:
    TMutex Lock;
    TCondVar CondVar;
    TAtomic IsShutdown;
    THashMap<TString, TAtomicSharedPtr<TCacheData>> CacheData;

    TScheduledQueue JobsQueue;
    TSimpleThreadPool ThreadPool;
};

} // namespace NBASS
