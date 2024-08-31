#pragma once

#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NGranet {

template <class T>
class TOrderedSet {
public:
    size_t Insert(const T& value) {
        const auto [it, isNew] = ValueToIndex.try_emplace(value, Values.size());
        if (isNew) {
            Values.push_back(value);
        }
        return it->second;
    }

    const TVector<T>& GetValues() const {
        return Values;
    }

    TVector<T> ReleaseValues() {
        ValueToIndex.clear();
        return std::move(Values);
    }

private:
    TVector<T> Values;
    THashMap<T, size_t> ValueToIndex;
};

} // namespace NGranet
