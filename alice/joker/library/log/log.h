#pragma once

#include <library/cpp/logger/global/global.h>

#include <util/system/tls.h>

struct TLogging {
    static void InitTlsUniqId();

    static Y_THREAD(ui64) ReqId;
};

#define LOG(level) level##_LOG << '<' << TLogging::ReqId.Get() << "> "
