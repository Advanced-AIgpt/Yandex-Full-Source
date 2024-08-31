#pragma once

#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <apphost/api/client/client.h>
#include <apphost/api/client/fixed_grpc_backend.h>
#include <apphost/lib/grpc/client/grpc_client.h>

#include <library/cpp/neh/asio/executor.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>
#include <util/system/hp_timer.h>

namespace NAlice {
    class TCachalotClient {
    public:
        struct TConfig {
            TConfig()
                : Host("localhost")
                , Port(8081)
                , Path("/")
                , Timeout(TDuration::Seconds(10))
                , ChunkRWTimeout(TDuration::Seconds(100))
            {}

            TString Host;
            ui16 Port;  // grpc port
            TString Path;
            TDuration Timeout;
            TDuration ChunkRWTimeout;
        };

        TCachalotClient(const TConfig& config);
        TCachalotClient(const TConfig& config, NAppHost::NClient::TClient&);

        NAppHost::NClient::TStream CreateStream();

        class TRequest: public TThrRefBase {
        public:
            typedef TIntrusivePtr<TRequest> TPtr;
            TRequest(TCachalotClient& cachalotClient, NAsio::TIOService& ioService)
                : CachalotClient_(cachalotClient)
                , IOService_(ioService)
                , Stream_(cachalotClient.CreateStream())
            {
                (void)CachalotClient_;
            }

            //TODO:more request types
            void SendYabioContextRequest(NCachalotProtocol::TYabioContextRequest&&, bool appHostGraph);
            virtual bool NextAsyncRead();
            void OnSubscibeCallback(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future);
            void OnResponse(const NAppHost::NClient::TOutputDataChunk& response);

            virtual void OnYabioContextResponse(NCachalotProtocol::TYabioContextResponse&) {
                DLOG("OnYabioContextResponse");
            }
            virtual void OnEndNextResponseProcessing() {
                DLOG("OnEndNextResponseProcessing");
            }
            virtual void OnEndOfResponsesStream() {
                DLOG("got EOS");
            }
            virtual void OnTimeout() {
                DLOG("Timeout"); // TODO
            }
            virtual void OnError(const TString& s) {
                (void)s;
                DLOG("ERROR: " << s); // TODO
            }

        protected:
            TCachalotClient& CachalotClient_;
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
