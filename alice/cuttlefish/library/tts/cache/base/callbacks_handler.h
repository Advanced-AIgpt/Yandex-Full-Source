#pragma once

#include "interface.h"

#include <alice/cuttlefish/library/apphost/async_input_request_handler.h>

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NTtsCache {
    class TCallbacksHandler: public TInterface::TCallbacks {
    public:
        explicit TCallbacksHandler(NCuttlefish::TInputAppHostAsyncRequestHandlerPtr requestHandler)
            : RequestHandler_(requestHandler)
        {}

        ~TCallbacksHandler() {
            if (!Closed_) {
                OnClosed();
            }
            Finish();
        }

        void OnCacheSetRequestCompleted(const TString& key, const TMaybe<TString>& error) override;
        void OnCacheWarmUpRequestCompleted(const TString& key, const TMaybe<TString>& error) override;

        void OnCacheGetResponse(const NProtobuf::TCacheGetResponse& cacheGetResponse) override;

        void OnClosed() override;
        void OnAnyError(const TString& error) override;
        void Finish() override;

    protected:
        virtual void AddCacheGetResponseAndFlush(const NProtobuf::TCacheGetResponse& cacheGetResponse);
        virtual void FlushAppHostContext(bool isFinalFlush);

    protected:
        NCuttlefish::TInputAppHostAsyncRequestHandlerPtr RequestHandler_;
        bool Closed_ = false;
    };
}
