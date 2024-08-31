#include "tts_cache.h"

#include <util/string/builder.h>

using namespace NAlice::NTtsCache;
using namespace NAlice::NTtsCacheProxy;

TTtsCache::TTtsCache(
    TIntrusivePtr<TCallbacks> callbacks,
    const TCachalotClient::TConfig& cachalotClientConfig,
    NVoicetech::THttpClient& httpClient,
    NRTLog::TRequestLoggerPtr rtLogger
)
    : TInterface(callbacks)
    , Metrics_(TTtsCache::SOURCE_NAME)
    , CachalotClient_(
        cachalotClientConfig,
        callbacks,
        httpClient,
        rtLogger
    )
{
}

void TTtsCache::ProcessCacheSetRequest(const NProtobuf::TCacheSetRequest& cacheSetRequest) {
    Metrics_.PushRate("set", "added");
    CachalotClient_.AddCacheSetRequest(cacheSetRequest);
}
void TTtsCache::ProcessCacheGetRequest(const NProtobuf::TCacheGetRequest& cacheGetRequest) {
    Metrics_.PushRate("get", "added");
    CachalotClient_.AddCacheGetRequest(cacheGetRequest);
}
void TTtsCache::ProcessCacheWarmUpRequest(const NProtobuf::TCacheWarmUpRequest& cacheWarmUpRequest) {
    Metrics_.PushRate("warmup", "added");
    CachalotClient_.AddCacheWarmUpRequest(cacheWarmUpRequest);
}

void TTtsCache::Cancel() {
    Metrics_.PushRate("cancel", "added");
    CachalotClient_.CancelAll();
}
