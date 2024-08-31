#include "bass_cache.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/neh/neh.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/threading/future/future.h>

#include <util/generic/maybe.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/system/backtrace.h>
#include <util/system/fs.h>
#include <util/thread/pool.h>

namespace NBASS {

TCacheManager::TCacheManager()
    : IsShutdown(false)
{
    ThreadPool.Start(1 /* thread num */, 1 /* max queue size */);
    LOG(DEBUG) << "Start cache manager" << Endl;
    ThreadPool.SafeAddFunc(std::bind(&TCacheManager::CacheUpdateLoop, this));
}

TCacheManager::~TCacheManager() {
    Shutdown();
}

void TCacheManager::Schedule(TJob job, TDuration initialDelay) {
    with_lock (Lock) {
        JobsQueue.emplace(TInstant::Now() + initialDelay, TJobDescr{job, initialDelay});
        CondVar.BroadCast();
    }
}

void TCacheManager::CacheUpdateLoop() {
    while (!AtomicGet(IsShutdown)) {
        const TInstant now = TInstant::Now();
        TMaybe<TJobDescr> jobDescr;
        with_lock (Lock) {
            auto it = JobsQueue.cbegin();
            if (JobsQueue.cend() == it) {
                CondVar.Wait(Lock);
                continue;
            }

            if (now < it->first) {
                CondVar.WaitD(Lock, it->first);
                continue;
            }

            jobDescr = it->second;
            JobsQueue.erase(it);
        }

        Y_ASSERT(jobDescr);

        TDuration timeout = jobDescr->LastTimeout;
        try {
            timeout = jobDescr->JobCallback();
        }
        catch (...) {
            LOG(ERR) << TStringBuf("Exception during update scheduled job; refresh time: ") << timeout << Endl;
            TString bt;
            TStringOutput btStream(bt);
            FormatBackTrace(&btStream);
            LOG(ERR) << bt << Endl;
        }

        // If timeout is zero it means that the job wanted to stop or
        // exception is happened for the first run which also means
        // that the job must be removed from the schedule.
        if (timeout) {
            with_lock (Lock) {
                JobsQueue.emplace(TInstant::Now() + timeout, TJobDescr{jobDescr->JobCallback, timeout});
            }
        }
    }
}

void TCacheManager::WaitFirstRun() {
    TAtomicSharedPtr<NThreading::TPromise<void>> delay = new NThreading::TPromise<void>(NThreading::NewPromise());
    auto finalizeJob = [delay] {
        delay->SetValue();
        return TDuration::MicroSeconds(0);
    };

    Schedule(finalizeJob, TDuration::Zero());
    delay->GetFuture().Wait();
}

void TCacheManager::Shutdown() {
    AtomicSet(IsShutdown, true);
    with_lock (Lock) {
        CondVar.Signal();
    }
    ThreadPool.Stop();
}

TAtomicSharedPtr<const TCacheData> TCacheManager::GetData(const TStringBuf dataName) {
    with_lock (Lock) {
        return CacheData[dataName];
    }
}

} // namespace NBASS
