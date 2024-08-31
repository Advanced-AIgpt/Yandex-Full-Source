#pragma once

#include "metrics_http_request_base.h"

#include <ydb/public/sdk/cpp/client/extensions/solomon_stats/pull_connector.h>

namespace NMatrix {

class TYDBMetricsHttpRequest : public TMetricsHttpRequestBase<
    NEvClass::TMatrixYDBMetricsHttpRequestData,
    NEvClass::TMatrixYDBMetricsHttpResponseData
> {
public:
    TYDBMetricsHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        const NMonitoring::TMetricRegistry& metricRegistry
    );

protected:
    void GetMetricsJson(TStringStream& outStream) override final;
    void GetMetricsSpack(TStringStream& outStream) override final;

public:
    static inline constexpr TStringBuf NAME = "ydb_sensors";
    static inline constexpr TStringBuf PATH = "/ydb_sensors";

private:
    const NMonitoring::TMetricRegistry& MetricRegistry_;
};

} // namespace NMatrix
