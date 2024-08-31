#include "service.h"

namespace NMatrix {

IService::IService()
    : IsSuspended_(true)
    , ActiveRequestCounter_(0)
{}

IService::~IService() {
    // We can't just call SyncShutdown in destructor of base class because
    // all fields of derived class are destroyed
    // So someone must gurantee that SyncShutdown was called before destructor
}

void IService::SyncShutdown() {
    while (ActiveRequestCounter_.load() > 0) {
        // We guarantee that service will be alive longer than all requests to this service
        // We can't store refs to all requests because we want to have lockfree request processing
        // so we just wait when ActiveRequestCounter_ will become equal to zero

        // This code is executed only on shutdown, so it's ok to just sleep here
        // If we don't wait, random coredump possible will be occur
        // TODO(ZION-50): Add TAbortWatchDog
        Sleep(TDuration::Seconds(1));
    }
}

} // namespace NMatrix
