#pragma once

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/asr/base/protobuf.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/protos/wsevent.pb.h>
#include <alice/cuttlefish/library/protos/events.pb.h>
#include <alice/cuttlefish/library/tts/backend/base/protobuf.h>

#include <apphost/api/client/client.h>
#include <apphost/api/client/fixed_grpc_backend.h>
#include <apphost/api/client/stream_timeout.h>
#include <apphost/lib/grpc/client/grpc_client.h>

#include <library/cpp/neh/asio/executor.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>
#include <util/system/hp_timer.h>

namespace NAlice {
    class TCuttlefishClient {
    public:
        struct TConfig {
            TConfig()
                : Host("localhost")
                , Port(80)
                , Path("")
                , Timeout(TDuration::Seconds(1))
                , ChunkRWTimeout(TDuration::Seconds(100))
            {}

            TString Host;
            ui16 Port;  // grpc port
            TString Path;
            TDuration Timeout;
            TDuration ChunkRWTimeout;
        };

        TCuttlefishClient(const TConfig& config);
        TCuttlefishClient(const TConfig& config, NAppHost::NClient::TClient&);

        NAppHost::NClient::TStream CreateStream();

        class TRequest: public TThrRefBase {
        public:
            typedef TIntrusivePtr<TRequest> TPtr;
            TRequest(TCuttlefishClient& asrClient, NAsio::TIOService& ioService)
                : CuttlefishClient_(asrClient)
                , IOService_(ioService)
                , Stream_(asrClient.CreateStream())
                , ItemType_(TString(NCuttlefish::ITEM_TYPE_WS_MESSAGE))
            {
                (void)CuttlefishClient_;
            }

            void SendWsEvent(const NAliceProtocol::TWsEvent&, bool final=false);
            void SendAsrResponse(const NAlice::NAsr::NProtobuf::TResponse&, bool final=false);
            virtual bool NextAsyncRead();
            void OnSubscibeCallback(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future);
            void OnResponse(const NAppHost::NClient::TOutputDataChunk& response);

            virtual void OnCuttlefishResponse(const NAliceProtocol::TAudio&) {
                DLOG("OnCuttlefishResponse: audio");
            }
            virtual void OnCuttlefishResponse(const NAliceProtocol::TDirective&) {
                DLOG("OnCuttlefishResponse: directive");
            }
            virtual void OnCuttlefishResponse(const NAliceProtocol::TWsEvent&) {
                DLOG("OnCuttlefishResponse: wsEvent");
            }
            virtual void OnCuttlefishResponse(const NAlice::NTts::NProtobuf::TBackendRequest&) {
                DLOG("OnCuttlefishResponse: wsEvent");
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
            TCuttlefishClient& CuttlefishClient_;
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
