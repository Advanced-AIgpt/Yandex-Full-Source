#pragma once

#include <alice/cachalot/library/modules/cache/storage.h>
#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/thread/pool.h>

namespace NCachalot {

struct TRequestCacheSettings {
    uint64_t TtlSecondsForYdb = 0;
    uint64_t TtlSecondsForLocalStorage = 0;
    uint64_t AllowedTtlVariationSeconds = 0;
    bool EnableInterCacheUpdates = false;
};

struct TRequestCacheContext : public TThrRefBase {
    struct TKvStorageContext {
        TIntrusivePtr<TSimpleKvStorage> Imdb;
        TIntrusivePtr<TSimpleKvStorage> Redis;
        TIntrusivePtr<TSimpleKvStorage> Ydb;

        bool ImdbEnabled() { return Imdb.Get() != nullptr; }
        bool RedisEnabled() { return Redis.Get() != nullptr; }
        bool YdbEnabled() { return Ydb.Get() != nullptr; }
    };

    TRequestCacheSettings Settings;
    TKvStorageContext Storage;
    TThreadPool BackgroundThreadPool;
    TCacheServiceMetrics* ServiceMetrics;
};

using TRequestCacheContextPtr = TIntrusivePtr<TRequestCacheContext>;

class TStorageTag2Context {
public:
    explicit TStorageTag2Context(const TCacheServiceSettings& settings);

    TRequestCacheContextPtr operator[](const TString& storageTag) const;

    // Used for backward compatability
    TRequestCacheContextPtr Default() const { return this->operator[]("Tts"); };

private:
    TMap<TString, TRequestCacheContextPtr> Contexts;
};

}
