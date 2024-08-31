#pragma once

#include <alice/cuttlefish/library/api/handlers.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/protos/tts.pb.h>

#include <apphost/api/client/client.h>
#include <apphost/api/client/fixed_grpc_backend.h>
#include <apphost/api/client/stream_timeout.h>
#include <apphost/lib/grpc/client/grpc_client.h>

#include <library/cpp/neh/asio/executor.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>
#include <util/system/hp_timer.h>

namespace NAlice {
    class TTtsClient {
    public:
        struct TConfig {
            TConfig()
                : Host("localhost")
                , Port(80)
                , Path(NCuttlefish::SERVICE_HANDLE_TTS)
                , Timeout(TDuration::Seconds(60))
            {}

            TString Host;
            ui16 Port;  // grpc port
            TString Path;
            TDuration Timeout;
        };

        TTtsClient(const TConfig& config);

        NAppHost::NClient::TStream CreateStream();

        class TRequest: public TThrRefBase {
        public:
            typedef TIntrusivePtr<TRequest> TPtr;
            TRequest(TTtsClient& ttsClient, NAsio::TIOService& ioService)
                : TtsClient_(ttsClient)
                , IOService_(ioService)
                , Stream_(ttsClient.CreateStream())
            {}

            void SendBackendRequest(::NTts::TBackendRequest&& backendRequest);
            void SendEndOfStream();
            void Cancel();

            virtual bool NextAsyncRead();
            void OnSubscibeCallback(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future);
            void OnResponse(const NAppHost::NClient::TOutputDataChunk& response);

            virtual void OnProtocolAudio(NAliceProtocol::TAudio& audio) {
                DLOG("OnProtocolAudio");
                Y_UNUSED(audio);
            }
            virtual void OnEndNextResponseProcessing() {
                NextAsyncRead();
            }
            virtual void OnEndOfResponsesStream() {
                DLOG("GOT EOS");
            }
            virtual void OnTimeout() {
                DLOG("Timeout");
            }
            virtual void OnError(const TString& error) {
                Y_UNUSED(error);
                DLOG("ERROR: " << error);
            }

        protected:
            TTtsClient& TtsClient_;
            NAsio::TIOService& IOService_;
            NAppHost::NClient::TStream Stream_;
            bool NeedNextRead_ = true;
        };

    private:
        const TConfig& Config_;
        NAppHost::NClient::TClient ApphostClient_;
    };
}
