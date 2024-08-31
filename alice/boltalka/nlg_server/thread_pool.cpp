#include "thread_pool.h"

namespace NNlgServer {

namespace {

class TEnumeratingJobStarter : public IObjectInQueue {
public:
    TEnumeratingJobStarter(TThreadJobWithId job, TLockFreeQueue<int> *freeWorkerIds)
        : Job(job)
        , FreeWorkerIds(freeWorkerIds)
    {
    }

    void Process(void*) override {
        THolder<TEnumeratingJobStarter> This(this);
        int workerId;
        bool success = FreeWorkerIds->Dequeue(&workerId);
        Y_VERIFY(success, "Logic error");
        Job(workerId);
        FreeWorkerIds->Enqueue(workerId);
    }

private:
    TThreadJobWithId Job;
    TLockFreeQueue<int> *FreeWorkerIds;
};

}

TThreadPool::TThreadPool(int threadCount)
    : Pool(::TThreadPool::TParams().SetBlocking(true).SetCatching(true))
{
    Pool.Start(threadCount, threadCount * 2);
    for (int workerId = 0; workerId < threadCount; ++workerId) {
        FreeWorkerIds.Enqueue(workerId);
    }
}

void TThreadPool::Add(TThreadJobWithId job) {
    bool success = Pool.Add(new TEnumeratingJobStarter(job, &FreeWorkerIds));
    Y_VERIFY(success, "Queue is full");
}

}

