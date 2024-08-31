#pragma once

#ifndef NDEBUG
    #include "dlog.h"
    #include <util/generic/guid.h>

    // CTX is treated as `const NAppHost::IServiceContext&` defined in 'apphost/api/service/cpp/service.h'
    #define APPHOST_DLOG(CTX, MSG) \
        DLOG_IMPL("[" \
            << GetGuidAsString(CTX.GetRequestID()) << '/' << CTX.GetRUID() \
            << " at " << CTX.GetLocation().Path \
            << "] " << MSG \
            , "APPHOST_DLOG " \
        )
#else
    #define APPHOST_DLOG(CTX, MSG)
#endif
