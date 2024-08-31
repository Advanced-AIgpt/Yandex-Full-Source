#pragma once

#include "cachalot_client.h"
#include "metrics.h"

#include <alice/cuttlefish/library/tts/cache/base/interface.h>

namespace NAlice::NTtsCacheProxy {

    class TTtsCache : public NTtsCache::TInterface {
    public:
        TTtsCache(
            TIntrusivePtr<TCallbacks> callbacks,
            const TCachalotClient::TConfig& cachalotClientConfig,
            NVoicetech::THttpClient& httpClient,
            NRTLog::TRequestLoggerPtr rtLogger
        );

        void ProcessCacheSetRequest(const NTtsCache::NProtobuf::TCacheSetRequest& cacheSetRequest) override;
        void ProcessCacheGetRequest(const NTtsCache::NProtobuf::TCacheGetRequest& cacheGetRequest) override;
        void ProcessCacheWarmUpRequest(const NTtsCache::NProtobuf::TCacheWarmUpRequest& cacheWarmUpRequest) override;

        void Cancel() override;

    private:
        static constexpr TStringBuf SOURCE_NAME = "tts_cache";
        TSourceMetrics Metrics_;

        TCachalotClient CachalotClient_;
    };
}
