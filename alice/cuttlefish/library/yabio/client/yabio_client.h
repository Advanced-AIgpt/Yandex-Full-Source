#pragma once

#include <alice/cuttlefish/library/api/handlers.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/yabio/base/protobuf.h>

#include <apphost/api/client/client.h>
#include <apphost/api/client/fixed_grpc_backend.h>
#include <apphost/api/client/stream_timeout.h>
#include <apphost/lib/grpc/client/grpc_client.h>

#include <library/cpp/neh/asio/executor.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>
#include <util/system/hp_timer.h>

namespace NAlice {
    class TYabioClient {
    public:
        struct TConfig {
            TConfig()
                : Host("localhost")
                , Port(80)
                , Path(NCuttlefish::SERVICE_HANDLE_BIO)
                , Timeout(TDuration::Seconds(1))
                , ChunkRWTimeout(TDuration::Seconds(100))
            {}

            TString Host;
            ui16 Port;  // grpc port
            TString Path;
            TDuration Timeout;
            TDuration ChunkRWTimeout;
        };

        TYabioClient(const TConfig& config);
        TYabioClient(const TConfig& config, NAppHost::NClient::TClient&);

        NAppHost::NClient::TStream CreateStream();

        class TRequest: public TThrRefBase {
        public:
            typedef TIntrusivePtr<TRequest> TPtr;
            TRequest(TYabioClient& yabioClient, NAsio::TIOService& ioService)
                : YabioClient_(yabioClient)
                , IOService_(ioService)
                , Stream_(yabioClient.CreateStream())
            {
                (void)YabioClient_;
            }

            void SendInit(NYabio::NProtobuf::TInitRequest&& initRequest, bool appHostGraph);
            void SendAudioChunk(const char* data, size_t size);
            void SendEndSpotter();
            void SendEndStream();
            virtual bool NextAsyncRead();
            void OnSubscibeCallback(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future);
            void OnResponse(const NAppHost::NClient::TOutputDataChunk& response);

            virtual void OnYabioResponse(NYabio::NProtobuf::TResponse&) {
                DLOG("OnYabioResponse");
            }
            virtual void OnEndNextResponseProcessing() {
                NextAsyncRead();
            }
            virtual void OnEndOfResponsesStream() {
                DLOG("GOT EOS");
            }
            virtual void OnTimeout() {
                DLOG("Timeout"); // TODO
            }
            virtual void OnError(const TString& s) {
                (void)s;
                DLOG("ERROR: " << s); // TODO
            }

        protected:
            TYabioClient& YabioClient_;
            NAsio::TIOService& IOService_;
            NAppHost::NClient::TStream Stream_;
            bool NeedNextRead_ = true;
            TString ItemType_;
        };

    private:
        const TConfig& Config_;
        //NAppHost::NGrpc::NClient::TGrpcCommunicationSystem System_;
        THolder<NAppHost::NClient::TClient> ApphostClientLocal_;
        NAppHost::NClient::TClient& ApphostClient_;
    };
}
