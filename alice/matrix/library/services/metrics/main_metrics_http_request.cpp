#include "main_metrics_http_request.h"

#include <infra/libs/sensors/sensor_registry.h>

namespace NMatrix {

TMainMetricsHttpRequest::TMainMetricsHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request
)
    : TMetricsHttpRequestBase<
        NEvClass::TMatrixMainMetricsHttpRequestData,
        NEvClass::TMatrixMainMetricsHttpResponseData
    >(
        NAME,
        requestCounterRef,
        rtLogClient,
        request
    )
{}

void TMainMetricsHttpRequest::GetMetricsJson(TStringStream& outStream) {
    NInfra::SensorRegistryPrint(outStream, NInfra::ESensorsSerializationType::JSON);
}

void TMainMetricsHttpRequest::GetMetricsSpack(TStringStream& outStream) {
    NInfra::SensorRegistryPrint(outStream, NInfra::ESensorsSerializationType::SPACK_V1);
}

} // namespace NMatrix