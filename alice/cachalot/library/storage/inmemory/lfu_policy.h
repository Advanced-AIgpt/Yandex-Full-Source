#pragma once

#include <alice/cachalot/library/storage/inmemory/utils.h>

#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <util/generic/maybe.h>

#include <unordered_set>


namespace NCachalot::NPrivate {

template <typename TKey>
class TLfuPolicy {
    // Implementation with TMap is slow but simple.
    // Simplicity of implementation of main logic allows us to implement expiration logic.
private:
    using TKeySet = std::unordered_set<TKey, THashWithSalt<TKey, 0x22876cb7dc0229c1ull>>;

    static constexpr int64_t InitialUsage = 1;
    static constexpr int64_t OnTouchDelta = 1;

public:
    void OnTouch(const TKey& key) {
        auto usageIter = RemoveKeyFromCurrentGroup(key);
        if (Y_LIKELY(usageIter != Key2Usage.end())) {
            int64_t& usageCount = usageIter->second;
            usageCount += OnTouchDelta;
            Usage2Keys[usageCount].insert(key);
        }
    }

    void OnNewItem(const TKey& key) {
        Key2Usage.emplace(key, InitialUsage);
        Usage2Keys[InitialUsage].insert(key);
    }

    TMaybe<TKey> GetKeyToRemove() {
        if (Y_UNLIKELY(Usage2Keys.empty())) {
            return Nothing();
        }

        // Removing a key from the first group.
        // First group (begin of ordered TMap) corresponds to least used keys.
        auto groupPos = Usage2Keys.begin();

        TKeySet& group = groupPos->second;

        if (Y_LIKELY(!group.empty())) {
            const TKey keyToRemove = *group.begin();
            RemoveKeyFromCurrentGroupImpl(keyToRemove, groupPos);
            Key2Usage.erase(keyToRemove);
            return keyToRemove;
        } else {
            Y_ASSERT(false);
            Usage2Keys.erase(groupPos);
        }

        return Nothing();
    }

    void RemoveExpiredKey(const TKey& key) {
        RemoveKeyFromCurrentGroup(key);
    }

    uint64_t CalcMemoryUsage(const TKey& key) const {
        return GetMemoryUsage(key) * 2 + 32;
    }

private:
    // Returns Key2Usage::iterator pointing on usageCount of given key.
    auto RemoveKeyFromCurrentGroup(const TKey& key) {
        auto usageIter = Key2Usage.find(key);
        if (Y_LIKELY(usageIter != Key2Usage.end())) {
            const int64_t usageCount = usageIter->second;
            RemoveKeyFromCurrentGroupImpl(key, Usage2Keys.find(usageCount));
        }
        return usageIter;
    }

    void RemoveKeyFromCurrentGroupImpl(const TKey& key, typename TMap<int64_t, TKeySet>::iterator groupPos) {
        if (Y_LIKELY(groupPos != Usage2Keys.end())) {
            TKeySet& group = groupPos->second;
            group.erase(key);
            if (group.empty()) {
                Usage2Keys.erase(groupPos);
            }
        }
    }

private:
    TMap<int64_t, TKeySet> Usage2Keys;
    THashMap<TKey, int64_t, THashWithSalt<TKey, 0xd5a8e1c6086169b6ull>> Key2Usage;
};

}  // namespace NCachalot::NPrivate
