#include "log.h"

#include <atomic>

Y_THREAD(ui64) TLogging::ReqId = 0;

// static
void TLogging::InitTlsUniqId() {
    static std::atomic<ui64> counter = 0;
    TLogging::ReqId = counter++;
}

