#pragma once

#include <alice/cachalot/library/storage/inmemory/ordered_policy_base.h>


namespace NCachalot::NPrivate {

template <typename TKey>
class TLruPolicy : public TOrderedPolicyBase<TLruPolicy<TKey>, TKey> {
private:
    using TBase = TOrderedPolicyBase<TLruPolicy<TKey>, TKey>;
    friend TBase;

public:
    void OnTouch(const TKey& key) {
        TBase::OnTouchImpl(key, TBase::GetOrderBegin());
    }

protected:
    void OnRemove(typename TBase::TListIterator /* posToRemove */) {
    }
};

}  // namespace NCachalot::NPrivate
