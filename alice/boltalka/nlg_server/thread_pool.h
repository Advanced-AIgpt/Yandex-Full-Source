#pragma once
#include <util/thread/pool.h>
#include <util/thread/lfqueue.h>

namespace NNlgServer {

using TThreadJobWithId = std::function<void(int)>;

class TThreadPool {
public:
    TThreadPool(int threadCount);
    void Add(TThreadJobWithId job);

private:
    TLockFreeQueue<int> FreeWorkerIds;
    ::TThreadPool Pool;
};

}

