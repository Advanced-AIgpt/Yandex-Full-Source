#pragma once

#include "interface.h"

#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/apphost/async_input_request_handler.h>

namespace NAlice::NTts {
    class TCallbacksHandler: public TInterface::TCallbacks {
    public:
        TCallbacksHandler(NCuttlefish::TInputAppHostAsyncRequestHandlerPtr requestHandler)
            : RequestHandler_(requestHandler)
        {}
        ~TCallbacksHandler() {
            if (!Closed_) {
                OnClosed();
            }
            if (!Finished_) {
                Finish();
            }
        }

        void OnStartRequestProcessing(ui32 reqSeqNo) override;
        void OnRequestProcessingStarted(ui32 reqSeqNo) override;
        void OnBackendResponse(NProtobuf::TBackendResponse& backendResponse, const TString& mime) override;

        void OnDublicateRequest(ui32 reqSeqNo) override;
        void OnInvalidRequest(ui32 reqSeqNo, const TString& errorMessage) override;

        void OnClosed() override;
        void OnAnyError(const TString& error, bool fastError, ui32 reqSeqNo) override;
        void Finish() override;

    protected:
        virtual void AddAudioAndFlush(const NAliceProtocol::TAudio& audio);
        virtual void FlushAppHostContext(bool isFinalFlush);

    protected:
        NCuttlefish::TInputAppHostAsyncRequestHandlerPtr RequestHandler_;

        THashSet<ui32> StartedReqSeqNo_;
        THashSet<ui32> FinishedReqSeqNo_;

        bool Finished_ = false;
        bool Closed_ = false;
        bool AnyRequestProcessingStarted_ = false;
    };
}
