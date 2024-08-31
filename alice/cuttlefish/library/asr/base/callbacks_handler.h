#pragma once

#include "interface.h"

#include <alice/cuttlefish/library/apphost/async_input_request_handler.h>
#include <apphost/api/service/cpp/service.h>

namespace NAlice::NAsr {
    class TCallbacksHandler: public TInterface::TCallbacks {
    public:
        TCallbacksHandler(NCuttlefish::TInputAppHostAsyncRequestHandlerPtr requestHandler)
            : RequestHandler_(requestHandler)
        {}

        void OnInitResponse(const NProtobuf::TResponse&) override;
        void OnSpotterValidation(bool valid) override;
        void OnAddDataResponse(const NProtobuf::TResponse&) override;
        void OnSessionLog(const NAliceProtocol::TSessionLogRecord&) override;
        void OnClosed() override;
        void OnAnyError(const TString& error, bool fastError) override;

        virtual void AddAsrFinished(const NAliceProtocol::TAsrFinished&);
        virtual void AddAndFlush(const NProtobuf::TResponse&, bool isFinalResponse);

        void Finish() override {
            RequestHandler_->Finish();
        }

    protected:
        NCuttlefish::TInputAppHostAsyncRequestHandlerPtr RequestHandler_;
        bool HasInitResponse_ = false;
        bool Closed_ = false;

    private:
        unsigned AddDataResponseNumber_ = 0;
    };
}
