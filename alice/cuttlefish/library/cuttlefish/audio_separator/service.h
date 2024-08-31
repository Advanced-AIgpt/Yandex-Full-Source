#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NCuttlefish::NAppHostServices {

NThreading::TPromise<void> AudioSeparator(NAppHost::TServiceContextPtr ctx, TLogContext logContext);

}  // namespace NAlice::NCuttlefish::NAppHostServices

