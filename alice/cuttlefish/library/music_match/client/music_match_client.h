#pragma once

#include <alice/cuttlefish/library/api/handlers.h>
#include <alice/cuttlefish/library/music_match/base/protobuf.h>
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
    class TMusicMatchClient {
    public:
        struct TConfig {
            TConfig()
                : Host("localhost")
                , Port(80)
                , Path(NCuttlefish::SERVICE_HANDLE_MUSIC_MATCH)
                , Timeout(TDuration::Seconds(1))
                , ChunkRWTimeout(TDuration::Seconds(100))
                , TvmTicket("")
            {}

            TString Host;
            ui16 Port;  // grpc port
            TString Path;
            TDuration Timeout;
            TDuration ChunkRWTimeout;
            TString TvmTicket;
        };

        TMusicMatchClient(const TConfig& config);
        TMusicMatchClient(const TConfig& config, NAppHost::NClient::TClient&);

        NAppHost::NClient::TStream CreateStream();

        class TRequest: public TThrRefBase {
        public:
            typedef TIntrusivePtr<TRequest> TPtr;
            TRequest(TMusicMatchClient& musicMatchClient, NAsio::TIOService& ioService)
                : MusicMatchClient_(musicMatchClient)
                , IOService_(ioService)
                , Stream_(musicMatchClient.CreateStream())
            {
                (void)MusicMatchClient_;
            }

            void SendMusicMatchFlag();
            // Must be sent in a same chunk
            void SendContextLoadResponseAndSessionContext(
                NMusicMatch::NProtobuf::TContextLoadResponse&& contextLoadResponse,
                NMusicMatch::NProtobuf::TSessionContext&& sessionContext
            );
            void SendInitRequest(NMusicMatch::NProtobuf::TInitRequest&& initRequest);
            void SendBeginStream(const TString& mime);
            void SendEndStream();
            void SendBeginSpotter();
            void SendEndSpotter();
            void SendAudioChunk(const void* data, size_t dataSize);

            virtual bool NextAsyncRead();
            void OnSubscibeCallback(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future);
            void OnResponse(const NAppHost::NClient::TOutputDataChunk& response);

            virtual void OnInitResponse(NMusicMatch::NProtobuf::TInitResponse&) {
                DLOG("OnInitResponse");
            }
            virtual void OnStreamResponse(NMusicMatch::NProtobuf::TStreamResponse&) {
                DLOG("OnStreamResponse");
            }
            virtual void OnEndNextResponseProcessing() {
                NextAsyncRead();
            }
            virtual void OnTimeout() {
                DLOG("Timeout"); // TODO
            }
            virtual void OnError(const TString& error) {
                Y_UNUSED(error);
                DLOG("ERROR: " << error); // TODO
            }

        protected:
            TMusicMatchClient& MusicMatchClient_;
            NAsio::TIOService& IOService_;
            NAppHost::NClient::TStream Stream_;
            bool NeedNextRead_ = true;
        };

    private:
        const TConfig& Config_;
        THolder<NAppHost::NClient::TClient> AppHostClientLocal_;
        NAppHost::NClient::TClient& AppHostClient_;
    };
}
