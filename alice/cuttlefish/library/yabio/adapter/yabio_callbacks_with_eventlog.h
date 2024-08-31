#pragma once

#include <alice/cuttlefish/library/yabio/base/callbacks_handler.h>

#include <alice/cuttlefish/library/logging/log_context.h>

namespace NAlice::NYabioAdapter {
    // add logging to callbacks usage
    class TYabioCallbacksWithEventlog: public NYabio::TCallbacksHandler {
    public:
        TYabioCallbacksWithEventlog(
            NCuttlefish::TInputAppHostAsyncRequestHandlerPtr,
            const TString& requestId,
            TAtomicBase requestNumber,
            NAlice::NCuttlefish::TLogContext&&);

        void OnAnyError(NYabio::NProtobuf::EResponseCode, const TString& error, bool fastError) override;
        void AddNewEnrolling(const NAliceProtocol::TBioContextSaveNewEnrolling&) override;
        void AddAndFlush(const NYabio::NProtobuf::TResponse&, bool isFinalResponse) override;
        void OnClosed() override;

        const NCuttlefish::TLogFramePtr& LogFrame() noexcept {
            return Log_.FramePtr();
        }

    private:
        NAlice::NCuttlefish::TLogContext Log_;
        const TString RequestId_;
        const TAtomicBase RequestNumber_;
    };

}
