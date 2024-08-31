#pragma once

#include <alice/matrix/library/clients/tvm_client/tvm_client.h>
#include <alice/matrix/library/config/config.pb.h>
#include <alice/matrix/library/logging/log_context.h>
#include <alice/matrix/library/metrics/metrics.h>

#include <alice/megamind/protos/common/iot.pb.h>

#include <apphost/lib/transport/transport_neh_backend.h>

#include <infra/libs/outcome/result.h>

#include <library/cpp/neh/asio/asio.h>
#include <library/cpp/neh/asio/executor.h>

namespace NMatrix {

class TIoTClient : public TNonCopyable {
public:
    explicit TIoTClient(
        const TIoTClientSettings& config,
        TTvmClient& tvmClient
    );

    NThreading::TFuture<TExpected<NAlice::TIoTUserInfo, TString>> GetUserInfo(
        const TString& userTicket,
        TLogContext logContext,
        TSourceMetrics& metrics
    );

private:
    NAppHost::NTransport::TNehCommunicationSystem Ncs_;
    NAsio::TIOServiceExecutor DeadlineTimersExecutor_;

    const TString Host_;
    const ui32 Port_;
    const TDuration Timeout_;

    TTvmClient& TvmClient_;
};

} // namespace NMatrix
