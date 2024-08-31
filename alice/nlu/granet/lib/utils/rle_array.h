#pragma once

#include "serialization.h"
#include <util/generic/vector.h>
#include <util/ysaveload.h>

namespace NGranet {

template <class T>
class TRleArrayIterator;

// ~~~~ TRleArray ~~~~

template<class T>
class TRleArray {
public:
    using value_type = T;
    using size_type = size_t;
    using reference = const T&;
    using const_reference = const T&;
    using const_iterator = TRleArrayIterator<T>;
    using iterator = const_iterator;

    TRleArray() = default;

    inline explicit TRleArray(size_t count)
    {
        Values.resize(1);
        Endings.push_back(count);
    }

    inline TRleArray(size_t count, const T& value)
    {
        Values.push_back(value);
        Endings.push_back(count);
    }

    inline TRleArray(std::initializer_list<T> list)
    {
        for (const auto& value : list) {
            push_back(value);
        }
    }

    template <class Iter>
    inline TRleArray(Iter first, Iter last)
    {
        for (Iter it = first; it != last; ++it) {
            push_back(*it);
        }
    }

    TRleArray& operator=(std::initializer_list<T> list) {
        *this = TRleArray(list);
        return *this;
    }

    inline bool operator==(const TRleArray& other) const {
        return Endings == other.Endings && Values == other.Values;
    }

    inline bool operator!=(const TRleArray& other) const {
        return !(*this == other);
    }

    inline void push_back(T&& value) {
        PushBackImpl(value);
    }

    inline void push_back(const T& value) {
        PushBackImpl(value);
    }

    inline bool empty() const {
        return Endings.empty();
    }

    inline void clear() {
        Endings.clear();
        Values.clear();
    }

    inline size_t size() const {
        return Endings.empty() ? 0 : Endings.back();
    }

    inline yssize_t ysize() const {
        return static_cast<yssize_t>(size());
    }

    inline const T& operator[](size_t index) const {
        return Values[ToGroupIndex(index)];
    }

    inline const T& back() const {
        return Values.back();
    }

    inline const_iterator begin() const noexcept {
        return const_iterator(this);
    }

    inline const_iterator end() const noexcept {
        return const_iterator();
    }

    inline size_t GetGroupCount() const {
        Y_ASSERT(Values.size() == Endings.size());
        return Endings.size();
    }

    inline size_t GetGroupEnd(size_t index) const {
        return Endings[index];
    }

    inline size_t GetGroupLength(size_t index) const {
        return index > 0 ? Endings[index] - Endings[index - 1] : Endings[index];
    }

    inline const T& GetGroupValue(size_t index) const {
        Y_ASSERT(index < Values.size());
        return Values[index];
    }

    void Save(IOutputStream* output) const {
        ::Save(output, Values);
        size_t prev = 0;
        for (const size_t curr : Endings) {
            SaveSmallSize(output, curr - prev);
            prev = curr;
        }
    }

    void Load(IInputStream* input) {
        ::Load(input, Values);
        size_t pos = 0;
        Endings.resize(Values.size());
        for (size_t& ending : Endings) {
            pos += LoadSmallSize(input);
            ending = pos;
        }
    }

private:
    template<class Arg>
    void PushBackImpl(Arg&& value) {
        Y_ASSERT(Values.size() == Endings.size());
        if (Values.empty()) {
            Endings.push_back(1);
            Values.push_back(std::forward<Arg>(value));
            return;
        }
        if (Values.back() == value) {
            Endings.back()++;
            return;
        }
        Endings.push_back(Endings.back() + 1);
        Values.push_back(std::forward<Arg>(value));
    }

    inline size_t ToGroupIndex(size_t index) const {
        return UpperBound(Endings.begin(), Endings.end(), index) - Endings.begin();
    }

private:
    // Parallel arrays:
    //   Endings - end positions of each group interval.
    //   Values - values of each group.
    TVector<size_t> Endings;
    TVector<T> Values;
};

// ~~~~ TRleArrayIterator ~~~~

template <class T>
class TRleArrayIterator {
public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = const T*;
    using reference = const T&;
    using iterator_category = std::forward_iterator_tag;

    TRleArrayIterator() = default;

    inline explicit TRleArrayIterator(const TRleArray<T>* array) noexcept
        : Array(array)
    {
        if (Array != nullptr && Array->empty()) {
            Array = nullptr;
        }
    }

    inline bool operator==(const TRleArrayIterator& it) const noexcept {
        return Array == it.Array && Index == it.Index;
    }

    inline bool operator!=(const TRleArrayIterator& it) const noexcept {
        return !(*this == it);
    }

    inline pointer operator->() const noexcept {
        Y_ASSERT(Array != nullptr);
        return Array->GetGroupValue(GroupIndex);
    }

    inline reference operator*() const noexcept {
        Y_ASSERT(Array != nullptr);
        return Array->GetGroupValue(GroupIndex);
    }

    TRleArrayIterator& operator++() noexcept {
        Y_ASSERT(Array != nullptr);
        Y_ASSERT(Index < Array->size());
        Index++;
        const size_t ending = Array->GetGroupEnd(GroupIndex);
        if (Index >= ending) {
            Y_ASSERT(Index == ending);
            GroupIndex++;
        }
        if (GroupIndex == Array->GetGroupCount()) {
            *this = TRleArrayIterator();
        }
        return *this;
    }

private:
    const TRleArray<T>* Array = nullptr;
    size_t Index = 0;
    size_t GroupIndex = 0;
};

} // namespace NGranet
