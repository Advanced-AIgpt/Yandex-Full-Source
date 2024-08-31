#pragma once

#include "metrics.h"
#include "callbacks_handler.h"

#include <alice/cuttlefish/library/logging/log_context.h>

namespace NAlice::NTts {
    // Add logging to callbacks usage
    class TCallbacksWithEventlog: public NTts::TCallbacksHandler {
    public:
        TCallbacksWithEventlog(
            NCuttlefish::TInputAppHostAsyncRequestHandlerPtr requestHandler,
            NAlice::NCuttlefish::TLogContext&& logContext,
            TAtomicBase requestNumber
        );

        void OnStartRequestProcessing(ui32 reqSeqNo) override;
        void OnRequestProcessingStarted(ui32 reqSeqNo) override;

        void OnDublicateRequest(ui32 reqSeqNo) override;
        void OnInvalidRequest(ui32 reqSeqNo, const TString& errorMessage) override;

    protected:
        void OnAnyError(const TString& error, bool fastError, ui32 reqSeqNo) override;
        void AddAudioAndFlush(const NAliceProtocol::TAudio& audio) override;
        void FlushAppHostContext(bool isFinalFlush) override;

    private:
        static constexpr TStringBuf SOURCE_NAME = "tts_callbacks";
        TSourceMetrics Metrics_;

        NCuttlefish::TLogContext LogContext_;
        const TAtomicBase RequestNumber_;
    };

}
