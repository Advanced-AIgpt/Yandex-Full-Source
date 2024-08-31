#include "cachalot_cache.h"

namespace NAlice::NAppHostServices {

    NCachalotProtocol::TGetRequest TCachalotCache::MakeGetRequest(
        TString cacheKey,
        TString storageTag
    ) {
        NCachalotProtocol::TGetRequest request;

        request.SetKey(std::move(cacheKey));
        request.SetStorageTag(std::move(storageTag));

        return request;
    }

    NCachalotProtocol::TSetRequest TCachalotCache::MakeSetRequest(
        TString cacheKey,
        TString data,
        TString storageTag
    ) {
        NCachalotProtocol::TSetRequest request;

        request.SetKey(std::move(cacheKey));
        request.SetData(std::move(data));
        request.SetStorageTag(std::move(storageTag));

        return request;
    }

    NCachalotProtocol::TSetRequest TCachalotCache::MakeSetRequest(
        TString cacheKey,
        TString data,
        TString storageTag,
        uint64_t ttlSeconds
    ) {
        NCachalotProtocol::TSetRequest request = MakeSetRequest(cacheKey, data, storageTag);

        request.SetTTL(ttlSeconds);

        return request;
    }

    NCachalotProtocol::TDeleteRequest TCachalotCache::MakeDeleteRequest(
        TString cacheKey,
        TString storageTag
    ) {
        NCachalotProtocol::TDeleteRequest request;

        request.SetKey(std::move(cacheKey));
        request.SetStorageTag(std::move(storageTag));

        return request;
    }

}
