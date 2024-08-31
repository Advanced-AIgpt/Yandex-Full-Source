#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    /**
    *  @brief prepares requests, that require blackbox response to be constructed
    */
    void GuestContextLoadBlackboxSetdown(NAppHost::IServiceContext &serviceCtx, TLogContext logContext);

}  // namespace NAlice::NCuttlefish::NAppHostServices
