#pragma once

#include "sd_client.h"

#include <alice/matrix/library/request/http_request.h>

#include <apphost/lib/transport/transport_neh_backend.h>

#include <library/cpp/neh/asio/asio.h>
#include <library/cpp/neh/asio/executor.h>

namespace NMatrix::NNotificator {

class TProxyHttpRequest : public THttpRequest<
    NEvClass::TMatrixNotificatorProxyHttpRequestData,
    NEvClass::TMatrixNotificatorProxyHttpResponseData
> {
public:
    TProxyHttpRequest(
        std::atomic<size_t>& requestCounterRef,
        TRtLogClient& rtLogClient,
        const NNeh::IRequestRef& request,
        const TString& destinationServiceName,
        const THolder<TSDClientBase>& sdClient,
        const TDuration timeout,
        NAppHost::NTransport::TNehCommunicationSystem& ncs,
        NAsio::TIOService& deadlineTimersIOService
    );

    NThreading::TFuture<void> ServeAsync() override;

private:
    TReply GetReply() const override;

public:
    static inline constexpr TStringBuf NAME = "proxy";

private:
    const TDuration Timeout_;
    NNeh::TMessage Message_;
    TRtLogActivation ProxyRequestRTLogActivation_;

    TString DataFromBackend_;
    THttpHeaders HeadersFromBackend_;
    int CodeFromBackend_;
    NAppHost::NTransport::TNehCommunicationSystem& Ncs_;
    NAsio::TDeadlineTimer DeadlineTimer_;
};

} // namespace NMatrix::NNotificator
