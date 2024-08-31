#pragma once

#include "small_stack.h"

template <typename T, size_t SmallSize>
class TSmallTypedAllocator {
public:
    template <typename... TArgs>
    T& Alloc(TArgs&&... args) {
        if (FreeIndices.empty()) {
            auto& valueRef = Storage.emplace_back(std::forward<TArgs>(args)...);
            valueRef.index = Storage.size() - 1;
            return valueRef;
        }
        auto index = FreeIndices.back();
        FreeIndices.pop_back();
        auto& valueRef = Storage[index];
        valueRef = T{std::forward<TArgs>(args)...};
        valueRef.index = index;
        return valueRef;
    }

    void Free(const T& value) {
        FreeIndices.emplace_back(value.index);
    }

    void Clear() {
        Storage.clear();
        FreeIndices.clear();
    }

private:
    friend struct TAllocatorTest;
    TSmallStack<T, SmallSize> Storage;
    TSmallStack<uint32_t, SmallSize> FreeIndices;
};
