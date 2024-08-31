#pragma once

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/system/mutex.h>

namespace NGranet {

template<class TKey, class TValue>
class TSimpleCache {
public:
    TSimpleCache() {
    }

    TSimpleCache(size_t cacheLimit)
        : CacheLimit(cacheLimit)
    {
    }

    size_t GetCacheLimit() const {
        return CacheLimit;
    }

    size_t SetCacheLimit(size_t cacheLimit) {
        CacheLimit = cacheLimit;
    }

    TMaybe<TValue> Find(const TKey& key) {
        with_lock (Lock) {
            if (TValue* ptr = Cache1.FindPtr(key); ptr != nullptr) {
                return *ptr;
            }
            if (const auto it = Cache2.find(key); it != Cache2.end()) {
                TValue& ref = Cache1[key];
                ref = std::move(it->second);
                Cache2.erase(it);
                return ref;
            }
            return Nothing();
        }
    }

    void Insert(const TKey& key, TValue value) {
        with_lock (Lock) {
            if (Cache1.size() >= CacheLimit) {
                Cache2 = std::move(Cache1);
                Cache1.clear();
            }
            if (Cache2.contains(key)) {
                return;
            }
            const auto [it, isNew] = Cache1.try_emplace(key);
            if (!isNew) {
                return;
            }
            it->second = std::move(value);
        }
    }

private:
    size_t CacheLimit = 0;
    THashMap<TKey, TValue> Cache1;
    THashMap<TKey, TValue> Cache2;
    TMutex Lock;
};

}; // namespace NGranet
