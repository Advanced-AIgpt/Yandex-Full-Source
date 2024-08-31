#pragma once

#include <alice/cachalot/library/storage/inmemory/utils.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>

#include <list>


namespace NCachalot::NPrivate {

template <typename TDerived, typename TKey>
class TOrderedPolicyBase {
protected:
    using TList = std::list<TKey>;
    using TListIterator = typename TList::const_iterator;
    using TIteratorMap = THashMap<TKey, TListIterator, THashWithSalt<TKey, 0xa2ccd4b017c77e59ull>>;

    using TSelf = TOrderedPolicyBase<TDerived, TKey>;

protected:
    TListIterator OnTouchImpl(const TKey& key, TListIterator posToInsert) {
        auto pos = Key2ListNode.find(key);
        if (Y_LIKELY(pos != Key2ListNode.end())) {
            // Using the same key object to optimize amount of memory used by COW-strings.
            const auto insertedPos = KeyOrder.insert(posToInsert, pos->first);
            KeyOrder.erase(pos->second);
            pos->second = insertedPos;
            return insertedPos;
        }
        return posToInsert;
    }

public:
    void OnNewItem(const TKey& key) {
        Key2ListNode[key] = KeyOrder.insert(KeyOrder.begin(), key);
    }

    TMaybe<TKey> GetKeyToRemove() {
        if (Y_UNLIKELY(Key2ListNode.empty())) {
            return Nothing();
        }

        const TKey keyToRemove = KeyOrder.back();
        RemoveKey(keyToRemove);
        return keyToRemove;
    }

    void RemoveExpiredKey(const TKey& key) {
        RemoveKey(key);
    }

    uint64_t CalcMemoryUsage(const TKey& key) const {
        return GetMemoryUsage(key) + 64;
    }

private:
    void RemoveKey(const TKey& key) {
        auto pos = Key2ListNode.find(key);
        Y_ASSERT(pos != Key2ListNode.end());

        static_assert(std::is_base_of_v<TSelf, TDerived>);
        static_cast<TDerived*>(this)->OnRemove(pos->second);
        KeyOrder.erase(pos->second);

        Key2ListNode.erase(pos);
    }

protected:
    TListIterator GetOrderBegin() const {
        return KeyOrder.begin();
    }

    TListIterator GetOrderEnd() const {
        return KeyOrder.end();
    }

private:
    TList KeyOrder;
    TIteratorMap Key2ListNode;
};

}  // namespace NCachalot::NPrivate
