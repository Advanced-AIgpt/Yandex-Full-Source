#pragma once

#include <alice/cachalot/api/protos/cachalot.pb.h>
#include <alice/cuttlefish/library/tts/cache/base/interface.h>

#include <alice/cuttlefish/library/rtlog/rtlog.h>

#include <voicetech/library/ws_server/http_client.h>


namespace NAlice::NTtsCacheProxy {
    class TCachalotClient {
    public:
        struct TConfig {
            TString SetUrl_;
            TString GetUrl_;
            TString WarmUpUrl_;
        };

        enum ERequestType {
            SET = 1      /* "set" */,
            GET = 2      /* "get" */,
            WARM_UP = 3  /* "warm_up" */,
        };

    public:
        TCachalotClient(
            const TConfig& config,
            TIntrusivePtr<NTtsCache::TInterface::TCallbacks> callbacks,
            NVoicetech::THttpClient& httpClient,
            NRTLog::TRequestLoggerPtr rtLogger
        );
        ~TCachalotClient();

        // All methods are not thread safe
        // Be careful
        void AddCacheSetRequest(const NTtsCache::NProtobuf::TCacheSetRequest& cacheSetRequest);
        void AddCacheGetRequest(const NTtsCache::NProtobuf::TCacheGetRequest& cacheGetRequest);
        void AddCacheWarmUpRequest(const NTtsCache::NProtobuf::TCacheWarmUpRequest& cacheWarmUpRequest);
        void CancelAll();

    private:
        TString GetUrl(ERequestType requestType) const;
        static TStringBuf RequestTypeToString(ERequestType requestType);

        void AddCachalotRequest(
            const TString& key,
            ERequestType requestType,
            const NCachalotProtocol::TRequest& cachalotRequest
        );

    private:
        const TConfig& Config_;

        TIntrusivePtr<NTtsCache::TInterface::TCallbacks> Callbacks_;
        NVoicetech::THttpClient& HttpClient_;
        NRTLog::TRequestLoggerPtr RtLogger_;

        // Refs to all http handler Which have ever been created by this client
        // Used for canceling requests
        // Request is not removed from this vector even if it is completed
        // because after completion cancel does nothing
        // and we don't want to access this client from more than one thread
        // Memory leak or OOM will not happen because for each new apphost request we create a new client
        // and remove this client after request end
        TVector<NVoicetech::THttpHandlerRef> HttpHandlers_;
    };
}
