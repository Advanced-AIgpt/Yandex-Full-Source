#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>

#include <apphost/api/service/cpp/service.h>

#include <util/generic/strbuf.h>


namespace NAlice::NCuttlefish::NAppHostServices {

void AnyInputPre(NAppHost::IServiceContext& serviceCtx, TLogContext logContext);

}  // namespace NAlice::NCuttlefish::NAppHostServices
