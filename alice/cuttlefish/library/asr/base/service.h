#pragma once

#include "callbacks_handler.h"
#include "fake.h"

#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/rtlog/rtlog.h>
#include <apphost/api/service/cpp/service.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>

namespace NAlice::NAsr {

    class TService {
    public:
        using TRequestHandler = NCuttlefish::TInputAppHostAsyncRequestHandler;
        using TRequestHandlerPtr = NCuttlefish::TInputAppHostAsyncRequestHandlerPtr;

        class TRequestProcessor: public TThrRefBase {
        public:
            TRequestProcessor(TService& service)
                : Service_(service)
            {
            }

            // main entry-point for processing next message from apphost graph
            void ProcessInput(NAppHost::TServiceContextPtr ctx, NThreading::TPromise<void> promise) {
                ProcessInput(MakeIntrusive<TRequestHandler>(ctx, promise));
            }
            virtual void ProcessInput(TRequestHandlerPtr);

        protected:
            virtual void ProcessInputImpl();
            virtual void OnBeginProcessInput() {}
            // return false if processing request finished
            virtual bool OnAppHostProtoItem(const TString& type, const NAppHost::NService::TProtobufItem& item);
            //virtual bool OnAsrRequest(NAppHost::TServiceContextPtr&, const NProtobuf::TRequest&);
            virtual bool OnCuttlefishAudio(const NAliceProtocol::TAudio&);
            virtual TIntrusivePtr<NAsr::TInterface::TCallbacks> CreateAsrCallbacks(TRequestHandlerPtr rh, const TString& requestId) {
                (void)requestId; // can be used for mark log records
                return new TCallbacksHandler(rh);
            }
            virtual bool OnInitRequest(NProtobuf::TRequest& request, TIntrusivePtr<NAsr::TInterface::TCallbacks> callbacks, const TString& requestId) {
                (void)requestId; // can be used for mark log records
                // by default create fake asr (emulator for testing/debug)
                Asr_.Reset(new NAsr::TFake(callbacks));
                Asr_->ProcessAsrRequest(request);
                return true;
            }
            virtual void OnInvalidProtobuf(const TString& error) {
                OnError(error);
            }
            virtual void OnAppHostEmptyInput() {}
            virtual void OnAppHostClose();
            virtual void OnWarning(const TString&) {}
            virtual void OnError(const TString& error);

        protected:
            TService& Service_;
            TRequestHandlerPtr RequestHandler_;
            bool HasInitRequest_ = false;
            bool HasEndOfStream_ = false;
            TIntrusivePtr<NAsr::TInterface> Asr_;
            bool CloseConnectionReceived_ = false;
            bool SpotterStream_ = false;  // flag for keep spotter mode
        };
        virtual ~TService() = default;

        static TString Path() {
            return "/asr";
        }

        virtual TIntrusivePtr<TRequestProcessor> CreateProcessor(NAppHost::IServiceContext&, NAlice::NCuttlefish::TRtLogClient*) {
            return MakeIntrusive<TRequestProcessor>(*this);
        }
    };
}
