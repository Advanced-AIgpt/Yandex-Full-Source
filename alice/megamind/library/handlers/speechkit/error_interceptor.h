#pragma once

#include <alice/megamind/library/requestctx/common.h>
#include <alice/megamind/library/handlers/utils/http_response.h>

#include <alice/library/logger/logger.h>
#include <alice/library/metrics/sensors.h>

#include <functional>

namespace NAlice::NMegamind {

class TResponseErrorInterceptor {
public:
    TResponseErrorInterceptor(IHttpResponse& httpResponse);

    TResponseErrorInterceptor& EnableSensors(NMetrics::ISensors& sensors);
    TResponseErrorInterceptor& EnableLogging(TRTLogger& logger);
    TResponseErrorInterceptor& SetNetLocation(TStringBuf netLocation);
    TResponseErrorInterceptor& SetLabel(TStringBuf label);

    void ProcessExecutor(std::function<TStatus()> executor);

private:
    IHttpResponse& HttpResponse_;
    TString Label_;
    TString NetLocation_;
    NMetrics::ISensors* Sensors_ = nullptr;
    TRTLogger* Logger_ = nullptr;
};

} // namespace NAlice::NMegamind
