#include "hw_service.h"

#include <alice/hollywood/library/dispatcher/common_handles/util/util.h>
#include <alice/hollywood/library/hw_service_context/context.h>
#include <alice/hollywood/library/metrics/metrics.h>


namespace NAlice::NHollywood {

namespace {

void UpdateHwServiceSensors(THwServiceContext& context, const IHwServiceHandle& handle,
    const ERequestResult requestResult, const i64 timeMs) {

    context.GlobalContext().Sensors().AddHistogram(HwServiceResponseTime(handle, requestResult), timeMs, NMetrics::TIME_INTERVALS);
    context.GlobalContext().Sensors().AddHistogram(HwServiceResponseTime(handle, ERequestResult::TOTAL), timeMs, NMetrics::TIME_INTERVALS);

    context.GlobalContext().Sensors().IncRate(HwServiceResponse(handle, requestResult));
    context.GlobalContext().Sensors().IncRate(HwServiceResponse(handle, ERequestResult::TOTAL));
    LOG_INFO(context.Logger()) << "Hw service " << handle.Name() << " finished in " << timeMs
        << " ms, result: " << requestResult;
}

} // namespace

void DispatchHwServiceHandle(const IHwServiceHandle& handle,
    TGlobalContext& globalContext,
    NAppHost::IServiceContext& ctx) {

    TInstant start = TInstant::Now();

    const auto appHostParams = GetAppHostParams(ctx);
    auto logger = CreateLogger(globalContext, GetRTLogToken(appHostParams, ctx.GetRUID()));

    THwServiceContext hwServiceContext{globalContext, ctx, logger};

    try {
        handle.Do(hwServiceContext);
        UpdateHwServiceSensors(hwServiceContext, handle, ERequestResult::SUCCESS, (TInstant::Now() - start).MilliSeconds());
    } catch (...) {
        UpdateHwServiceSensors(hwServiceContext, handle, ERequestResult::ERROR, (TInstant::Now() - start).MilliSeconds());
        LOG_ERROR(hwServiceContext.Logger()) << "hw service " << handle.Name()
            << " has failed with exception, " << CurrentExceptionMessage();
        throw;
    }
}

} // namespace NAlice::NHollywood
