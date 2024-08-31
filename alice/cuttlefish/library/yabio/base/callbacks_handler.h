#pragma once

#include "interface.h"

#include <alice/cuttlefish/library/apphost/async_input_request_handler.h>
#include <alice/cuttlefish/library/protos/bio_context_save.pb.h>
#include <apphost/api/service/cpp/service.h>

namespace NAlice::NYabio {
    class TCallbacksHandler: public TInterface::TCallbacks {
    public:
        TCallbacksHandler(NCuttlefish::TInputAppHostAsyncRequestHandlerPtr requestHandler)
            : RequestHandler_(requestHandler)
        {}

        void OnInitResponse(const NProtobuf::TResponse&) override;
        void OnAddDataResponse(const NProtobuf::TResponse&) override;
        void OnClosed() override;
        void OnAnyError(NProtobuf::EResponseCode responseCode, const TString& error, bool fastError) override;
        void Finish() override {
            RequestHandler_->Finish();
        }

        virtual void AddNewEnrolling(const NAliceProtocol::TBioContextSaveNewEnrolling&);
        virtual void AddAndFlush(const NProtobuf::TResponse&, bool isFinalResponse);

    protected:
        NCuttlefish::TInputAppHostAsyncRequestHandlerPtr RequestHandler_;
        bool Closed_ = false;
        bool HasInitResponse_ = false;
    };
}
