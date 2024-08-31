#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>
#include <apphost/api/service/cpp/service.h>

namespace NAlice::NCuttlefish::NAppHostServices {
    void BioContextLoadPost(NAppHost::IServiceContext& serviceCtx, TLogContext logContext);
}
