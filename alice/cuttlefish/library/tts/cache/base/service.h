#pragma once

#include "callbacks_handler.h"
#include "fake.h"

#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/rtlog/rtlog.h>

#include <apphost/api/service/cpp/service.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>

namespace NAlice::NTtsCache {
    class TService {
    public:
        using TRequestHandler = NCuttlefish::TInputAppHostAsyncRequestHandler;
        using TRequestHandlerPtr = NCuttlefish::TInputAppHostAsyncRequestHandlerPtr;

    public:
        virtual ~TService() = default;

    public:
        class TRequestProcessor: public TThrRefBase {
        public:
            explicit TRequestProcessor(TService& service)
                : Service_(service)
            {
            }

            // Main entry-point for processing next message from apphost graph
            void ProcessInput(NAppHost::TServiceContextPtr ctx, NThreading::TPromise<void> promise) {
                ProcessInput(MakeIntrusive<TRequestHandler>(ctx, promise));
            }
            virtual void ProcessInput(TRequestHandlerPtr);

        protected:
            virtual void ProcessInputImpl();

            virtual void ProcessCacheSetRequests();
            virtual void ProcessCacheGetRequests();
            virtual void ProcessCacheWarmUpRequests();
            virtual void ProcessUnknownRequestItems();

            virtual void OnCacheSetRequest(const NProtobuf::TCacheSetRequest& cacheSetRequest);
            virtual void OnCacheGetRequest(const NProtobuf::TCacheGetRequest& cacheGetRequest);
            virtual void OnCacheWarmUpRequest(const NProtobuf::TCacheWarmUpRequest& cacheWarmUpRequest);

            virtual void InitializeTtsCache(TIntrusivePtr<NTtsCache::TInterface::TCallbacks> callbacks) {
                // By default create fake tts cache (emulator for testing/debug)
                TtsCache_.Reset(new NTtsCache::TFake(callbacks));
            }
            virtual TIntrusivePtr<NTtsCache::TInterface::TCallbacks> CreateTtsCacheCallbacks() {
                return new TCallbacksHandler(RequestHandler_);
            }
            virtual void OnAppHostEmptyInput() {
                DLOG("RequestProcessor.OnAppHostEmptyInput");
            }
            virtual void OnAppHostClose();
            virtual void OnUnknownItemType(const TString& tag, const TString& type) {
                DLOG("RequestProcessor.OnUnknownItemType tag=" << tag << " type=" << type);

                Y_UNUSED(tag);
                Y_UNUSED(type);
            }
            virtual void OnWarning(const TString& warning) {
                DLOG("RequestProcessor.OnWarning " << warning);
                Y_UNUSED(warning);
            }
            virtual void OnError(const TString& error) {
                DLOG("RequestProcessor.OnError: " << error);
                Y_UNUSED(error);
            }

        protected:
            TService& Service_;
            TRequestHandlerPtr RequestHandler_;
            TIntrusivePtr<NTtsCache::TInterface> TtsCache_;
        };

        static TString Path() {
            return "/tts_cache";
        }

        virtual TIntrusivePtr<TRequestProcessor> CreateProcessor(NAppHost::IServiceContext&, NAlice::NCuttlefish::TRtLogClient*) {
            return MakeIntrusive<TRequestProcessor>(*this);
        }
    };
}
