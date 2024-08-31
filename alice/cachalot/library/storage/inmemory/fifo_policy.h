#pragma once

#include <alice/cachalot/library/storage/inmemory/ordered_policy_base.h>


namespace NCachalot::NPrivate {

template <typename TKey>
class TFifoPolicy : public TOrderedPolicyBase<TFifoPolicy<TKey>, TKey> {
private:
    using TBase = TOrderedPolicyBase<TFifoPolicy<TKey>, TKey>;
    friend TBase;

public:
    void OnTouch(const TKey& /* key */) {
        // If we do not touch items in ordered policy, they keep original order.
    }

protected:
    void OnRemove(typename TBase::TListIterator /* posToRemove */) {
    }
};

}  // namespace NCachalot::NPrivate
