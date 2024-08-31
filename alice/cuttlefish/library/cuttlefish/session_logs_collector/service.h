#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    /**
    *  @brief collects all possible directives with session logs and forwards them
    */
    void SessionLogsCollector(NAppHost::IServiceContext& serviceCtx, TLogContext logContext);

}  // namespace NAlice::NCuttlefish::NAppHostServices
