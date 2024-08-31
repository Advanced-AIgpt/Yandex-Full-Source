#pragma once

#include <util/datetime/base.h>
#include <util/generic/function.h>


class IScheduler {
public:
    virtual ~IScheduler() = default;

    /**
     * Function which is used in scheduler to run smth from time to time.
     * Callback returns duration which is used to schedule next run.
     * If exception happens it gets the previous call duration.
     * Hovewer, if duration is zero then function stops scheduling.
     */
    using TJob = std::function<TDuration()>;

    /**
     * Add job to schedule. It first time it runs after <code>initialDelay</code>.
     * @param[in] job function to which do the job and returns refresh time
     * @param[in] initialDelay initial delay to run the job
     */
    virtual void Schedule(TJob job, TDuration initialDelay = TDuration::Zero()) = 0;

    /**
     * It adds the task and wait until its done. So we can definitelly wait
     * until all the current tasks are finished
     */
    virtual void WaitFirstRun() = 0;

    virtual void Shutdown() = 0;
};
