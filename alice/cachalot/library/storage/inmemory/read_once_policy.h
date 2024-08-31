#pragma once

#include <alice/cachalot/library/storage/inmemory/ordered_policy_base.h>

#include <util/generic/maybe.h>


namespace NCachalot::NPrivate {

template <typename TKey>
class TReadOncePolicy : public TOrderedPolicyBase<TReadOncePolicy<TKey>, TKey> {
private:
    using TBase = TOrderedPolicyBase<TReadOncePolicy<TKey>, TKey>;
    friend TBase;

public:
    void OnTouch(const TKey& key) {
        LastRemovedOnTouchPos = TBase::OnTouchImpl(key, LastRemovedOnTouchPos.GetOrElse(TBase::GetOrderEnd()));
    }

protected:
    void OnRemove(typename TBase::TListIterator posToRemove) {
        if (LastRemovedOnTouchPos.Defined() && posToRemove == LastRemovedOnTouchPos.GetRef()) {
            LastRemovedOnTouchPos = Nothing();
        }
    }

private:
    // Not removed actually. We just put touched keys to the right part of order list,
    // so they will be removed before those which are not read yet.
    TMaybe<typename TBase::TListIterator> LastRemovedOnTouchPos = Nothing();
};

}  // namespace NCachalot::NPrivate
