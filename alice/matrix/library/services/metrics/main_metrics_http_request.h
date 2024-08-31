#pragma once

#include "metrics_http_request_base.h"

namespace NMatrix {

class TMainMetricsHttpRequest : public TMetricsHttpRequestBase<
    NEvClass::TMatrixMainMetricsHttpRequestData,
    NEvClass::TMatrixMainMetricsHttpResponseData
> {
public:
    TMainMetricsHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request
    );

protected:
    void GetMetricsJson(TStringStream& outStream) override final;
    void GetMetricsSpack(TStringStream& outStream) override final;

public:
    static inline constexpr TStringBuf NAME = "sensors";
    static inline constexpr TStringBuf PATH = "/sensors";
};

} // namespace NMatrix
