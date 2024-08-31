#pragma once

#include "protobuf.h"

#include <util/generic/ptr.h>

namespace NAlice::NTts {
    class TInterface : public TThrRefBase {
    public:
        class TCallbacks : public TThrRefBase {
        public:
            // Request was taken out of the queue and started processing
            virtual void OnStartRequestProcessing(ui32 reqSeqNo) = 0;
            // We got first response data and really start processing request (fast error is impossible after this point)
            virtual void OnRequestProcessingStarted(ui32 reqSeqNo) = 0;
            virtual void OnBackendResponse(NProtobuf::TBackendResponse& backendResponse, const TString& mime) = 0;

            virtual void OnDublicateRequest(ui32 reqSeqNo) = 0;
            virtual void OnInvalidRequest(ui32 reqSeqNo, const TString& errorMessage) = 0;

            virtual void OnClosed() = 0;
            virtual void OnAnyError(const TString& error, bool fastError, ui32 reqSeqNo) = 0;
            virtual void Finish() = 0;
        };

        TInterface(TIntrusivePtr<TCallbacks>& callbacks)
            : Callbacks_(callbacks)
        {}

        virtual void ProcessBackendRequest(const NProtobuf::TBackendRequest& backendRequest) = 0;
        virtual void Cancel() = 0;

    protected:
        TIntrusivePtr<TCallbacks> Callbacks_;
    };
}
