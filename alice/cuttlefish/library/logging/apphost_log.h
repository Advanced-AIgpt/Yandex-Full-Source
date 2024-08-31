#pragma once

#include "log_context.h"

namespace NAlice::NCuttlefish {

    void TryGetLogOptionsFromAppHostContext(NAppHost::IServiceContext&, TLogContext::TOptions&);
    TLogContext LogContextFor(NAppHost::IServiceContext&, NAlice::NCuttlefish::TRtLogClient*);

}
