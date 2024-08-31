#pragma once

#include <alice/cuttlefish/library/asr/base/callbacks_handler.h>

#include <alice/cuttlefish/library/logging/log_context.h>

namespace NAlice::NAsrAdapter {
    // add logging to callbacks usage
    class TAsrCallbacksWithEventlog: public NAsr::TCallbacksHandler {
    public:
        TAsrCallbacksWithEventlog(
            NCuttlefish::TInputAppHostAsyncRequestHandlerPtr requestHandler,
            const TString& requestId,
            TAtomicBase requestNumber,
            NRTLog::TRequestLoggerPtr,
            const NCuttlefish::TLogContext::TOptions&
        );

        void OnSpotterValidation(bool valid) override;
        void OnClosed() override;
        void OnAnyError(const TString& error, bool fastError) override;
        void AddAsrFinished(const NAliceProtocol::TAsrFinished&) override;
        void AddAndFlush(const NAsr::NProtobuf::TResponse&, bool isFinalResponse) override;

        void OnSessionLog(const NAliceProtocol::TSessionLogRecord&) override;

        const NCuttlefish::TLogContext& LogContext() noexcept {
            return Log_;
        }

    private:
        NAlice::NCuttlefish::TLogContext Log_;
        const TString RequestId_;
        const TAtomicBase RequestNumber_;
    };

}
