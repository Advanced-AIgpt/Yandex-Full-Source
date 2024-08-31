#pragma once

#include <alice/matrix/library/config/config.pb.h>
#include <alice/matrix/library/logging/log_context.h>
#include <alice/matrix/library/metrics/metrics.h>

#include <alice/uniproxy/library/protos/uniproxy.pb.h>

#include <apphost/lib/transport/transport_neh_backend.h>

#include <infra/libs/outcome/result.h>

#include <library/cpp/neh/asio/asio.h>
#include <library/cpp/neh/asio/executor.h>

namespace NMatrix {

class TSubwayClient : public TNonCopyable {
public:
    explicit TSubwayClient(
        const TSubwayClientSettings& config
    );

    NThreading::TFuture<TExpected<NUniproxy::TSubwayResponse, TString>> SendSubwayMessage(
        const NUniproxy::TSubwayMessage& subwayMessage,
        const TString& puid,
        TString hostOrIp,
        ui32 port,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

private:
    NAppHost::NTransport::TNehCommunicationSystem Ncs_;
    NAsio::TIOServiceExecutor DeadlineTimersExecutor_;

    const TDuration Timeout_;

    // For tests.
    const TMaybe<TString> HostOrIpOverride_;
    const TMaybe<ui32> PortOverride_;
};

} // namespace NMatrix
