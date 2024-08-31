#pragma once

#include <alice/cuttlefish/library/api/handlers.h>
#include <alice/cuttlefish/library/tts/cache/base/protobuf.h>
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
    class TTtsCacheClient {
    public:
        struct TConfig {
            TConfig()
                : Host("localhost")
                , Port(80)
                , Path(NCuttlefish::SERVICE_HANDLE_TTS_CACHE)
                , Timeout(TDuration::Seconds(1))
            {}

            TString Host;
            ui16 Port; // Grpc port
            TString Path;
            TDuration Timeout;
        };

        explicit TTtsCacheClient(const TConfig& config);

        NAppHost::NClient::TStream CreateStream();

        class TRequest: public TThrRefBase {
        public:
            typedef TIntrusivePtr<TRequest> TPtr;

            TRequest(TTtsCacheClient& ttsCacheClient, NAsio::TIOService& ioService)
                : TtsCacheClient_(ttsCacheClient)
                , IOService_(ioService)
                , Stream_(ttsCacheClient.CreateStream())
            {}

            void SendCacheSetRequest(NTtsCache::NProtobuf::TCacheSetRequest&& cacheSetRequest);
            void SendCacheGetRequest(NTtsCache::NProtobuf::TCacheGetRequest&& cacheGetRequest);
            void SendCacheWarmUpRequest(NTtsCache::NProtobuf::TCacheWarmUpRequest&& cacheWarmUpRequest);
            void SendEndOfStream();
            void Cancel();

            virtual bool NextAsyncRead();
            void OnSubscibeCallback(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future);
            void OnResponse(const NAppHost::NClient::TOutputDataChunk& response);

            virtual void OnCacheGetResponse(const NTtsCache::NProtobuf::TCacheGetResponse& cacheGetResponse) {
                Y_UNUSED(cacheGetResponse);
                DLOG("OnCacheGetResponse" << cacheGetResponse.GetKey());
            }
            virtual void OnEndNextResponseProcessing() {
                NextAsyncRead();
            }
            virtual void OnEndOfResponsesStream() {
                DLOG("OnEndOfResponsesStream");
            }
            virtual void OnTimeout() {
                DLOG("OnTimeout");
            }
            virtual void OnError(const TString& error) {
                Y_UNUSED(error);
                DLOG("OnError: " << error);
            }

        protected:
            TTtsCacheClient& TtsCacheClient_;
            NAsio::TIOService& IOService_;
            NAppHost::NClient::TStream Stream_;

            bool NeedNextRead_ = true;
        };

    private:
        const TConfig& Config_;
        NAppHost::NClient::TClient ApphostClient_;
    };
}
