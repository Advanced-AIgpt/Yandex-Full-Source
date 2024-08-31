#pragma once

#include <alice/cuttlefish/library/api/handlers.h>
#include <alice/cuttlefish/library/asr/base/protobuf.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <apphost/api/client/client.h>
#include <apphost/api/client/fixed_grpc_backend.h>
#include <apphost/api/client/stream_timeout.h>
#include <apphost/lib/grpc/client/grpc_client.h>

#include <library/cpp/neh/asio/executor.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>
#include <util/system/hp_timer.h>

namespace NAlice {
    class TAsrClient {
    public:
        struct TConfig {
            TConfig()
                : Host("localhost")
                , Port(10501)
                , Path(NCuttlefish::SERVICE_HANDLE_ASR)
                , Timeout(TDuration::Seconds(1))
                , ChunkRWTimeout(TDuration::Seconds(100))
            {}

            TString Host;
            ui16 Port;  // grpc port
            TString Path;
            TDuration Timeout;
            TDuration ChunkRWTimeout;
        };

        TAsrClient(const TConfig& config);
        TAsrClient(const TConfig& config, NAppHost::NClient::TClient&);

        NAppHost::NClient::TStream CreateStream();

        class TRequest: public TThrRefBase {
        public:
            typedef TIntrusivePtr<TRequest> TPtr;
            TRequest(TAsrClient& asrClient, NAsio::TIOService& ioService)
                : AsrClient_(asrClient)
                , IOService_(ioService)
                , Stream_(asrClient.CreateStream())
            {
                (void)AsrClient_;
            }

            void SendInit(NAsr::NProtobuf::TInitRequest&& initRequest, bool appHostGraph);
            void SendAudioChunk(const void* data, size_t dataSize);
            void SendEndSpotter();
            void SendEndStream();

            virtual bool NextAsyncRead();
            void OnSubscibeCallback(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future);
            void OnResponse(const NAppHost::NClient::TOutputDataChunk& response);

            virtual void OnAsrResponse(NAsr::NProtobuf::TResponse&) {
                DLOG("OnAsrResponse");
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
            virtual void OnError(const TString& error) {
                Y_UNUSED(error);
                DLOG("ERROR: " << error); // TODO
            }

        protected:
            TAsrClient& AsrClient_;
            NAsio::TIOService& IOService_;
            NAppHost::NClient::TStream Stream_;
            bool NeedNextRead_ = true;
            TString ItemType_;
        };

    private:
        const TConfig& Config_;
        THolder<NAppHost::NClient::TClient> ApphostClientLocal_;
        NAppHost::NClient::TClient& ApphostClient_;
    };
}
