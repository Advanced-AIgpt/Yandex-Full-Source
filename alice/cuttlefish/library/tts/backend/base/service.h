#pragma once

#include "callbacks_handler.h"
#include "fake.h"

#include <alice/cuttlefish/library/protos/tts.pb.h>
#include <alice/cuttlefish/library/rtlog/rtlog.h>

#include <apphost/api/service/cpp/service.h>

#include <util/stream/str.h>

namespace NAlice::NTts {

    class TService {
    public:
        using TRequestHandler = NCuttlefish::TInputAppHostAsyncRequestHandler;
        using TRequestHandlerPtr = NCuttlefish::TInputAppHostAsyncRequestHandlerPtr;

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

            virtual void ProcessBackendRequests();
            virtual void ProcessUnknownRequestItems();

            virtual void OnBackendRequest(const NTts::NProtobuf::TBackendRequest& backendRequest, const TStringBuf& itemType);

            virtual void InitializeTts(TIntrusivePtr<NTts::TInterface::TCallbacks> callbacks) {
                // By default create fake tts (emulator for testing/debug)
                Tts_.Reset(new NTts::TFake(callbacks));
            }
            virtual TIntrusivePtr<NTts::TInterface::TCallbacks> CreateTtsCallbacks() {
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
            TIntrusivePtr<NTts::TInterface> Tts_;
        };
        virtual ~TService() = default;

        static TString Path() {
            return "/tts";
        }

        virtual TIntrusivePtr<TRequestProcessor> CreateProcessor(NAppHost::IServiceContext&, NAlice::NCuttlefish::TRtLogClient*) {
            return MakeIntrusive<TRequestProcessor>(*this);
        }
    };
}
