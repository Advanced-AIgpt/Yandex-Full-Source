#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>

#include <library/cpp/neh/multiclient.h>


namespace NAlice::NCuttlefish::NAppHostServices {


using THttpRequestCallback = std::function<void(NNeh::TResponseRef)>;

void SendHttpRequest(const NNeh::TMessage& msg, const TDuration timeout, unsigned additionalAttempts, THttpRequestCallback&& callback, TLogContext logContext, int subrequestId);

}  // namespace NAlice::NCuttlefish::NAppHostServices
