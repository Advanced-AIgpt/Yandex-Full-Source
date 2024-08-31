#include "ydb_metrics_http_request.h"

#include <library/cpp/monlib/encode/json/json.h>
#include <library/cpp/monlib/encode/spack/spack_v1.h>

namespace NMatrix {

TYDBMetricsHttpRequest::TYDBMetricsHttpRequest(
    std::atomic<size_t>& requestCounterRef,
    TRtLogClient& rtLogClient,
    const NNeh::IRequestRef& request,
    const NMonitoring::TMetricRegistry& metricRegistry
)
    : TMetricsHttpRequestBase<
        NEvClass::TMatrixYDBMetricsHttpRequestData,
        NEvClass::TMatrixYDBMetricsHttpResponseData
    >(
        NAME,
        requestCounterRef,
        rtLogClient,
        request
    )
    , MetricRegistry_(metricRegistry)
{}

void TYDBMetricsHttpRequest::GetMetricsJson(TStringStream& outStream) {
    NMonitoring::IMetricEncoderPtr encoder = NMonitoring::EncoderJson(&outStream);
    MetricRegistry_.Accept(TInstant::Zero(), encoder.Get());
    encoder->Close();
}

void TYDBMetricsHttpRequest::GetMetricsSpack(TStringStream& outStream) {
    NMonitoring::IMetricEncoderPtr encoder = NMonitoring::EncoderSpackV1(&outStream, NMonitoring::ETimePrecision::SECONDS, NMonitoring::ECompression::LZ4);
    MetricRegistry_.Accept(TInstant::Zero(), encoder.Get());
    encoder->Close();
}

} // namespace NMatrix