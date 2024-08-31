#pragma once

#include "callbacks_handler.h"
#include "fake.h"

#include <alice/cuttlefish/library/protos/asr.pb.h>
#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/rtlog/rtlog.h>
#include <util/generic/ptr.h>
#include <util/stream/str.h>
#include <apphost/api/service/cpp/service.h>

#include <queue>

namespace NAlice::NYabio {

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
            virtual void ProcessInputImpl();

        protected:
            // return false if processing request finished
            virtual bool OnAppHostProtoItem(const TString& type, const NAppHost::NService::TProtobufItem&);
            virtual bool OnCuttlefishAudio(NAliceProtocol::TAudio&, bool postponed=false);
            virtual bool OnYabioContext(bool hasData, const TString& data);
            virtual void OnAsrFinished(const NAliceProtocol::TAsrFinished&);
            virtual bool TryBeginScore();
            virtual TIntrusivePtr<NYabio::TInterface::TCallbacks> CreateYabioCallbacks(const TString& requestId) {
                (void)requestId; // can be used for mark log records
                return new TCallbacksHandler(RequestHandler_);
            }
            virtual void OnIgnoreInitRequest(NProtobuf::TInitRequest&, const TString& /*reason*/) {}
            virtual bool OnRecvInitRequest(NProtobuf::TInitRequest&);
            virtual bool OnInitRequest();
            virtual bool OnInitRequest(TIntrusivePtr<NYabio::TInterface::TCallbacks>, const TString& requestId);
            virtual void OnInvalidProtobuf(const TString& error) {
                OnError(NProtobuf::RESPONSE_CODE_BAD_REQUEST, error);
            }
            virtual void OnAppHostEmptyInput();
            virtual void OnAppHostClose();
            virtual void OnWarning(const TString&) {}
            virtual void OnError(NProtobuf::EResponseCode, const TString& error);
            virtual void SendFastInitResponseError(NProtobuf::EResponseCode);

        protected:
            TService& Service_;
            TRequestHandlerPtr RequestHandler_;
            TString Mime_;
            NProtobuf::EResponseCode PosponedInitRequestError_ = NProtobuf::RESPONSE_CODE_OK;
            NProtobuf::TInitRequest InitRequest_;
            NProtobuf::TContext Context_;
            // we MUST has both Context & InitRequest for begin scoring, so collect audio messages here
            std::queue<std::unique_ptr<google::protobuf::Message>> InputQueue_;
            bool HasContextResponse_ = false;
            bool HasContextData_ = false;
            bool HasInitRequest_ = false;
            enum {
                MethodAny,
                MethodClassify,
                MethodScore,
            } EnabledBiometryMethod_ = MethodAny;
            bool PostponedMode_ = false;
            bool SpotterStream_ = false;
            bool HasEndOfStream_ = false;
            TIntrusivePtr<NYabio::TInterface> Yabio_;
            bool CloseConnectionReceived_ = false;
        };
        virtual ~TService() = default;

        static TString Path() {
            return "/bio";
        }

        virtual TIntrusivePtr<TRequestProcessor> CreateProcessor(NAppHost::IServiceContext&, NAlice::NCuttlefish::TRtLogClient*) {
            return MakeIntrusive<TRequestProcessor>(*this);
        }
    };
}
