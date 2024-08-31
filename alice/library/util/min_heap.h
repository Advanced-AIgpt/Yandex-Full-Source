#pragma once

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/system/yassert.h>

#include <cstddef>
#include <functional>
#include <utility>

namespace NAlice {

template <typename T>
class TMinHeap {
public:
    const T& GetMin() const {
        Y_ASSERT(!Empty());
        return Heap.front();
    }

    void Push(const T& t) {
        Heap.push_back(t);
        PushHeap(Heap.begin(), Heap.end(), std::greater<T>{});
    }

    template <typename... TArgs>
    void Emplace(TArgs&&... args) {
        Heap.emplace_back(std::forward<TArgs>(args)...);
        PushHeap(Heap.begin(), Heap.end(), std::greater<T>{});
    }

    void ExtractMin() {
        Y_ASSERT(!Empty());
        PopHeap(Heap.begin(), Heap.end(), std::greater<T>{});
        Heap.pop_back();
    }

    size_t Size() const {
        return Heap.size();
    }

    bool Empty() const {
        return Heap.empty();
    }

    TVector<T> Steal() {
        TVector<T> result;
        result.swap(Heap);
        return result;
    }

    void Clear() {
        Heap.clear();
    }

    auto begin() { return Heap.begin(); }
    auto end() { return Heap.end(); }

    auto begin() const { return Heap.begin(); }
    auto end() const { return Heap.end(); }

    auto cbegin() const { return Heap.cbegin(); }
    auto cend() const { return Heap.cend(); }

private:
    TVector<T> Heap;
};

} // namespace NAlice
