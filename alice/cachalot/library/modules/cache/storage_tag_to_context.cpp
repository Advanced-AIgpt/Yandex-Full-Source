#include <alice/cachalot/library/modules/cache/storage_tag_to_context.h>

#include <alice/cachalot/library/storage/redis.h>
#include <alice/cuttlefish/library/logging/dlog.h>

namespace NCachalot {

namespace {

TRequestCacheContextPtr MakeRequestContext(const TCacheSettings& cacheSettings, const TString& storageTag) {
    auto requestContext = MakeIntrusive<TRequestCacheContext>();

    requestContext->Settings.TtlSecondsForYdb = cacheSettings.Ydb().TimeToLiveSeconds();
    requestContext->Settings.TtlSecondsForLocalStorage = cacheSettings.Imdb().TimeToLiveSeconds();
    requestContext->Settings.AllowedTtlVariationSeconds = cacheSettings.AllowedTtlVariationSeconds();
    requestContext->Settings.EnableInterCacheUpdates = cacheSettings.EnableInterCacheUpdates();

    if (cacheSettings.HasImdb()) {
        auto& metrics = TMetrics::GetInstance().CacheServiceMetrics.Imdb[storageTag];
        requestContext->Storage.Imdb = MakeSimpleImdbStorage(cacheSettings.Imdb(), &metrics);
    } else {
        DLOG("TCacheService::TCacheService Imdb is not configured for " + storageTag);
    }

    if (cacheSettings.HasYdb()) {
        requestContext->Storage.Ydb = MakeSimpleYdbStorage(
            cacheSettings.Ydb(),
            cacheSettings.YdbOperationSettings()
        );
    } else {
        DLOG("TCacheService::TCacheService Ydb is not configured for " + storageTag);
    }

    // Threads created for executing Background Set operation.
    // Background Set can be used for refreshing local cache after time-consuming write operation in YDB (Global cache).
    // However, it doesn't make sense, when YDB is disabled, so background threads allocation doesn't make sense as well
    if (requestContext->Storage.YdbEnabled() && requestContext->Settings.EnableInterCacheUpdates) {
        requestContext->BackgroundThreadPool.Start(
            cacheSettings.ThreadPoolConfig().NumberOfThreads(),
            cacheSettings.ThreadPoolConfig().QueueSize());
    } else {
        DLOG("TCacheService::TCacheService Background Set disabled for " + storageTag);
    }

    requestContext->ServiceMetrics = &TMetrics::GetInstance().CacheServiceMetrics;

    return requestContext;
}

}

TStorageTag2Context::TStorageTag2Context(const TCacheServiceSettings& settings) {
    for (const auto& [storageTag, cacheSettings] : settings.Storages()) {
        Contexts[storageTag] = MakeRequestContext(cacheSettings, storageTag);
    }
}

TRequestCacheContextPtr TStorageTag2Context::operator[](const TString& storageTag) const {
    if (auto it = Contexts.find(storageTag); it != Contexts.end()) {
        return it->second;
    } else {
        throw yexception()
            << "Unexpected StorageTag: " << storageTag << ". "
            << "Please, use an existing one or add missed configuration "
            << "into \"Storages\" section of the cachalot.json file";
    }
}

}
